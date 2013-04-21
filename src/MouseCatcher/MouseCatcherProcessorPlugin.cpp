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
*   File Name   : MouseCatcherProcessorPlugin.cpp
*   Version     : 1.0
*   Description : Base class for an EventProcessor
*
*****************************************************************************/


#include "MouseCatcherProcessorPlugin.h"
#include "PluginConfig.h"

extern std::map<std::string, MouseCatcherProcessorPlugin*> g_mcprocessors;

/**
 * Load a Processor plugin and set the base config up
 *
 * @param config Configuration for the plugin
 * @param h      Link back to the GlobalStuff structures
 */
MouseCatcherProcessorPlugin::MouseCatcherProcessorPlugin (PluginConfig config, Hook h) :
        Plugin(config, h)
{
    m_processorinfo.name = config.m_name;

    if (config.m_plugindata_map.count("Description"))
    {
        m_processorinfo.description = config.m_plugindata_map["description"];
    }
    else
    {
        h.gs->L->info("EventProcessor Loader",
                "No description set for " + config.m_instance);
    }

    g_mcprocessors[m_pluginname] = this;
}

/**
 * Destructor
 */
MouseCatcherProcessorPlugin::~MouseCatcherProcessorPlugin ()
{

}

/**
 * Get the ProcessorInformation struct for this plugin
 *
 * @return Struct containing plugin name, description and data
 */
ProcessorInformation MouseCatcherProcessorPlugin::getProcessorInformation ()
{
    return m_processorinfo;
}

