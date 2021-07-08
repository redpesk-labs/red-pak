/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of microdnf: https://github.com/rpm-software-management/libdnf/

Microdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Microdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with microdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "manager.hpp"

#include "../../context.hpp"

#include "libdnf-cli/output/transaction_table.hpp"

#include <libdnf/base/goal.hpp>
#include <libdnf/conf/option_string.hpp>
#include <libdnf/rpm/package.hpp>
#include <libdnf/rpm/package_query.hpp>
#include <libdnf/rpm/package_set.hpp>
#include <libdnf/rpm/transaction.hpp>

#include <filesystem>
#include <iostream>

#include "redconfig.hpp"

namespace microdnf {

using namespace libdnf::cli;

void CmdManager::set_argument_parser(Context & ctx) {

    auto arg_alias= ctx.arg_parser.add_new_named_arg("alias");
    arg_alias->set_has_value(true);
    arg_alias->set_long_name("alias");
    arg_alias->set_short_description("rednode alias name [mandatory when creating");
    arg_alias->link_value(&alias);

    auto arg_update = ctx.arg_parser.add_new_named_arg("update");
    arg_update->set_has_value(false);
    arg_update->set_long_name("update");
    arg_update->set_short_description("force creation even when node exist");
    arg_update->set_const_value("true");
    arg_update->link_value(&update);

    auto arg_create = ctx.arg_parser.add_new_named_arg("create");
    arg_create->set_has_value(false);
    arg_create->set_long_name("create");
    arg_create->set_short_description("Create an empty node [require --alias]");
    arg_create->set_const_value("true");
    arg_create->link_value(&create);

    auto arg_tmplate = ctx.arg_parser.add_new_named_arg("template");
    arg_tmplate->set_has_value(true);
    arg_tmplate->set_long_name("template");
    arg_tmplate->set_short_description("Create node config from temmplate [default= /etc/redpak/template.d/default.yaml]");
    arg_tmplate->link_value(&tmplate);

    auto manager = ctx.arg_parser.add_new_command("manager");
    manager->set_short_description("Create/Delete redpak rootpath leaves");
    manager->set_description("");
    manager->set_named_args_help_header("Optional arguments:");
    manager->set_positional_args_help_header("Positional arguments:");
    manager->set_parse_hook_func([this, &ctx](
                                     [[maybe_unused]] ArgumentParser::Argument * arg,
                                     [[maybe_unused]] const char * option,
                                     [[maybe_unused]] int argc,
                                     [[maybe_unused]] const char * const argv[]) {
        ctx.select_command(this);
        return true;
    });

    manager->register_named_arg(arg_alias);
    manager->register_named_arg(arg_create);
    manager->register_named_arg(arg_update);
    manager->register_named_arg(arg_tmplate);

    ctx.arg_parser.get_root_command()->register_command(manager);
}

void CmdManager::configure([[maybe_unused]] Context & ctx) {
    bool nodeconfig = false;

    if (update.get_value()) {
        create.set(libdnf::Option::Priority::RUNTIME, true);
    }

    if (create.get_value()) {
        nodeconfig = true;
        if (alias.get_value().empty())
            throw std::runtime_error("--create require --alias=xxxx [no default]");

        if (tmplate.get_value().empty())
            create.set(libdnf::Option::Priority::RUNTIME, "default");

    }

    // check we redpath is defined and exist
    if (ctx.rednode.isRedpath(false))
        throw std::runtime_error("Syntax Error: redpak --redpath=/xx/../xyz subcommand (missing --redpath)");
}

void CmdManager::run(Context & ctx) {
    if (create.get_value()) {
        ctx.rednode.createRedNode(alias.get_value(), create.get_value(), update.get_value(), tmplate.get_value());
    }

    //create system repo
    auto & package_sack = *ctx.base.get_rpm_package_sack();
    package_sack.create_system_repo(false);

    fmt::print(fmt::format("SUCCESS: rednode ready at {}", ctx.get_redpath().get_value()));
}

}  // namespace microdnf
