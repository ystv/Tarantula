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
 *   File Name   : EventProcessor_LiveShow.cpp
 *   Version     : 1.0
 *   Description : Generate events to support a live show
 *
 *****************************************************************************/

#include <functional>

#include "EventProcessor_LiveShow.h"
#include "MouseCatcherCore.h"

EventProcessor_LiveShow::EventProcessor_LiveShow (PluginConfig config, Hook h) :
    MouseCatcherProcessorPlugin(config, h)
{
    // Read configuration data
    try
    {
        if (!config.m_plugindata_map.at("EnableOverlay").compare("true"))
        {
            m_cgdevice = config.m_plugindata_map.at("CGDevice");
            m_nownextname = config.m_plugindata_map.at("NowNextName");
            m_nownextminimum = ConvertType::stringToInt(config.m_plugindata_map.at("NowNextMin"));
            m_nownextperiod = ConvertType::stringToInt(config.m_plugindata_map.at("NowNextPeriod"));
            m_nownextduration = ConvertType::stringToInt(config.m_plugindata_map.at("NowNextDuration"));
            m_nownextlayer = ConvertType::stringToInt(config.m_plugindata_map.at("NowNextHostLayer"));
            m_enableoverlay = true;
        }
        else
        {
            m_enableoverlay = false;
        }

        m_continuitygenerator = config.m_plugindata_map.at("FillProcessorName");
        m_continuitylength = ConvertType::stringToInt(config.m_plugindata_map.at("FillLength"));


        m_crosspointdevice = config.m_plugindata_map.at("CrosspointDevice");
        m_liveinput = config.m_plugindata_map.at("LiveInput");
        m_videoinput = config.m_plugindata_map.at("DefaultInput");
        m_xpoutput = config.m_plugindata_map.at("Output");

        m_vtdevice = config.m_plugindata_map.at("VTDevice");
        m_vtfile = config.m_plugindata_map.at("VTClockFilename");
        m_vtduration = std::stoi(config.m_plugindata_map.at("VTClockDuration"));
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname, "Configuration data invalid or not supplied");
        m_status = FAILED;
        return;
    }

    // Populate information array
    m_processorinfo.data["duration"] = "int";


    m_status = READY;
    
}

EventProcessor_LiveShow::~EventProcessor_LiveShow ()
{
}

