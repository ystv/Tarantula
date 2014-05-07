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
*   File Name   : BaseConfigLoader.cpp
*   Version     : 1.0
*   Description : /
*
*****************************************************************************/

#include "BaseConfigLoader.h"
#include "TarantulaCore.h"
#include "pugixml.hpp"
#include "Misc.h"
#include "Log.h"

#include <vector>

/**
 * Loads the main Tarantula configuration file
 * Not to be confused with parent class ConfigBase
 */
BaseConfigLoader::BaseConfigLoader ()
{
}

/**
 * Can't call the parent constructor with variable because the virtuals
 * won't be resolved properly!
 *
 * @param filename ustring The config file to load
 */
BaseConfigLoader::BaseConfigLoader (std::string filename)
{
    LoadConfig(filename);
}

/**
 * Parse the configuration file
 *
 * @param filename XML configuration file to parse
 */
void BaseConfigLoader::LoadConfig (std::string filename)
{
    // Attempt to load the file
    pugi::xml_parse_result result = m_configdata.load_file(filename.c_str());

    // Return an error if parsing fails
    if (pugi::status_file_not_found == result.status)
    {
        throw std::exception();
    }
    else if (pugi::status_out_of_memory == result.status
            || pugi::status_internal_error == result.status)
    {
        throw std::exception();
    }
    else if (pugi::status_ok != result.status)
    {
        throw std::exception();
    }

    // Grab the Config node and get some paths
    pugi::xml_node confignode = m_configdata.document_element().child("Config");
    if (confignode.empty())
    {
        g_logger.warn("Base Config Loader", "No Config node in config file");
    }
    else
    {
        m_devicespath = confignode.child_value("Devices");
        if (m_devicespath.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No Devices path in config file. Assuming \"Devices\"");
            m_devicespath = "Devices";
        }

        m_interfacespath = confignode.child_value("Interfaces");
        if (m_interfacespath.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No Interfaces path in config file. Assuming \"Interfaces\"");
            m_interfacespath = "Interfaces";
        }

        m_logspath = confignode.child_value("Logs");
        if (m_logspath.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No Logs path in config file. Assuming \"Logs\"");
            m_logspath = "Logs";
        }

        m_eventsourcepath = confignode.child_value("EventSources");
        if (m_eventsourcepath.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No EventSources path in config file. Assuming \"EventSources\"");
            m_eventsourcepath = "EventSources";
        }

        m_eventprocessorspath = confignode.child_value("EventProcessors");
        if (m_eventprocessorspath.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No EventProcessors path in config file. Assuming \"EventProcessors\"");
            m_eventprocessorspath = "EventProcessors";
        }
    }

    // Grab the System node and get name and framerate
    pugi::xml_node systemnode = m_configdata.document_element().child("System");
    if (systemnode.empty())
    {
        g_logger.warn("Base Config Loader", "No System node in config file");
    }
    else
    {
        m_systemname = systemnode.child_value("Name");
        if (m_systemname.empty())
        {
            g_logger.warn("Base Config Loader",
                    "No SystemName in config file. Assuming \"Default System\"");
            m_systemname = "Default System";
        }

        m_framerate = systemnode.child("Framerate").text().as_float(-1);
        if (-1 == m_framerate)
        {
            g_logger.warn("Base Config Loader",
                    "No Framerate in config file. Using 25 FPS");
            m_framerate = 25;
        }

        m_database_path = systemnode.child_value("Database");

        if (m_database_path.empty())
        {
            g_logger.OMGWTF("Base Config Loader", "No Database path in config file. This will be a problem");
            m_database_path = "temp.db";
        }
    }

    // Grab the Plugins node and work out what the reload times are
    pugi::xml_node pluginsnode = m_configdata.document_element().child("Plugins");
    if (pluginsnode.empty())
    {
        g_logger.error("Base Config Loader", "No Plugins node in config file");
        throw std::exception();
    }
    else
    {
        for (auto it : pluginsnode.child("ReloadTimes").children())
        {
            m_pluginreloadpoints.push_back(it.text().as_int(10));
        }
    }

    // Grab the Channels node and load channels
    pugi::xml_node channelsnode = m_configdata.document_element().child("Channels");
    if (channelsnode.empty())
    {
        g_logger.error("Base Config Loader", "No Channels node in config file");
        throw std::exception();
    }
    else
    {
        for (auto it : channelsnode.children())
        {
            ChannelDetails thischannel;
            thischannel.m_channame = it.child_value("Name");
            thischannel.m_xpname = it.child_value("CrosspointName");
            thischannel.m_xpport = it.child_value("CrosspointPort");

            if (thischannel.m_channame.empty() || thischannel.m_xpname.empty() ||
                    thischannel.m_xpport.empty())
            {
                g_logger.error("Base Config Loader", "Invalid channel");
                throw std::exception();
            }

            m_loadedchannels.push_back(thischannel);
        }

    }
}

float BaseConfigLoader::getFramerate ()
{
    return m_framerate;
}

/**
 * Return the reload time for a certain reload index
 *
 * @param reloadsremain Number of reloads plugin has left
 * @return              Time in frames before plugin restart, or zero if no reloads left
 */
int BaseConfigLoader::getPluginReloadTime (int reloadsremain)
{
    unsigned int index = m_pluginreloadpoints.size() - reloadsremain;

    if (index < m_pluginreloadpoints.size())
    {
        return m_pluginreloadpoints[index];
    }
    else
    {
        return 0;
    }
}

/**
 * Get the number of times to reload a crashed plugin before unloading it
 *
 * @return Number of times to reload
 */
int BaseConfigLoader::getPluginReloadCount (void)
{
    return m_pluginreloadpoints.size();
}

std::string BaseConfigLoader::getSystemName ()
{
    return m_systemname;
}

std::string BaseConfigLoader::getDevicesPath ()
{
    return m_devicespath;
}

std::string BaseConfigLoader::getInterfacesPath ()
{
    return m_interfacespath;
}

std::string BaseConfigLoader::getLogsPath ()
{
    return m_logspath;
}

std::string BaseConfigLoader::getEventSourcesPath ()
{
    return m_eventsourcepath;
}

std::string BaseConfigLoader::getEventProcessorsPath ()
{
    return m_eventprocessorspath;
}

std::vector<ChannelDetails> BaseConfigLoader::getLoadedChannels ()
{
    return m_loadedchannels;
}

std::string BaseConfigLoader::getDatabasePath ()
{
    return m_database_path;
}

/**
 * Get the number of deleted events to cache before updating EventSources
 *
 * @return Number of events
 */
int BaseConfigLoader::getMCDeletedEventCount (void)
{
    return m_mcdeletedvents;
}

