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
*   File Name   : PluginConfigLoader.cpp
*   Version     : 1.0
*   Description : Generic configuration loader for plugins
*
*****************************************************************************/


#include <dirent.h> // For reading the directories
#include "PluginConfigLoader.h"
#include "pugixml.hpp" // XML parser library
#include "TarantulaPluginLoader.h"
#include "TarantulaCore.h"
#include "TarantulaPlugin.h"

/**
 * Constructor
 *
 * Sets some defaults.
 */
PluginConfigLoader::PluginConfigLoader ()
{
    m_config.m_enabled = false;
    m_config.m_name = "UNLOADED CONFIGURATION";
    m_config.m_library = "";
}

/**
 * Destructor
 *
 * Does nothing
 */
PluginConfigLoader::~PluginConfigLoader ()
{

}

/**
 * Load configuration file and attempt to parse plugin configuration
 *
 * @param filename XML configuration file to load.
 */
plugin_config_error_t PluginConfigLoader::loadConfig (std::string filename,
        std::string type)
{
    m_plugintype = type;
    m_config.m_filename = filename;

    // Attempt to load the file
    pugi::xml_parse_result result = m_configdata.load_file(filename.c_str());

    // Return an error if parsing fails
    if (pugi::status_file_not_found == result.status)
    {
        return FILE_NOT_FOUND;
    }
    else if (pugi::status_out_of_memory == result.status
            || pugi::status_internal_error == result.status)
    {
        return UNKNOWN_ERROR;
    }
    else if (pugi::status_ok != result.status)
    {
        // A little hacky but saves a massive if statement for all errors.
        return PARSE_ERROR;
    }

    // Grab the PluginLoad node (if it exists)
    pugi::xml_node pluginload = m_configdata.document_element().child("PluginLoad");
    if (pluginload.empty())
    {
        return INVALID_CONFIG;
    }

    // Check the device type matches the expected type
    if (m_plugintype.compare(pluginload.child_value("Type")))
    {
        return INCORRECT_TYPE;
    }
    else
    {
        m_config.m_type = m_plugintype;
    }

    // Check if the plugin is enabled
    m_config.m_enabled = pluginload.child("Enabled").text().as_bool();

    // Get a name for the plugin
    m_config.m_name = pluginload.child_value("Name");
    if (m_config.m_name.empty())
    {
        return INVALID_CONFIG;
    }

    // Get a library file for the plugin
    m_config.m_library = pluginload.child_value("Library");
    if (m_config.m_library.empty())
    {
        return INVALID_CONFIG;
    }
    else
    {
        // Tack on the right extension.
        m_config.m_library += LIBRARY_EXTENSION;
    }

    // Get an instance name for the plugin
    m_config.m_instance = pluginload.child_value("Instance");
    if (m_config.m_instance.empty())
    {
        return INVALID_CONFIG;
    }

    // Get the data block for this plugin
    m_config.m_plugindata_xml = m_configdata.document_element().child("PluginData");

    // Work out if we want to parse strings
    std::string mode = m_config.m_plugindata_xml.attribute("mode").as_string();
    if (!mode.empty())
    {
        // Loop over the children of PluginData and drop into the map
        for (pugi::xml_node child = m_config.m_plugindata_xml.first_child(); child;
                child = child.next_sibling())
        {
            // Check whether this value exists: spec says it must be unique so fail
            if (m_config.m_plugindata_map.find(child.name())
                    != m_config.m_plugindata_map.end())
            {
                return INVALID_CONFIG;
            }
            m_config.m_plugindata_map.insert(
                    std::make_pair(child.name(), child.child_value()));

        }
    }

    // If we got here all went well
    return SUCCESS;
}

/**
 * Returns the PluginConfig
 *
 * @return The configuration loaded by the loader
 */
PluginConfig PluginConfigLoader::getConfig ()
{
    return m_config;
}

/**
 * Loads all the plugins in the directory specified
 *
 * @param path     ustring Path to config files
 */
void loadAllPlugins (std::string path, std::string type)
{
    DIR *dp;
    dirent *de;
    if ((dp = opendir(path.c_str())))
    {
        while ((de = readdir(dp)))
        {
            if (de->d_type != DT_DIR)
            {
                PluginConfigLoader plugin_config;
                plugin_config.loadConfig(path + "/" + de->d_name, type);

                PluginStateData state;

                state.type = type;
                state.reloadsremaining = g_pbaseconfig->getPluginReloadCount();
                state.ppluginreference = NULL;

                if (plugin_config.getConfig().m_enabled)
                {
                    std::shared_ptr<Plugin> newplugin = ActivatePlugin(plugin_config.getConfig(), state.ppluginreference);
                }

                if (NULL != state.ppluginreference)
                {
                    g_plugins.push_back(state);
                }
            }
        }
        closedir(dp);
    }
    else
    {
        g_logger.warn("MouseCatcher Event Processor Loader",
                "Cannot load directory " + path);
    }
}
