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
*   File Name   : BaseConfigLoader.h
*   Version     : 1.0
*   Description : BaseConfigLoader.h - loads the main Tarantula configs
*
*****************************************************************************/


#pragma once
#include "pugixml.hpp"
#include <vector>

/**
 * Required fields for Channel constructor
 */
class ChannelDetails
{
public:
    std::string m_channame;
    std::string m_xpname;
    std::string m_xpport;
};

/**
 * Loads the main Tarantula configuration file
 * Not to be confused with parent class ConfigBase
 */
class BaseConfigLoader
{
public:
    BaseConfigLoader ();
    BaseConfigLoader (std::string filename);
    void LoadConfig (std::string filename);

    float getFramerate ();

    int getPluginReloadTime (int reloadsremain);
    int getPluginReloadCount ();

    std::string getSystemName ();
    std::string getDevicesPath ();
    std::string getInterfacesPath ();
    std::string getLogsPath ();
    std::string getEventSourcesPath ();
    std::string getEventProcessorsPath ();
    std::string getOfflineDatabasePath ();
    int getDatabaseSyncTime ();

    int getMCDeletedEventCount ();

    std::vector<ChannelDetails> getLoadedChannels ();

private:
    float m_framerate;
    std::string m_systemname;
    std::string m_devicespath;
    std::string m_interfacespath;
    std::string m_logspath;
    std::string m_eventsourcepath;
    std::string m_eventprocessorspath;
    std::string m_offlinedata_path;
    int m_db_synctime;
    std::vector<ChannelDetails> m_loadedchannels;

    int m_mcdeletedvents;

    std::vector<int> m_pluginreloadpoints;

    void setDefaults (); //needs to be called in different places depending on constructor

    // Storage for loaded XML data
    pugi::xml_document m_configdata;

};
