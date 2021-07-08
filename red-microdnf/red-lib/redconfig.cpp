/*
 Copyright 2021 IoT.bzh

 author: Clément Bénier <clement.benier@iot.bzh>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "redconfig.hpp"

#include <stdexcept>
#include <cstdlib>
#include <sys/stat.h>
#include <regex>
#include <ctime>
#include <uuid/uuid.h>
#include <chrono>
#include <memory>
#include <fstream>
#include <rpm/rpmmacro.h>

#include <context.hpp>

namespace redlib {


bool RedNode::isRedpath(bool strict) const {
    bool ret = redpathOpt.empty();
    if (!ret && strict)
        throw_error("No redpath set! Aborting...");
    return ret;
}

void RedNode::addOptions(libdnf::cli::ArgumentParser::Command *microdnf) {
    auto redpath_opt = ctx->arg_parser.add_new_named_arg("redpath");
    redpath_opt->set_long_name("redpath");
    redpath_opt->set_has_value(true);
    redpath_opt->set_arg_value_help("ABSOLUTE_PATH");
    redpath_opt->set_short_description("redpath path");
    redpath_opt->link_value(&redpathOpt);
    microdnf->register_named_arg(redpath_opt);
}

void RedNode::configure() {

    // if no redpath return
    if (redpathOpt.empty())
        return;

    rpmPushMacro(NULL, "__transaction_redpak", NULL, "%{__plugindir}/redpak.so", RMIL_CMDLINE);

    //active rednode
    active = true;

    setenv("NODE_PATH", redpath().c_str(), true);

    //installroot: set to redpath
    ctx->base.get_config().installroot().set(libdnf::Option::Priority::RUNTIME, redpath());

    //prepend redpath in repodirs
    std::vector<std::string> reposdir(ctx->base.get_config().reposdir().get_value());
    for (auto i = reposdir.begin(); i != reposdir.end(); ++i)
        i->insert(0, redpath());
    ctx->base.get_config().reposdir().set(libdnf::Option::Priority::RUNTIME, reposdir);
}

void RedNode::scanNode() {
    // If we have no redpath or when redpath=/ than we seach for coreos redpack.yaml otherwise search for a rednode
	if (redpath() != "/") {
		getConf();
	} else {
		getMain();
	}
}

bool RedNode::hasConf() {
    return isnode;
}

void RedNode::getConf() {
	if (!std::filesystem::exists(redpath())) {
		if (strictmode) {
            throw_error(fmt::format("No file at path={}", redpath()));
		} else {
			isnode = false;
            fmt::print("Info: Node ignored [no redpack.yaml] path={} ()", redpath().c_str());
		}
	} else {
		isnode = true;
	}

    node =  RedNodesScan(redpath().c_str(), 0);
    config = node->config;
	if (!node) {
        throw_error("Issue RedNodeScan");
	}
}

// get main /etc/repack/main.yaml and set umask default value
void RedNode::getMain() {

    getConf();

    if (!node->config->acl->umask)
        umask(00077);
    else
        umask(node->config->acl->umask);
}

void RedNode::setPersistDir(libdnf::ConfigMain & dnfconfig) {
	auto ex_persistdir = RedNodeStringExpand(node, NULL, node->config->conftag->persistdir, NULL, NULL);
	dnfconfig.persistdir().set(libdnf::Option::Priority::RUNTIME, ex_persistdir);
}

void RedNode::setGpgCheck(libdnf::ConfigMain & dnfconfig) {
	dnfconfig.gpgcheck().set(libdnf::Option::Priority::RUNTIME, node->config->conftag->gpgcheck);
}

void RedNode::checkdir(const std::string & label, const std::string & dirpath, bool create) {
    if (create) {
        std::filesystem::create_directory(dirpath);
    }

    std::filesystem::perms perms = std::filesystem::status(dirpath).permissions();

    // check write permission on dirpath
    if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
        throw_error(fmt::format("{}: No permissions to write on {}", label, dirpath));

}

void RedNode::registerNode(redNodeT * node, libdnf::rpm::PackageSack & package_sack) {
    // make sure node is not disabled
    if (node->status->state !=  RED_STATUS_ENABLE) {
        throw_error(fmt::format("*** Node [{}""] is DISABLED [check/update node] nodepath={}", node->config->headers->alias, node->redpath));
    }

    // Expand node config to fit within redpath
    package_sack.append_extra_system_repo(node->redpath);
}

void RedNode::appendFamilyDb(libdnf::rpm::PackageSack & package_sack) {
    if(!node)
        throw_error("Need node in apppendFamilyDb!");

    // Scan redpath family nodes from terminal leaf to root node
    for (redNodeT *ancestor_node=node->ancestor; ancestor_node != NULL; ancestor_node=ancestor_node->ancestor) {
        registerNode(ancestor_node, package_sack);
    }
}

void RedNode::get_uuid(char * uuid_str) {
    uuid_t u;
    uuid_generate(u);
    uuid_parse(uuid_str, u);
}

std::string RedNode::expandTplFile(std::filesystem::path & tmplpath) {

    // tmp expand template file
    std::string tmp_path = std::tmpnam(nullptr);
    std::ofstream tmp_tpl(tmp_path);

    if(!tmp_tpl.is_open())
        throw_error("Cannot open tmpfile in write mode");

    //template file
    std::ifstream file(tmplpath);
    if (!file.is_open())
        throw_error(fmt::format("Cannot read confpath {}", tmplpath.string()));

    std::string line;
    while (std::getline(file, line)) {
        std::unique_ptr<const char> line_expand(RedNodeStringExpand(NULL, NULL, line.c_str(), NULL, NULL));
        tmp_tpl << line_expand.get() << std::endl;
    }

    file.close();
    tmp_tpl.close();
    return tmp_path;
}

std::filesystem::path RedNode::confpath() {
    return std::filesystem::path(RedGetDefaultExpand(NULL, NULL, "REDNODE_CONF"));
}

void RedNode::saveto(bool update) {

    std::unique_ptr<const char> confpath_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_CONF"));

    std::filesystem::path confpath(confpath_ptr.get());
    auto confdir = confpath.parent_path();
    std::filesystem::create_directories(confdir);


    if (update && std::filesystem::exists(confpath))
        throw_error(fmt::format("Rednode config already exist [use --update] confpath={}", confpath.c_str()));

    if(RedSaveConfig(confpath.c_str(), config, 0))
        throw_error(fmt::format("Fail to push rednode config at path={}", confpath.c_str()));

}

void RedNode::date(char *today, std::size_t size) {
    std::time_t t = std::time(nullptr);
    if (!std::strftime(today, size, "%d:%b:%Y %H:%M (%Z)", std::localtime(&t)))
        throw_error("strftime issue");
}

void RedNode::reloadConfig(const std::string & tmplname) {
    std::unique_ptr<const char> tmpldir(RedGetDefaultExpand(NULL, NULL, "redpak_TMPL"));
    std::filesystem::path tmplpath(fmt::format("{}/{}.yaml", tmpldir.get(), tmplname));

    std::string tmp_tpl = expandTplFile(tmplpath);

    config = RedLoadConfig(tmp_tpl.c_str(), 0);
    //remove config file
    int ret_code = std::remove(tmp_tpl.c_str());
    if(ret_code)
        throw_error(fmt::format("Cannot remove temporary file: err={}", ret_code));

    if (!config)
        throw_error(fmt::format("Issue RedLoadConfig with template={}", tmplname));

}

void RedNode::rednode_template(const std::string & alias, const std::string & tmplname, bool update) {
    char today[100];
    char uuid[100];

    date(today, sizeof(today));
    get_uuid(uuid);

    setenv("NODE_ALIAS", alias.c_str(), true);
    setenv("NODE_UUID", uuid, true);
    setenv("TODAY", today, true);

    reloadConfig(tmplname);
    saveto(update);
}

unsigned long RedNode::timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void RedNode::rednode_status() {

    char today[100];
    date(today, sizeof(today));

    std::unique_ptr<const char> rednode_status_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_STATUS"));
    std::filesystem::path rednode_status_path(rednode_status_ptr.get());

    std::string info = fmt::format("created by red-manager the {}", today);
    // create default node status
    redStatusT status = {
        .state = RED_STATUS_ENABLE,
        .realpath = redpath().c_str(),
        .timestamp = timestamp(),
        .info = info.c_str()
    };

    if (RedSaveStatus(rednode_status_path.c_str(), &status, 0))
        throw_error(fmt::format("Fail to push rednode status at path={}", rednode_status_path.c_str()));
}

void RedNode::createRedNode(const std::string & alias, bool create, bool update, const std::string & tmplate) {
    if(!active)
        throw_error("No redpath: aborting...");

    checkdir("redpath", redpath(), true);

    rednode_template(alias, tmplate, update);
    rednode_status();
}

void RedNode::install(libdnf::rpm::PackageSack & package_sack) {
    if (!active)
        return;

    scanNode();
    setPersistDir(ctx->base.get_config());
    setGpgCheck(ctx->base.get_config());

    checkdir("redpath", redpath(), false);
    checkdir("dnf persistdir", ctx->base.get_config().persistdir().get_value(), false);
    appendFamilyDb(package_sack);
}

} //end namespace
