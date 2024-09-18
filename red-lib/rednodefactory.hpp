/*
 Copyright 2021 IoT.bzh

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

#ifndef REDLIB_REDNODEFACTORY_HPP
#define REDLIB_REDNODEFACTORY_HPP

#include <string>
#include <filesystem>
#include <memory>

extern "C" {
#include "rednode-factory.h"
}

namespace redlib {

/**
* @brief tiny class wrapping calls to rednode-factory C module
*/
class RedNodeFactory
{
public:

    using Status = rednode_factory_error_t;

    /// default builder
    RedNodeFactory() noexcept;

    /// clear or reset current instance
    void clear() noexcept;

    /// get current status, true if no error, false if error reported
    operator bool() const noexcept { return status_ == RednodeFactory_OK; };

    /// get current status
    Status status() const noexcept { return status_; };

    /// get string of the current status
    const char *statusText() const noexcept { return rednode_factory_error_text(status_); };

    /// set the root directory
    bool setRootDir(const char *rootdir, size_t length) noexcept;
    bool setRootDir(const char *rootdir) noexcept;
    bool setRootDir(const std::string &rootdir) noexcept;
    bool setRootDir(const std::filesystem::path &rootdir) noexcept;

    /// set the node directory
    bool setNodeDir(const char *nodedir, size_t length) noexcept;
    bool setNodeDir(const char *nodedir) noexcept;
    bool setNodeDir(const std::string &nodedir) noexcept;
    bool setNodeDir(const std::filesystem::path &nodedir) noexcept;

    /// create the node
    bool createRedNode(bool no_system_node = true) noexcept;
    bool createRedNode(const char *alias, const char *tmplate = NULL, const char *tmplateadmin = NULL, bool no_system_node = true) noexcept;
    bool createRedNode(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node = true) noexcept;

    /// update the node
    bool updateRedNode(bool no_system_node = true) noexcept;
    bool updateRedNode(const char *alias, const char *tmplate = NULL, const char *tmplateadmin = NULL, bool no_system_node = true) noexcept;
    bool updateRedNode(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node = true) noexcept;

    /// create or update the node
    bool process(const char *alias = NULL, const char *tmplate = NULL, const char *tmplateadmin = NULL, bool no_system_node = true, bool update = false) noexcept;
    bool process(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node = true, bool update = false) noexcept;

private:

    /// set the return status
    bool x_(int status) noexcept;

    /// last status
    Status status_ {RednodeFactory_OK};

    /// factory object
    rednode_factory_t factory_;
};

inline
RedNodeFactory::RedNodeFactory() noexcept
{
    clear();
}

inline
void RedNodeFactory::clear() noexcept
{
    rednode_factory_clear(&factory_);
    status_ = RednodeFactory_OK;
}

inline
bool RedNodeFactory::x_(int status) noexcept
{
    status_ = (Status)-status;
    return status_ == RednodeFactory_OK;
}

inline
bool RedNodeFactory::setRootDir(const char *rootdir, size_t length) noexcept
{
    return x_(rednode_factory_set_root_len(&factory_, rootdir, length));
}

inline
bool RedNodeFactory::setRootDir(const char *rootdir) noexcept
{
    return x_(rednode_factory_set_root(&factory_, rootdir));
}

inline
bool RedNodeFactory::setRootDir(const std::string &rootdir) noexcept
{
    return setRootDir(rootdir.c_str(), rootdir.length());
}

inline
bool RedNodeFactory::setRootDir(const std::filesystem::path &rootdir) noexcept
{
    return setRootDir(rootdir.c_str());
}

inline
bool RedNodeFactory::setNodeDir(const char *nodedir, size_t length) noexcept
{
    return x_(rednode_factory_set_node_len(&factory_, nodedir, length));
}

inline
bool RedNodeFactory::setNodeDir(const char *nodedir) noexcept
{
    return x_(rednode_factory_set_node(&factory_, nodedir));
}

inline
bool RedNodeFactory::setNodeDir(const std::string &nodedir) noexcept
{
    return setNodeDir(nodedir.c_str(), nodedir.length());
}

inline
bool RedNodeFactory::setNodeDir(const std::filesystem::path &nodedir) noexcept
{
    return setNodeDir(nodedir.c_str());
}

inline
bool RedNodeFactory::createRedNode(bool no_system_node) noexcept
{
    return x_(rednode_factory_create_node(&factory_, NULL, false, !no_system_node));
}

inline
bool RedNodeFactory::createRedNode(const char *alias, const char *tmplate, const char *tmplateadmin, bool no_system_node) noexcept
{
    return process(alias, tmplate, tmplateadmin, no_system_node, false);
}

inline
bool RedNodeFactory::createRedNode(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node) noexcept
{
    return createRedNode(alias.c_str(), tmplate.c_str(), tmplateadmin.c_str(), no_system_node);
}

inline
bool RedNodeFactory::updateRedNode(bool no_system_node) noexcept
{
    return x_(rednode_factory_create_node(&factory_, NULL, true, !no_system_node));
}

inline
bool RedNodeFactory::updateRedNode(const char *alias, const char *tmplate, const char *tmplateadmin, bool no_system_node) noexcept
{
    return process(alias, tmplate, tmplateadmin, no_system_node, true);
}

inline
bool RedNodeFactory::updateRedNode(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node) noexcept
{
    return updateRedNode(alias.c_str(), tmplate.c_str(), tmplateadmin.c_str(), no_system_node);
}

inline
bool RedNodeFactory::process(const std::string &alias, const std::string &tmplate, const std::string & tmplateadmin, bool no_system_node, bool update) noexcept
{
    return process(alias.c_str(), tmplate.c_str(), tmplateadmin.c_str(), no_system_node, update);
}

inline
bool RedNodeFactory::process(const char *alias, const char *tmplate, const char *tmplateadmin, bool no_system_node, bool update) noexcept
{
    rednode_factory_param_t params;
    params.alias = alias;
    params.normal = tmplate;
    params.admin = tmplateadmin;
    params.templatedir = NULL;
    return x_(rednode_factory_create_node(&factory_, &params, update, !no_system_node));
}

}

#endif //REDLIB_REDNODEFACTORY_HPP
