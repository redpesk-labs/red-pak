/*
 Copyright (C) 2021-2025 IoT.bzh Company

 Author: Clément Bénier <clement.benier@iot.bzh>

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

#ifndef REDLIB_REDCONFIG_HPP
#define REDLIB_REDCONFIG_HPP

#include <string>
#include <filesystem>
#include <fmt/format.h>
#include <memory>

#include <libdnf/base/base.hpp>
#include <libdnf/base/transaction.hpp>
#include <libdnf/rpm/package.hpp>
#include <libdnf-cli/argument_parser.hpp>

extern "C" {
#include "redconf.h"
}

namespace redlib {

class RedNode
{
public:
    RedNode(libdnf::cli::ArgumentParser *arg_parser, libdnf::Base &base): arg_parser(arg_parser), base(base) {}

    void addOptions(libdnf::cli::ArgumentParser::Group *global_options);
    void configure();
    void install(libdnf::rpm::PackageSack & package_sack);
    void createRedNode(const std::string & alias, bool update, const std::string & tmplate, const std::string & tmplateadmin, bool no_system_node);
     std::vector<libdnf::base::TransactionPackage> checkTransactionPkgs(libdnf::base::Transaction & transaction);

    bool isRedpath(bool strict = true) const;
    const std::filesystem::path &getRedpath() const { return redpath; }

private:
    /* node members */
    bool strictmode{false};
    bool isnode{false};
    int verbose{0};
    std::filesystem::path installrootnode{"/"};
    std::filesystem::path redpath;

    /* dnf objects */
    libdnf::cli::ArgumentParser *arg_parser{nullptr};
    libdnf::Base &base;
    libdnf::LogRouter & logger{*base.get_logger()};

    /* parser options */
    libdnf::OptionPath redpathOpt{nullptr};
    libdnf::OptionPath installRootNodeOpt{"/"};
    libdnf::OptionBool forcenode{false};
    libdnf::OptionNumber<std::int32_t> rpmverbosity{2};

    /* redconf pointers */
    std::unique_ptr<redNodeT> node{nullptr};
    std::unique_ptr<redConfigT> config{nullptr};
    std::unique_ptr<redConfigT> configadmin{nullptr};

    static void throw_error(std::string msg) { throw std::runtime_error(msg); }

    static std::filesystem::path confpath();
    static void checkdir(const std::string & label, const std::filesystem::path & dirpath, bool create);

    void scanNode();
    void setPersistDir();
    void setGpgCheck();
    void setCacheDir();
    void appendFamilyDb(libdnf::rpm::PackageSack & package_sack);

    void getMain();
    void getConf();
    bool hasConf();
    bool checkInNodeDataBase(std::string name);

    void registerNode(redNodeT * node, libdnf::rpm::PackageSack & package_sack);

#if !LEGACY_REDCONFIG
    /* default template names */
    inline static const std::string tmplDefaultNoSystemNode = "full-normal";
    inline static const std::string tmplAdminNoSystemNode = "full-admin";
    inline static const std::string tmplSystem = "root-normal";
    inline static const std::string tmplSystemAdmin = "root-admin";
    inline static const std::string tmplDefault = "leaf-normal";
    inline static const std::string tmplAdmin = "leaf-admin";

    static void get_uuid(char * uuid_str);
    void saveto(bool update, const std::string & var_rednode, std::unique_ptr<redConfigT> & redconfig);
    static void date(char *today, std::size_t size);
    void reloadConfig(const std::string & tmplname, std::unique_ptr<redConfigT> & redconfig);
    void rednode_template(const std::string & alias, const std::string & tmplname,
                          const std::string & tmpladmin, bool update);
    static long unsigned int timestamp();
    void rednode_status();
    void createRedNodePath(const std::string & alias, bool update,
                           const std::string & tmplate, const std::string & tmplateadmin);
    RedNode(std::filesystem::path installrootnode, std::filesystem::path redpath, libdnf::Base &base):
        installrootnode(installrootnode), redpath(redpath), base(base) {}
#endif

};
}

#endif