void EventProcessor_LiveShow::handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent)
{
    // Handle the slightly broken web UI
    if (originalEvent.m_extradata.count("duration") > 0)
    {
        originalEvent.m_duration = std::stoi(originalEvent.m_extradata["duration"]);
    }

    // Set some top-level defaults
    resultingEvent.m_channel = originalEvent.m_channel;
    resultingEvent.m_description = originalEvent.m_description;
    resultingEvent.m_eventtype = EVENT_FIXED;
    resultingEvent.m_triggertime = originalEvent.m_triggertime;
    resultingEvent.m_targetdevice = originalEvent.m_targetdevice;
    resultingEvent.m_duration = originalEvent.m_duration + m_continuitylength;
    resultingEvent.m_action = -1;

    // Run some checks
    if (0 == m_hook.gs->Devices->count(m_crosspointdevice) ||
            0 == g_mcprocessors.count(m_continuitygenerator) ||
            (m_enableoverlay && 0 == m_hook.gs->Devices->count(m_cgdevice)))
    {
        m_hook.gs->L->error(m_pluginname, "One of the required devices (" + m_crosspointdevice + ", " + m_cgdevice + ", " +
                m_continuitygenerator + ") is not available.");
        return;
    }

    // Template event for rest of children
    MouseCatcherEvent tempevent;
    tempevent.m_channel = originalEvent.m_channel;
    tempevent.m_description = originalEvent.m_description;
    tempevent.m_eventtype = EVENT_FIXED;

    // Generate the fill event
    MouseCatcherEvent fillevent = tempevent;
    fillevent.m_triggertime = originalEvent.m_triggertime;
    fillevent.m_targetdevice = m_continuitygenerator;
    fillevent.m_duration = m_continuitylength;
    resultingEvent.m_childevents.push_back(fillevent);

    // Generate manual hold event
    MouseCatcherEvent holdevent = tempevent;
    holdevent.m_triggertime = originalEvent.m_triggertime +
            static_cast<int>(m_continuitylength / g_pbaseconfig->getFramerate());
    holdevent.m_eventtype = EVENT_MANUAL;
    holdevent.m_duration = originalEvent.m_duration;
    holdevent.m_preprocessor = "Channel::manualHoldRelease";
    holdevent.m_extradata["switchchannel"] = m_videoinput;

    // Generate VT clock event
    MouseCatcherEvent clockevent = tempevent;
    clockevent.m_triggertime = holdevent.m_triggertime - m_vtduration;
    clockevent.m_targetdevice = m_vtdevice;
    clockevent.m_extradata["filename"] = m_vtfile;
    clockevent.m_action_name = "Play";
    clockevent.m_duration = m_vtduration * g_pbaseconfig->getFramerate();
    resultingEvent.m_childevents.push_back(clockevent);

    // Generate crosspoint event
    MouseCatcherEvent xpevent = tempevent;
    xpevent.m_triggertime = holdevent.m_triggertime;
    xpevent.m_targetdevice = m_crosspointdevice;
    xpevent.m_extradata["output"] = m_xpoutput;
    xpevent.m_extradata["input"] = m_liveinput;
    xpevent.m_action_name = "Switch";
    xpevent.m_duration = 1;
    // Child of the hold to ensure it runs.
    holdevent.m_childevents.push_back(xpevent);

    // Generate a set of CG events for overlays
    if (m_enableoverlay && holdevent.m_duration > m_nownextminimum)
    {
        int showseconds = static_cast<int>(holdevent.m_duration / g_pbaseconfig->getFramerate());
        MouseCatcherEvent cgparent = fillevent;
        cgparent.m_targetdevice = m_cgdevice;
        cgparent.m_action_name = "Parent";
        cgparent.m_duration = 1;
        cgparent.m_extradata["hostlayer"] = ConvertType::intToString(m_nownextlayer);

        int runningtrigtime;
        if (holdevent.m_duration > m_nownextminimum && holdevent.m_duration < (m_nownextperiod * 1.25))
        {
            runningtrigtime = holdevent.m_triggertime + showseconds / 2;
        }
        else
        {
            runningtrigtime = holdevent.m_triggertime +
                    static_cast<int>(m_nownextperiod / g_pbaseconfig->getFramerate());
        }


        while (runningtrigtime < (holdevent.m_triggertime + showseconds))
        {
            // Generate progressive CG events
            cgparent.m_triggertime = runningtrigtime;
            MouseCatcherEvent cgchild = cgparent;
            cgchild.m_action_name = "Add";
            cgchild.m_extradata["graphicname"] = m_nownextname;
            if (!originalEvent.m_description.empty())
            {
                cgchild.m_extradata["nowtext"] = originalEvent.m_description;
            }
            cgchild.m_extradata["nexttext"] = "ppfill"; //Checked if not blank and filled by PP

            // Hijack a preprocessor from EP_Fill
            cgchild.m_preprocessor = "EventProcessor_Fill::populateCGNowNext";
            cgparent.m_childevents.push_back(cgchild);

            cgchild.m_action_name = "Remove";
            cgchild.m_triggertime += static_cast<int>(m_nownextduration / g_pbaseconfig->getFramerate());
            cgparent.m_childevents.push_back(cgchild);

            runningtrigtime += static_cast<int>(m_nownextperiod / g_pbaseconfig->getFramerate());

            cgparent.m_duration = (runningtrigtime - cgparent.m_triggertime) * 25 - m_nownextperiod + 1;

            // These must be children of the hold event to be played while the hold is active
            holdevent.m_childevents.push_back(cgparent);
            cgparent.m_childevents.clear();
        }
    }

    resultingEvent.m_childevents.push_back(holdevent);
}

