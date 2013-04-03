/******************************************************************************
*   Copyright (C) 2011 - 2013  York Student Television
*
*   Tarantula is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   Tarantula is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Tarantula.  If not, see <http://www.gnu.org/licenses/>.
*
*   Contact     : tarantula@ystv.co.uk
*
*   File Name   : PluginConfigLoader.h
*   Version     : 1.0
*   Description : Generic configuration loader for plugins
*
*****************************************************************************/


#pragma once
#include "pugixml.hpp"
#include "Log.h"
#include "PluginConfig.h"

// Macro to set extension of library files
#ifdef _WIN32
#define LIBRARY_EXTENSION ".dll"
#else
#define LIBRARY_EXTENSION ".so"
#endif

/**
 * Plugin configuration parsing error values.
 */
enum plugin_config_error_t
{
    SUCCESS = 0,          //!< Config loading succeeds
    FILE_NOT_FOUND,       //!< The requested configuration file was not found
    INCORRECT_CONFIG_TYPE,   //!< The config file referred to a different plugin
    INVALID_CONFIG,       //!< The config file was wrong.
    PARSE_ERROR,          //!< The XML in the config file was invalid.
    INCORRECT_TYPE,       //!< The plugin type did not match the type requested.
    UNKNOWN_ERROR         //!< Something got upset somewhere.
};

// Helper function to load plugins
void loadAllPlugins (std::string path, std::string type);

/**
 * Basic plugin configuration loader. Will parse the required configuration
 * (name, shared library name, whether plugin enabled) along with a block of
 * plugin configuration data
 */
class PluginConfigLoader
{
public:
    PluginConfigLoader ();
    virtual ~PluginConfigLoader ();
    plugin_config_error_t loadConfig (std::string filename, std::string type);
    PluginConfig getConfig ();
private:
    // The expected plugin type which should appear in the config file
    std::string m_plugintype;

    // Temporary storage for loaded XML data
    pugi::xml_document m_configdata;

    // Storage for the configuration loaded from XML
    PluginConfig m_config;

};
