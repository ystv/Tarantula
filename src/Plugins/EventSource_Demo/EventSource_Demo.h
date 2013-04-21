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
*   File Name   : EventSource_Demo.h
*   Version     : 1.0
*   Description : Demonstration EventSource to create events for
*                 VideoDevice_Demo
*
*****************************************************************************/


#pragma once

#include <queue>

#include "MouseCatcherCommon.h"
#include "MouseCatcherSourcePlugin.h"
#include "PluginConfig.h"

/**
 * A demo EventSource to insert new events every minute or so
 *
 * @param config Configuration data for the plugin
 * @param h      Pointer back to GlobalStuff structures
 */
class EventSource_Demo: public MouseCatcherSourcePlugin
{
public:
    EventSource_Demo (PluginConfig config, Hook h);
    virtual ~EventSource_Demo ();
    virtual void tick (std::vector<EventAction> *EventQueue);

    void updatePlaylist (std::vector<MouseCatcherEvent>& playlist,
            std::shared_ptr<void> additionaldata);
    void updateDevices (std::map<std::string, std::string>&,
            std::shared_ptr<void> additionaldata);
    void updateDeviceActions (std::string device,
            std::vector<ActionInformation> &actions,
            std::shared_ptr<void> additionaldata);
    void updateEventProcessors (
            std::map<std::string, ProcessorInformation>& processors,
            std::shared_ptr<void> additionaldata);
    void updateFiles (std::string device, std::vector<std::pair<std::string, int>>& files,
            std::shared_ptr<void> additionaldata);
private:
    long int m_polltime;
    int m_pollperiod;
    time_t m_lastupdate;
    bool m_enablesystemdata;

    std::vector<EventAction> m_tempactionqueue;
};

