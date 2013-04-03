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
*   File Name   : PluginConfig.h
*   Version     : 1.0
*   Description : Plugin configuration data class definition
*
*****************************************************************************/


#pragma once
#include "pugixml.hpp"
#include <map>

/**
 * Plugin Config class
 *
 * Defines plugin configuration data.
 */
class PluginConfig {
public:
    std::string m_name; //!< Name of plugin
    std::string m_filename; //!< The name of the configuration file
    bool        m_enabled; //!< Whether plugin is enabled
    std::string m_type; //!< Type of plugin
    std::string m_library; //!< Library file containing plugin code
    std::string m_instance; //!< Instance name for plugin
    pugi::xml_node m_plugindata_xml; //!< Pointer to additional plugin config data
    std::map<std::string, std::string> m_plugindata_map; //!< Key value pairs for config data in simple plugins.
};



