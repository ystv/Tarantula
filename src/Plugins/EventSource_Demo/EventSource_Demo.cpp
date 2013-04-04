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
*   File Name   : EventSource_Demo.cpp
*   Version     : 1.0
*   Description : Demonstration EventSource to create events for
*                 VideoDevice_Demo
*
*****************************************************************************/


#include <queue>
#include <sstream>

#include "EventSource_Demo.h"
#include "MouseCatcherCore.h"
#include "Misc.h"

/**
 * Not much to do here, just set a poll period to guarantee tick() is called immediately
 *
 * @param config Configuration data for this plugin
 * @param h      Pointer back to GlobalStuff structures
 */
EventSource_Demo::EventSource_Demo (PluginConfig config, Hook h) :
        MouseCatcherSourcePlugin(config, h)
{
    m_polltime = 0;
    m_lastupdate = 0;
    m_pollperiod = ConvertType::stringToInt(config.m_plugindata_map["PollPeriod"]);

    if (!config.m_plugindata_map["GetSystemData"].compare("true"))
    {
        m_enablesystemdata = true;
    }
    else
    {
        m_enablesystemdata = false;
    }

    m_status = READY;
}

EventSource_Demo::~EventSource_Demo ()
{
}

/**
 * If a pollperiod has passed since we last did this, throw some more events in
 */
void EventSource_Demo::tick (std::vector<EventAction> *peventqueue)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for tick");
        return;
    }

    //Add events generated in the callbacks to the queue
    peventqueue->insert(peventqueue->end(), m_tempactionqueue.begin(),
            m_tempactionqueue.end());
    m_tempactionqueue.clear();

    if (time(NULL) > m_polltime)
    {
        m_hook.gs->L->info("EventSource_Demo", "Generating fresh events...");
        MouseCatcherEvent mev;
        mev.m_channel = "Campus Stream";
        mev.m_targetdevice = "Demo Event Processor 1";
        mev.m_eventtype = EVENT_FIXED;
        mev.m_triggertime = time(NULL) + 15;
        mev.m_duration = 0;

        EventAction action;

        action.action = ACTION_ADD;
        action.event = (mev);
        action.thisplugin = this;
        action.isprocessed = false;

        peventqueue->push_back(action);

        m_polltime = time(NULL) + m_pollperiod;

        EventAction action2;

        action2.action = ACTION_UPDATE_PLAYLIST;
        action2.event.m_triggertime = m_lastupdate;
        action2.thisplugin = this;
        action2.event.m_channel = "Campus Stream";
        action2.isprocessed = false;

        m_lastupdate = time(NULL);

        peventqueue->push_back(action2);

        if (m_enablesystemdata)
        {
            EventAction systemdata_action;
            systemdata_action.action = ACTION_UPDATE_DEVICES;
            systemdata_action.thisplugin = this;
            peventqueue->push_back(systemdata_action);

            systemdata_action.action = ACTION_UPDATE_PROCESSORS;
            peventqueue->push_back(systemdata_action);
        }
    }
}

void EventSource_Demo::updateDevices (
        std::map<std::string, std::string>& devicelist,
        std::shared_ptr<void> padditionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDevices");
        return;
    }

    std::stringstream datadump;
    datadump << "Got Devices:" << std::endl;
    for (auto device : devicelist)
    {
        datadump << "\t" << device.first << " of type " << device.second
                << std::endl;

        EventAction typeaction;
        typeaction.action = ACTION_UPDATE_ACTIONS;
        typeaction.event.m_channel = device.first;
        typeaction.thisplugin = this;
        m_tempactionqueue.push_back(typeaction);
    }
    m_hook.gs->L->info(m_pluginname, datadump.str());
}

void EventSource_Demo::updateDeviceActions (std::string device,
        std::vector<ActionInformation>& actions,
        std::shared_ptr<void> padditionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDeviceActions");
        return;
    }

    std::stringstream datadump;
    datadump << "Got actions for device " << device << std::endl;
    for (ActionInformation action : actions)
    {
        datadump << "\t" << action.actionid << ": " << action.name << " - "
                << action.description << std::endl;
    }
    m_hook.gs->L->info(m_pluginname, datadump.str());
}

void EventSource_Demo::updateEventProcessors (
        std::map<std::string, ProcessorInformation>& processors,
        std::shared_ptr<void> padditionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateEventProcessors");
        return;
    }

    std::stringstream datadump;
    datadump << "Got Processors:" << std::endl;
    for (std::pair<std::string, ProcessorInformation> processor : processors)
    {
        datadump << "\t" << processor.first << " (" << processor.second.name
                << ") - " << processor.second.description << std::endl;
    }
    m_hook.gs->L->info(m_pluginname, datadump.str());
}

void EventSource_Demo::updatePlaylist (std::vector<MouseCatcherEvent>& playlist,
        std::shared_ptr<void> padditionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updatePlaylist");
        return;
    }

    for (std::vector<MouseCatcherEvent>::iterator it = playlist.begin();
            it != playlist.end(); ++it)
    {
        m_hook.gs->L->info("EventSource_Demo",
                "Got event for " + it->m_targetdevice);
    }
}

void EventSource_Demo::updateFiles (std::string device,
        std::vector<std::string>& files, std::shared_ptr<void> padditionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateFiles");
        return;
    }

    std::stringstream datadump;
    datadump << "Got files for " << device << ": ";
    for (std::string file : files)
    {
        datadump << file << ", ";
    }
    m_hook.gs->L->info(m_pluginname, datadump.str());
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new EventSource_Demo(config, h);
    }
}

