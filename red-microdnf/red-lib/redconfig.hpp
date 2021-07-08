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

#ifndef REDLIB_REDCONFIG_HPP
#define REDLIB_REDCONFIG_HPP

#include <string>
#include <filesystem>
#include <fmt/format.h>

#include <libdnf/base/base.hpp>
#include <libdnf/rpm/package.hpp>
#include <libdnf-cli/argument_parser.hpp>

extern "C" {
#include "redconf.h"
}


namespace microdnf {
	class Context;
}

namespace redlib {

class RedNode
{
public:
	RedNode(microdnf::Context & ctx): ctx(&ctx) {}

	void addOptions(libdnf::cli::ArgumentParser::Command *microdnf);
	void configure();
	void install(libdnf::rpm::PackageSack & package_sack);
	void createRedNode(const std::string & alias, bool create, bool update, const std::string & tmplate);

	bool isRedpath(bool strict = true) const;

private:
	microdnf::Context * ctx;
    libdnf::OptionPath redpathOpt{nullptr};

	const std::string & redpath() const { return redpathOpt.get_value(); }

	bool active{false};
	bool strictmode{false};
	bool isnode{false};
	int verbose{0};

	redNodeT *node{nullptr};
	redConfigT *config{nullptr};

	static void throw_error(std::string msg) { throw std::runtime_error(msg); }

	static void get_uuid(char * uuid_str);
	static std::filesystem::path confpath();
	static void date(char *today, std::size_t size);
	static long unsigned int timestamp();
	static std::string expandTplFile(std::filesystem::path & confpath);

	static void checkdir(const std::string & label, const std::string & dirpath, bool create);

	void scanNode();
	void setPersistDir(libdnf::ConfigMain & dnfconfig);
	void setGpgCheck(libdnf::ConfigMain & dnfconfig);
	void appendFamilyDb(libdnf::rpm::PackageSack & package_sack);

	void getMain();
	void getConf();
	bool hasConf();
	void saveto(bool);
	void rednode_status();
	void rednode_template(const std::string & alias, const std::string & tmplname, bool update);
	void reloadConfig(const std::string & tmplname);

	void registerNode(redNodeT * node, libdnf::rpm::PackageSack & package_sack);
};
}

#endif