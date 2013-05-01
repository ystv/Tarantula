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
*   File Name   : MouseCatcherSourcePlugin.h
*   Version     : 1.0
*   Description : Base class for an EventSource
*
*****************************************************************************/


#pragma once

#include <TarantulaPlugin.h>

#include "PluginConfig.h"
#include "MouseCatcherProcessorPlugin.h"

/**
 * Base class for an EventSource Plugin
 */
class MouseCatcherSourcePlugin: public Plugin
{
public:
    MouseCatcherSourcePlugin (PluginConfig config, Hook h);
    virtual ~MouseCatcherSourcePlugin ();
    virtual void tick (std::vector<EventAction> *ActionQueue)=0;

    // Callbacks used to update plugin from the core
    virtual void updatePlaylist (std::vector<MouseCatcherEvent>& playlist,
            std::shared_ptr<void> additionaldata)=0;
    virtual void updateDevices (std::map<std::string, std::string>&,
            std::shared_ptr<void> additionaldata)=0;
    virtual void updateDeviceActions (std::string device,
            std::vector<ActionInformation> &actions,
            std::shared_ptr<void> additionaldata)=0;
    virtual void updateEventProcessors (
            std::map<std::string, ProcessorInformation>& processors,
            std::shared_ptr<void> additionaldata)=0;
    virtual void updateFiles (std::string device,
    		std::vector<std::pair<std::string, int>>& files,
            std::shared_ptr<void> additionaldata)=0;

    static bool actionCompleteCheck (const EventAction &a,
    		const MouseCatcherSourcePlugin *plugin);
protected:
    void getEvents (int channelid, time_t since,
            std::vector<MouseCatcherEvent>* eventvector);
    void addEvent (MouseCatcherEvent event);
    bool deleteEvent (int eventid);
    bool editEvent (int eventid, MouseCatcherEvent event);

};

