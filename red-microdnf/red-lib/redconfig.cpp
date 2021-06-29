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
#include <fmt/format.h>
#include <chrono>
#include <memory>
#include <fstream>

namespace redlib {

std::string Environment::expandValue(const char * value) {
    std::string res(value);
    std::regex env_regex("(\\$\\{\\w+\\})|(\\$\\w+)");
    std::regex nonchar_regex("\\W+");

    auto words_begin = std::sregex_iterator(res.begin(), res.end(), env_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string  env_name = std::regex_replace(i->str(), nonchar_regex, "");
        char *value_env = std::getenv(env_name.c_str());
        std::string value_dest(value_env ? value_env : defaults[env_name]);
        std::regex value_regex(value_dest);
        res = std::regex_replace(res, value_regex, value_dest);
    }
    return res;
}

ParseNode::ParseNode(const std::filesystem::path & redpath, bool strictmode, int verbose): Environment(), redpath(redpath), strictmode(strictmode), verbose(verbose) {

    defaults["redpath"] = redpath.c_str();
    setenv("NODE_PATH", redpath.c_str(), true);
}

void ParseNode::scanNode() {
       // If we have no redpath or when redpath=/ than we seach for coreos redpack.yaml otherwise search for a rednode
	if (redpath != "/") {
		getConf();
	} else {
		getMain();
	}
}

bool ParseNode::hasConf() {
       return isnode;
}

void ParseNode::getConf() {
	if (!std::filesystem::exists(redpath)) {
		if (strictmode) {
       		throw std::runtime_error("No file at path=" + redpath.string());
		} else {
			isnode = false;
			printf("Info: Node ignored [no redpack.yaml] path=%s ()", redpath.c_str());
		}
	} else {
		isnode = true;
	}

    node =  RedNodesScan(redpath.c_str(), 0);
    config = node->config;
	if (!node) {
		throw std::runtime_error("Issue RedNodeScan");
	}
}

// get main /etc/repack/main.yaml and set umask default value
void ParseNode::getMain() {

    getConf();

    if (!node->config->acl->umask)
        umask(00077);
    else
        umask(node->config->acl->umask);
}

void ParseNode::setPersistDir(libdnf::ConfigMain & dnfconfig) {
	auto ex_persistdir = RedNodeStringExpand(node, NULL, node->config->conftag->persistdir, NULL, NULL);
	dnfconfig.persistdir().set(libdnf::Option::Priority::RUNTIME, ex_persistdir);
}

void ParseNode::setGpgCheck(libdnf::ConfigMain & dnfconfig) {
	dnfconfig.gpgcheck().set(libdnf::Option::Priority::RUNTIME, node->config->conftag->gpgcheck);
}

void ParseNode::checkdir(const std::string & label, const std::string & dirpath, bool create) {
    if (create) {
        std::filesystem::create_directory(dirpath);
    }

    std::filesystem::perms perms = std::filesystem::status(dirpath).permissions();

    // check write permission on dirpath
    if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
        throw std::runtime_error(label + ": No permissions to write on " + dirpath);

}

void ParseNode::registerNode(redNodeT * node, libdnf::rpm::PackageSack & package_sack) {
    // make sure node is not disabled
    if (node->status->state !=  RED_STATUS_ENABLE) {
        throw std::runtime_error("*** Node [" + std::string(node->config->headers->alias) + "] is DISABLED [check/update node] nodepath=" + std::string(node->redpath));
    }

    // Expand node config to fit within redpath
    auto fullpath = RedNodeStringExpand(node, nodeConfigDefaults, node->config->conftag->rpmdir, NULL, NULL);

    package_sack.append_extra_system_repo(fullpath);
}

void ParseNode::appendFamilyDb(libdnf::rpm::PackageSack & package_sack) {
    if(!node)
        throw std::runtime_error("Need node in apppendFamilyDb!");

    // Scan redpath family nodes from terminal leaf to root node
    for (redNodeT *ancestor_node=node->ancestor; ancestor_node != NULL; ancestor_node=ancestor_node->ancestor) {
        registerNode(ancestor_node, package_sack);
    }
}

void ParseNode::get_uuid(char * uuid_str) {
    uuid_t u;
    uuid_generate(u);
    uuid_parse(uuid_str, u);
}

std::string ParseNode::expandTplFile(std::filesystem::path & tmplpath) {

    // tmp expand template file
    std::string tmp_path = std::tmpnam(nullptr);
    std::ofstream tmp_tpl(tmp_path);

    if(!tmp_tpl.is_open())
        throw std::runtime_error("Cannot open tmpfile in write mode");

    //template file
    std::ifstream file(tmplpath);
    if (!file.is_open())
        throw std::runtime_error(fmt::format("Cannot read confpath {}", tmplpath.string()));

    std::string line;
    while (std::getline(file, line)) {
        std::unique_ptr<const char> line_expand(RedNodeStringExpand(NULL, NULL, line.c_str(), NULL, NULL));
        tmp_tpl << line_expand.get() << std::endl;
    }

    file.close();
    tmp_tpl.close();
    return tmp_path;
}

std::filesystem::path ParseNode::confpath() {
    return std::filesystem::path(RedGetDefaultExpand(NULL, NULL, "REDNODE_CONF"));
}

void ParseNode::saveto(bool update) {

    std::unique_ptr<const char> confpath_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_CONF"));

    std::filesystem::path confpath(confpath_ptr.get());
    auto confdir = confpath.parent_path();
    std::filesystem::create_directories(confdir);


    if (update && std::filesystem::exists(confpath))
        throw std::runtime_error(fmt::format("Rednode config already exist [use --update] confpath={}", confpath.c_str()));

    if(RedSaveConfig(confpath.c_str(), config, 0))
        throw std::runtime_error(fmt::format("Fail to push rednode config at path={}", confpath.c_str()));

}

void ParseNode::date(char *today, std::size_t size) {
    std::time_t t = std::time(nullptr);
    if (!std::strftime(today, size, "%d:%b:%Y %H:%M (%Z)", std::localtime(&t)))
        throw std::runtime_error("strftime issue");
}

void ParseNode::reloadConfig(const std::string & tmplname) {
    std::unique_ptr<const char> tmpldir(RedGetDefaultExpand(NULL, NULL, "redpak_TMPL"));
    std::filesystem::path tmplpath(fmt::format("{}/{}.yaml", tmpldir.get(), tmplname));

    std::string tmp_tpl = expandTplFile(tmplpath);

    config = RedLoadConfig(tmp_tpl.c_str(), 0);
    //remove config file
    int ret_code = std::remove(tmp_tpl.c_str());
    if(ret_code)
        throw std::runtime_error(fmt::format("Cannot remove temporary file: err=", ret_code));

    if (!config)
        throw std::runtime_error(fmt::format("Issue RedLoadConfig with template={}", tmplname));

}

void ParseNode::rednode_template(const std::string & alias, const std::string & tmplname, bool update) {
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

unsigned long ParseNode::timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void ParseNode::rednode_status() {

    char today[100];
    date(today, sizeof(today));

    std::unique_ptr<const char> rednode_status_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_STATUS"));
    std::filesystem::path rednode_status_path(rednode_status_ptr.get());

    std::string info = fmt::format("created by red-manager the {}", today);
    // create default node status
    redStatusT status = {
        .state = RED_STATUS_ENABLE,
        .realpath = redpath.c_str(),
        .timestamp = timestamp(),
        .info = info.c_str()
    };

    if (RedSaveStatus(rednode_status_path.c_str(), &status, 0))
        throw std::runtime_error(fmt::format("Fail to push rednode status at path={}", rednode_status_path.c_str()));
}

void ParseNode::createRedNode(const std::string & alias, bool create, bool update, const std::string & tmplate) {
    checkdir("redpath", redpath.string(), true);

    rednode_template(alias, tmplate, update);
    rednode_status();
}

} //end namespace
