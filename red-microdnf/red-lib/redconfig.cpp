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
#include <iostream>
#include <fstream>
#include <rpm/rpmmacro.h>

#include <context.hpp>

extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/repo_rpmdb.h>
}


namespace redlib {


bool RedNode::isRedpath(bool strict) const {
    bool ret = redpathOpt.empty();
    if (!ret && strict)
        throw_error("No redpath set! Aborting...");
    return ret;
}

void RedNode::addOptions(libdnf::cli::ArgumentParser::Command *microdnf) {
    auto redpath_opt = arg_parser.add_new_named_arg("redpath");
    redpath_opt->set_long_name("redpath");
    redpath_opt->set_has_value(true);
    redpath_opt->set_arg_value_help("ABSOLUTE_PATH");
    redpath_opt->set_short_description("redpath path");
    redpath_opt->link_value(&redpathOpt);
    microdnf->register_named_arg(redpath_opt);

    auto forcenode_opt = arg_parser.add_new_named_arg("forcenode");
    forcenode_opt->set_long_name("forcenode");
    forcenode_opt->set_has_value(false);
    forcenode_opt->set_short_description("force node");
    forcenode_opt->link_value(&forcenode);
    microdnf->register_named_arg(forcenode_opt);
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
    base.get_config().installroot().set(libdnf::Option::Priority::RUNTIME, redpath());


    //prepend redpath in repodirs
    std::vector<std::string> reposdir(base.get_config().reposdir().get_value());
    for (auto i = reposdir.begin(); i != reposdir.end(); ++i)
        i->insert(0, redpath());
    base.get_config().reposdir().set(libdnf::Option::Priority::RUNTIME, reposdir);
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

    node =  std::unique_ptr<redNodeT>(RedNodesScan(redpath().c_str(), 0));
    config = std::unique_ptr<redConfigT>(node.get()->config);
	if (!node.get()) {
        throw_error("Issue RedNodeScan");
	}
}

// get main /etc/repack/main.yaml and set umask default value
void RedNode::getMain() {

    getConf();

    if (!node.get()->config->acl->umask)
        umask(00077);
    else
        umask(node.get()->config->acl->umask);
}

void RedNode::setPersistDir() {
	auto ex_persistdir = RedNodeStringExpand(node.get(), NULL, node.get()->config->conftag->persistdir, NULL, NULL);
	base.get_config().persistdir().set(libdnf::Option::Priority::RUNTIME, ex_persistdir);
}

void RedNode::setGpgCheck() {
	base.get_config().gpgcheck().set(libdnf::Option::Priority::RUNTIME, node.get()->config->conftag->gpgcheck);
}

void RedNode::setCacheDir() {
    const char *cachedir = NULL;
    for (redNodeT *ancestor_node=node->ancestor; ancestor_node != NULL; ancestor_node=ancestor_node->ancestor) {
    	if(ancestor_node->config->conftag->cachedir) {
		cachedir = ancestor_node->config->conftag->cachedir;
		break;
	}
    }

    if(!cachedir) {
	throw_error(fmt::format("No cachedir in conftag for node: {}", node->redpath));
	}
    base.get_config().cachedir().set(libdnf::Option::Priority::RUNTIME, cachedir);
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
    if(!node.get())
        throw_error("Need node in apppendFamilyDb!");

    // Scan redpath family nodes from terminal leaf to root node
    for (redNodeT *ancestor_node=node.get()->ancestor; ancestor_node != NULL; ancestor_node=ancestor_node->ancestor) {
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

void RedNode::saveto(bool update, const std::string & var_rednode, std::unique_ptr<redConfigT> & redconfig) {

    std::unique_ptr<const char> confpath_ptr(RedGetDefaultExpand(NULL, NULL, var_rednode.c_str()));
    if (!confpath_ptr.get())
        throw_error(fmt::format("Cannot find path to {} variable", var_rednode));

    std::filesystem::path confpath(confpath_ptr.get());
    auto confdir = confpath.parent_path();
    std::filesystem::create_directories(confdir);


    if (update && std::filesystem::exists(confpath))
        throw_error(fmt::format("Rednode config already exist [use --update] confpath={}", confpath.c_str()));

    if(RedSaveConfig(confpath.c_str(), redconfig.get(), 0))
        throw_error(fmt::format("Fail to push rednode config at path={}", confpath.c_str()));

}

void RedNode::date(char *today, std::size_t size) {
    std::time_t t = std::time(nullptr);
    if (!std::strftime(today, size, "%d:%b:%Y %H:%M (%Z)", std::localtime(&t)))
        throw_error("strftime issue");
}

void RedNode::reloadConfig(const std::string & tmplname, std::unique_ptr<redConfigT> & redconfig) {
    std::unique_ptr<const char> tmpldir(RedGetDefaultExpand(NULL, NULL, "redpak_TMPL"));
    std::filesystem::path tmplpath(fmt::format("{}/{}.yaml", tmpldir.get(), tmplname));

    std::string tmp_tpl = expandTplFile(tmplpath);

    redconfig = std::unique_ptr<redConfigT>(RedLoadConfig(tmp_tpl.c_str(), 0));
    //remove config file
    int ret_code = std::remove(tmp_tpl.c_str());
    if(ret_code)
        throw_error(fmt::format("Cannot remove temporary file: err={}", ret_code));

    if (!config)
        throw_error(fmt::format("Issue RedLoadConfig with template={}", tmplname));

}

void RedNode::rednode_template(const std::string & redpath, const std::string & alias, const std::string & tmplname,
                               const std::string & tmpladmin, bool update) {
    char today[100];
    char uuid[100];

    date(today, sizeof(today));
    get_uuid(uuid);

    setenv("NODE_ALIAS", alias.c_str(), true);
    setenv("NODE_UUID", uuid, true);
    setenv("TODAY", today, true);

    //save config file
    reloadConfig(tmplname, config);
    //system: create common cachedir
    if(!tmplname.compare("main-system")) {
        std::filesystem::create_directories(config->conftag->cachedir);
    }

    saveto(update, "REDNODE_CONF", config);

    //save admin config file
    reloadConfig(tmpladmin, configadmin);
    saveto(update, "REDNODE_ADMIN", configadmin);
}

unsigned long RedNode::timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void RedNode::rednode_status(const std::string & realpath) {

    char today[100];
    date(today, sizeof(today));

    std::unique_ptr<const char> rednode_status_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_STATUS"));
    std::filesystem::path rednode_status_path(rednode_status_ptr.get());

    std::string info = fmt::format("created by red-manager the {}", today);
    // create default node status
    redStatusT status = {
        .state = RED_STATUS_ENABLE,
        .realpath = realpath.c_str(),
        .timestamp = timestamp(),
        .info = info.c_str()
    };

    if (RedSaveStatus(rednode_status_path.c_str(), &status, 0))
        throw_error(fmt::format("Fail to push rednode status at path={}", rednode_status_path.c_str()));
}

void RedNode::createRedNodePath(const std::string & redpath, const std::string & alias, bool create, bool update, const std::string & tmplate, const std::string & tmplateadmin) {
    setenv("NODE_PATH", redpath.c_str(), true);

    checkdir("redpath", redpath, true);

    rednode_template(redpath, alias, tmplate, tmplateadmin, update);
    rednode_status(redpath);
}

void RedNode::createRedNode(const std::string & alias, bool create, bool update, const std::string & tmplate, const std::string & tmplateadmin) {
    if(!active)
        throw_error("No redpath: aborting...");

    //check parent exists
    auto parent_path = std::filesystem::path(redpath()).parent_path();
    if(!std::filesystem::exists(parent_path / REDNODE_STATUS)) {
        //create system node
        std::string alias_system = "system" + parent_path.string();
        std::replace(alias_system.begin(), alias_system.end(), '/', '-');
        createRedNodePath(parent_path.string(), alias_system, false, false, "main-system", "main-admin-system");

        //create symlink to /var/lib/rpm of system
        std::filesystem::path var_lib_rpm("/var/lib/rpm");
        std::filesystem::path var_lib_rpm_system_node(parent_path / var_lib_rpm.string().substr(1));
        std::filesystem::create_directories(var_lib_rpm_system_node.parent_path());
        std::filesystem::create_directory_symlink(var_lib_rpm, var_lib_rpm_system_node);
    }

    createRedNodePath(redpath(), alias, create, update, tmplate, tmplateadmin);
}

void RedNode::install(libdnf::rpm::PackageSack & package_sack) {
    if (!active)
        return;

    scanNode();
    setPersistDir();
    setGpgCheck();
    setCacheDir();

    checkdir("redpath", redpath(), false);
    checkdir("dnf persistdir", base.get_config().persistdir().get_value(), false);
    appendFamilyDb(package_sack);
}

bool RedNode::checkInNodeDataBase(std::string name) {
    bool present = false;
    Pool * pool = pool_create();
    pool_set_rootdir(pool, redpath().c_str());
    Repo * repo = repo_create(pool, "redpak");
    int flagsrpm = REPO_REUSE_REPODATA | RPM_ADD_WITH_HDRID | REPO_USE_ROOTDIR;
    int rc = repo_add_rpmdb(repo, nullptr, flagsrpm);
    void *rpmstate;
	Queue q;

	queue_init(&q);
	rpmstate = rpm_state_create(pool, redpath().c_str());
	rpm_installedrpmdbids(rpmstate, "Name", name.c_str(), &q);
   printf("CLEMENT: %s %d\n", name.c_str(), q.count);
    if (q.count)
        present = true;
	rpm_state_free(rpmstate);
	queue_free(&q);
    pool_free(pool);
    return present;
}

std::vector<libdnf::base::TransactionPackage> RedNode::checkTransactionPkgs(libdnf::base::Transaction & transaction) {
    std::string log_pkgs;

    auto tpkgs = transaction.get_transaction_packages();
    for (auto tpkg = tpkgs.begin(); tpkg != tpkgs.end(); tpkg++) {
        auto rpmdbid = tpkg->get_package().get_rpmdbid();
        if (rpmdbid == 0) //not installed
            continue;

        if (!checkInNodeDataBase(tpkg->get_package().get_name())) {
            log_pkgs += fmt::format(" {}", tpkg->get_package().get_full_nevra());
            tpkgs.erase(tpkg--);
            tpkgs.erase(tpkg--);
        }
    }

    if(!log_pkgs.empty()) {
        std::cout << fmt::format("Cannot installed/upgrade because rpms already presents in parent nodes:\n{}\n", log_pkgs);
        if(!forcenode.get_value())
            throw_error("Aborting... (Use --forcenode to force the installation in the node");
    }
	return tpkgs;

}

} //end namespace
