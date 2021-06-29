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
#include <map>
#include <filesystem>

#include <libdnf/base/base.hpp>
#include <libdnf/rpm/package.hpp>

#include "reddefaults.hpp"

extern "C" {
#include "redconf.h"
}

namespace redlib {

	class Environment
	{
	public:
		Environment(): defaults(REDDEFAULTS) {}
		std::string expandValue(const char * value);
	protected:
		std::map<std::string, std::string> defaults;
	};

	class ParseNode: public Environment
	{
	public:
		ParseNode(const std::filesystem::path & redpath, bool strictmode, int verbose);

		static void checkdir(const std::string & label, const std::string & dirpath, bool create);

		void scanNode();
		void setPersistDir(libdnf::ConfigMain & dnfconfig);
		void setGpgCheck(libdnf::ConfigMain & dnfconfig);
		void appendFamilyDb(libdnf::rpm::PackageSack & package_sack);
		void createRedNode(const std::string & alias, bool create, bool update, const std::string & tmplate);

	private:
		bool strictmode;
		std::filesystem::path redpath;
		bool isnode;
		redNodeT *node;
		redConfigT *config;
		int verbose;

		static void get_uuid(char * uuid_str);
		static std::filesystem::path confpath();
		static void date(char *today, std::size_t size);
		static long unsigned int timestamp();
		static std::string expandTplFile(std::filesystem::path & confpath);

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