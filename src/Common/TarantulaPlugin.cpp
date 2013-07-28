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
*   File Name   : TarantulaPlugin.cpp
*   Version     : 1.0
*   Description : Implements the base class for all plugins.
*
*****************************************************************************/


#include "TarantulaPlugin.h"

/**
 * Constructor. Configure some basics common to every plugin.
 *
 * @param config Configuration data array
 * @param h      Link back to global constructs
 */
Plugin::Plugin (PluginConfig config, Hook h)
{
    m_status = STARTING;
    m_pluginname = config.m_instance;
    m_hook = h;
    m_configfile = config.m_filename;
}

Plugin::~Plugin()
{

}

/**
 * Placeholder for callback to add a plugin to a management list
 *
 * @param thisplugin Reference to access new plugin
 */
void Plugin::addPluginReference (std::shared_ptr<Plugin> thisplugin)
{
    // Nothing to do by default
}

/**
 * Return the name of the plugin instance
 *
 * @return String of plugin instance name
 */
std::string Plugin::getPluginName ()
{
    return m_pluginname;
}

/**
 * Return current internal status of the plugin
 *
 * @return Status of plugin
 */
plugin_status_t Plugin::getStatus ()
{
    return m_status;
}

std::string Plugin::getConfigFilename ()
{
    return m_configfile;
}

/**
 * Shut down the plugin by marking it as ready to unload.
 */
void Plugin::disablePlugin ()
{
    m_status = UNLOAD;
}
