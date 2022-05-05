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
#include <rpm/rpmlog.h>

extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/repo_rpmdb.h>
}


namespace redlib {


bool RedNode::isRedpath(bool strict) const {
    bool empty = redpath.empty();
    if (empty && strict)
        throw_error("No redpath set! Aborting...");
    return empty;
}

void RedNode::addOptions(libdnf::cli::ArgumentParser::Group *global_options) {
    auto redpath_opt = arg_parser->add_new_named_arg("redpath");
    redpath_opt->set_long_name("redpath");
    redpath_opt->set_has_value(true);
    redpath_opt->set_arg_value_help("ABSOLUTE_PATH");
    redpath_opt->set_short_description("redpath path");
    redpath_opt->link_value(&redpathOpt);
    global_options->register_argument(redpath_opt);

    auto forcenode_opt = arg_parser->add_new_named_arg("forcenode");
    forcenode_opt->set_long_name("forcenode");
    forcenode_opt->set_has_value(false);
    forcenode_opt->set_short_description("force node");
    forcenode_opt->link_value(&forcenode);
    global_options->register_argument(forcenode_opt);

    auto installrootnode_opt = arg_parser->add_new_named_arg("installrootnode");
    forcenode_opt->set_long_name("installrootnode");
    forcenode_opt->set_has_value(true);
    forcenode_opt->set_short_description("install root path for node");
    forcenode_opt->link_value(&installRootNodeOpt);
    global_options->register_argument(installrootnode_opt);

    auto rpmverbosity_opt = arg_parser->add_new_named_arg("rpmverbosity");
    rpmverbosity_opt->set_long_name("rpmverbosity");
    rpmverbosity_opt->set_has_value(true);
    rpmverbosity_opt->set_short_description(fmt::format("rpmverbosity level: 0 to {}", RPMLOG_DEBUG));
    rpmverbosity_opt->link_value(&rpmverbosity);
    global_options->register_argument(rpmverbosity_opt);
}

void RedNode::configure() {

    // if no redpath return
    if (redpathOpt.empty())
        return;

    rpmPushMacro(NULL, "__transaction_redpak", NULL, "%{__plugindir}/redpak.so", RMIL_CMDLINE);

    //set redpath
    redpath = redpathOpt.get_value();
    logger.info(fmt::format("redpath is {}", redpath.string()));

    setenv("NODE_PATH", redpath.c_str(), true);

    //setenv redpakid for afmpkg-installer
    setenv("AFMPKG_REDPAKID", redpath.c_str(), true);


    //installroot: set to redpath
    base.get_config().installroot().set(libdnf::Option::Priority::RUNTIME, redpath);

    //set installrootnode
    installrootnode = installRootNodeOpt.get_value();

    //prepend redpath in repodirs
    std::vector<std::string> reposdir(base.get_config().reposdir().get_value());
    for (auto i = reposdir.begin(); i != reposdir.end(); ++i)
        i->insert(0, redpath);
    base.get_config().reposdir().set(libdnf::Option::Priority::RUNTIME, reposdir);

    // set rpm verbosity
    rpmSetVerbosity(rpmverbosity.get_value());
    logger.info(fmt::format("rpmverbosity is {}", rpmverbosity.get_value()));
}

void RedNode::scanNode() {
    // If we have no redpath or when redpath=/ than we seach for coreos redpack.yaml otherwise search for a rednode
    if (redpath != "/") {
        getConf();
    } else {
        getMain();
    }
}

bool RedNode::hasConf() {
    return isnode;
}

void RedNode::getConf() {
    if (!std::filesystem::exists(redpath)) {
        if (strictmode) {
            throw_error(fmt::format("No file at path={}", redpath.string()));
        } else {
            isnode = false;
            fmt::print("Info: Node ignored [no redpack.yaml] path={} ()", redpath.c_str());
        }
    } else {
        isnode = true;
    }

    node =  std::unique_ptr<redNodeT>(RedNodesScan(redpath.c_str(), 0));
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
    for (redNodeT *ancestor_node=node.get(); ancestor_node != NULL; ancestor_node=ancestor_node->ancestor) {
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

void RedNode::checkdir(const std::string & label, const std::filesystem::path & dirpath, bool create) {
    if (create) {
        std::filesystem::create_directories(dirpath);
    }

    std::filesystem::perms perms = std::filesystem::status(dirpath).permissions();

    // check write permission on dirpath
    if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
        throw_error(fmt::format("{}: No permissions to write on {}", label, dirpath.string()));

}

void RedNode::registerNode(redNodeT * node, libdnf::rpm::PackageSack & package_sack) {
    // make sure node is not disabled
    if (node->status->state !=  RED_STATUS_ENABLE) {
        throw_error(fmt::format("*** Node [{}""] is DISABLED [check/update node] nodepath={}", node->config->headers->alias, node->redpath));
    }

    // Expand node config to fit within redpath
    fmt::print("Append db from {}\n", node->redpath);
    package_sack.append_extra_system_repo(node->redpath);
}

void RedNode::appendFamilyDb(libdnf::rpm::PackageSack & package_sack) {
    if(!node.get())
        throw_error("Need node in apppendFamilyDb!");

    // Scan redpath family nodes from terminal leaf to root node
    redNodeT * ancestor_node = node.get();
    for (ancestor_node=node.get(); ancestor_node; ancestor_node=ancestor_node->ancestor) {
        registerNode(ancestor_node, package_sack);

        //no system node handle rpm system node
        if (!ancestor_node->ancestor) {
            std::string no_system_node_path(std::string(ancestor_node->redpath) + "/..");
            std::string no_system_node_path_var_lib_rpm(no_system_node_path + "/var/lib/rpm");
            if (std::filesystem::exists(no_system_node_path_var_lib_rpm)) {
                fmt::print("Append db from {}\n", no_system_node_path);
                package_sack.append_extra_system_repo(no_system_node_path);
            }
        }
    }

}

void RedNode::get_uuid(char * uuid_str) {
    uuid_t u;
    uuid_generate(u);
    uuid_unparse(u, uuid_str);
}

void RedNode::saveto(bool update, const std::string & var_rednode, std::unique_ptr<redConfigT> & redconfig) {

    std::unique_ptr<const char> confpath_ptr(RedGetDefaultExpand(NULL, NULL, var_rednode.c_str()));
    if (!confpath_ptr.get())
        throw_error(fmt::format("Cannot find path to {} variable", var_rednode));

    std::filesystem::path confpath(installrootnode / std::filesystem::path(confpath_ptr.get()).relative_path());
    auto confdir = confpath.parent_path();
    std::filesystem::create_directories(confdir);


    if (!update && std::filesystem::exists(confpath))
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
    std::filesystem::path tmplpath(tmplname);

    if (tmplname.substr(tmplname.length() - 5).compare(".yaml"))
        tmplpath += ".yaml";

    if (!tmplpath.has_parent_path())
        //tmplpath(fmt::format("{}/{}.yaml", tmpldir.get(), tmplname));
        tmplpath = std::filesystem::path(tmpldir.get()) / tmplpath;

    // check template path exists
    if (!std::filesystem::is_regular_file(tmplpath))
        throw_error(fmt::format("Cannot find {} template file (given opt was {})", tmplpath.string(), tmplname));

    redconfig = std::unique_ptr<redConfigT>(RedLoadConfig(tmplpath.c_str(), 0));

    if (!redconfig)
        throw_error(fmt::format("Issue RedLoadConfig with template={}", tmplname));

}

void RedNode::rednode_template(const std::string & alias, const std::string & tmplname,
                               const std::string & tmpladmin, bool update) {
    char today[100];
    char uuid[UUID_STR_LEN];
    const char *info;

    date(today, sizeof(today));
    get_uuid(uuid);

    setenv("NODE_ALIAS", alias.c_str(), true);
    setenv("NODE_UUID", uuid, true);
    setenv("TODAY", today, true);

    //save config file
    reloadConfig(tmplname, config);

    if(!update) {
        // set new headers
        info = RedNodeStringExpand(NULL, NULL, "Node created by $LOGNAME($HOSTNAME) the $TODAY", NULL, NULL);
        if(config->headers->info) free((char *)config->headers->info);
        config->headers->info = info;

        if(config->headers->name) free((char *)config->headers->name);
        config->headers->name = strdup(uuid);

        if(config->headers->alias) free((char *)config->headers->alias);
        config->headers->alias = strdup(alias.c_str());

    }

    saveto(update, "REDNODE_CONF", config);

    //save admin config file
    reloadConfig(tmpladmin, configadmin);
    saveto(update, "REDNODE_ADMIN", configadmin);
}

unsigned long RedNode::timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void RedNode::rednode_status() {

    char today[100];
    date(today, sizeof(today));

    std::unique_ptr<const char> rednode_status_ptr(RedGetDefaultExpand(NULL, NULL, "REDNODE_STATUS"));
    std::filesystem::path rednode_status_path(installrootnode / std::filesystem::path(rednode_status_ptr.get()).relative_path());

    std::string info = fmt::format("created by red-manager the {}", today);
    // create default node status
    redStatusT status = {
        .state = RED_STATUS_ENABLE,
        .realpath = redpath.c_str(),
        .timestamp = timestamp(),
        .info = info.c_str()
    };

    if (RedSaveStatus(rednode_status_path.c_str(), &status, 0))
        throw_error(fmt::format("Fail to push rednode status at path={}", rednode_status_path.c_str()));
}

void RedNode::createRedNodePath(const std::string & alias, bool update, const std::string & tmplate, const std::string & tmplateadmin) {
    setenv("NODE_PATH", redpath.c_str(), true);

    checkdir("redpath", installrootnode / redpath.relative_path(), true);

    rednode_template(alias, tmplate, tmplateadmin, update);
    rednode_status();
}

void RedNode::createRedNode(const std::string & alias, bool update, const std::string & tmplate, const std::string & tmplateadmin, bool no_system_node) {
    isRedpath(true);

    /* default templates */
    const std::string *tpl = &(tmplate.empty() ? RedNode::tmplDefault : tmplate);
    const std::string *tpladmin = &(tmplateadmin.empty() ? RedNode::tmplAdmin : tmplateadmin);

    /* default alias */
    const std::string &real_alias = alias.empty() ? redpath.filename().string() : alias;

    //check parent exists
    auto parent_path = std::filesystem::path(redpath).parent_path();
    if(!std::filesystem::exists( installrootnode / redpath.parent_path().relative_path() / REDNODE_STATUS)) {
        if(!no_system_node) {
            auto systemnode = RedNode(installrootnode, redpath.parent_path(), base);
            systemnode.createRedNode("system", false, RedNode::tmplSystem, RedNode::tmplSystemAdmin, true);

        } else {
            /* templates are different if not system node (indeed system node is inside first parent node) */
            tpl = &(tmplate.empty() ? RedNode::tmplDefaultNoSystemNode : tmplate);
            tpladmin = &(tmplateadmin.empty() ? RedNode::tmplAdminNoSystemNode : tmplateadmin);
        }
    }

    createRedNodePath(real_alias, update, *tpl, *tpladmin);
}

void RedNode::install(libdnf::rpm::PackageSack & package_sack) {
    isRedpath(true);

    scanNode();
    setPersistDir();
    setGpgCheck();
    setCacheDir();

    checkdir("redpath", redpath, false);
    checkdir("dnf persistdir", base.get_config().persistdir().get_value(), false);
    appendFamilyDb(package_sack);
}

bool RedNode::checkInNodeDataBase(std::string name) {
    bool present = false;
    Pool * pool = pool_create();
    pool_set_rootdir(pool, redpath.c_str());
    Repo * repo = repo_create(pool, "redpak");
    int flagsrpm = REPO_REUSE_REPODATA | RPM_ADD_WITH_HDRID | REPO_USE_ROOTDIR;
    repo_add_rpmdb(repo, nullptr, flagsrpm);
    void *rpmstate;
    Queue q;

    queue_init(&q);
    rpmstate = rpm_state_create(pool, redpath.c_str());
    rpm_installedrpmdbids(rpmstate, "Name", name.c_str(), &q);
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
