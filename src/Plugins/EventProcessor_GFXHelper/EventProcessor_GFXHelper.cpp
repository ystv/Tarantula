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
 *   File Name   : EventProcessor_GFXHelper.cpp
 *   Version     : 1.0
 *   Description : Generate events to add a graphics item and remove it later.
 *
 *****************************************************************************/

#include "EventProcessor_GFXHelper.h"
#include "MouseCatcherCore.h"

/**
 * Default constructor. Reads configuration data.
 */
EventProcessor_GFXHelper::EventProcessor_GFXHelper (PluginConfig config, Hook h) :
    MouseCatcherProcessorPlugin(config, h)
{
    // Read device name from config file
    if (config.m_plugindata_map.count("Device"))
    {
        m_gfxdevice = config.m_plugindata_map["Device"];
    }
    else
    {
        h.gs->L->error(config.m_instance + ERROR_LOC, "No Device set in configuration file. Shutting down");
        m_status = FAILED;
        return;
    }
    
    // Populate information array
    m_processorinfo.data["graphicname"] = "string";
    m_processorinfo.data["hostlayer"] = "int";
    m_processorinfo.data["duration"] = "int";

    m_status = READY;

}

EventProcessor_GFXHelper::~EventProcessor_GFXHelper ()
{
    // TODO Auto-generated destructor stub
}

/**
 * Generate events. Generates an Add event at the start time set, and a Remove event at
 * start time + duration
 */
void EventProcessor_GFXHelper::handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent)
{
    // Handle the slightly broken web UI
    if (originalEvent.m_extradata.count("duration") > 0)
    {
        originalEvent.m_duration = std::stoi(originalEvent.m_extradata["duration"]);
    }

    // Validate
    if (originalEvent.m_extradata.count("hostlayer") == 0)
    {
        m_hook.gs->L->error(m_pluginname + ERROR_LOC, "No hostlayer set for event");
        return;
    }

    // Set some top-level defaults
    resultingEvent.m_channel = originalEvent.m_channel;
    resultingEvent.m_description = originalEvent.m_description;
    resultingEvent.m_eventtype = EVENT_FIXED;
    resultingEvent.m_triggertime = originalEvent.m_triggertime;
    resultingEvent.m_targetdevice = originalEvent.m_targetdevice;
    resultingEvent.m_duration = originalEvent.m_duration;
    resultingEvent.m_action = -1;

    // Generate remove event
    MouseCatcherEvent removeChild;
    removeChild.m_channel = originalEvent.m_channel;
    removeChild.m_description = originalEvent.m_description;
    removeChild.m_eventtype = EVENT_FIXED;
    removeChild.m_targetdevice = m_gfxdevice;
    removeChild.m_duration = 1;

    // Generate Add event
    MouseCatcherEvent addChild = removeChild;
    addChild.m_triggertime = originalEvent.m_triggertime;
    addChild.m_action_name = "Add";
    addChild.m_extradata = originalEvent.m_extradata;

    // Finish Remove event
    removeChild.m_action_name = "Remove";
    removeChild.m_extradata["hostlayer"] = originalEvent.m_extradata["hostlayer"];
    removeChild.m_triggertime = originalEvent.m_triggertime +
            static_cast<int>(originalEvent.m_duration / g_pbaseconfig->getFramerate());

    // Add events and finish
    resultingEvent.m_childevents.push_back(addChild);
    resultingEvent.m_childevents.push_back(removeChild);

}


extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        std::shared_ptr<EventProcessor_GFXHelper> plugtemp = std::make_shared<EventProcessor_GFXHelper>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}
