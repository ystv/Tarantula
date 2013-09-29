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
 *   File Name   : EventProcessorShow.cpp
 *   Version     : 1.0
 *   Description : 
 *
 *****************************************************************************/

#include "EventProcessor_Show.h"
#include "MouseCatcherCore.h"
#include "VideoDevice.h"
#include "Misc.h"

/**
 * Constructor. Reads configuration data
 *
 * @param config Plugin configuration from file
 * @param h      Link back to GlobalStuff structures
 */
EventProcessor_Show::EventProcessor_Show (PluginConfig config, Hook h) :
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
        m_videodevice = config.m_plugindata_map.at("VideoDevice");
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname, "Configuration data invalid or not supplied");
        m_status = FAILED;
        return;
    }

    // Populate information array
    m_processorinfo.data["filename"] = "string";
    m_processorinfo.data["duration"] = "int";

    m_status = READY;
}

/**
 * Default destructor.
 */
EventProcessor_Show::~EventProcessor_Show ()
{

}

/**
 * Generate a set of child events to run a show
 *
 * @param originalEvent  Template event containing a file name and description for the show to play
 * @param resultingEvent Generated event with child events to run aspects of the show
 */
void EventProcessor_Show::handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent)
{
    // Handle the slightly broken web UI
    if (originalEvent.m_extradata.count("duration") > 0)
    {
        originalEvent.m_duration = ConvertType::stringToInt(originalEvent.m_extradata["duration"]);
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
    if (0 == m_hook.gs->Devices->count(m_videodevice) ||
            0 == g_mcprocessors.count(m_continuitygenerator) ||
            (m_enableoverlay && 0 == m_hook.gs->Devices->count(m_cgdevice)))
    {
        m_hook.gs->L->error(m_pluginname, "One of the required devices (" + m_videodevice + ", " + m_cgdevice + ", " +
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

    // Generate the video event
    MouseCatcherEvent videoevent = tempevent;
    videoevent.m_triggertime = originalEvent.m_triggertime +
            static_cast<int>(m_continuitylength / g_pbaseconfig->getFramerate());
    videoevent.m_targetdevice = m_videodevice;
    videoevent.m_action_name = "Play";

    if (originalEvent.m_extradata.count("filename") > 0)
    {
        videoevent.m_extradata["filename"] = originalEvent.m_extradata["filename"];
    }
    else
    {
        m_hook.gs->L->error(m_pluginname, "Video filename not set" );
        return;
    }

    videoevent.m_duration = originalEvent.m_duration;
    resultingEvent.m_childevents.push_back(videoevent);


    // Generate a set of CG events for overlays
    if (m_enableoverlay && videoevent.m_duration > m_nownextminimum)
    {
        int videoseconds = static_cast<int>(videoevent.m_duration / g_pbaseconfig->getFramerate());
        MouseCatcherEvent cgparent = fillevent;
        cgparent.m_targetdevice = m_cgdevice;
        cgparent.m_action_name = "Parent";
        cgparent.m_duration = 1;
        cgparent.m_extradata["hostlayer"] = ConvertType::intToString(m_nownextlayer);

        int runningtrigtime;
        if (videoevent.m_duration > m_nownextminimum && videoevent.m_duration < (m_nownextperiod * 1.25))
        {
            runningtrigtime = videoevent.m_triggertime + videoseconds / 2;
        }
        else
        {
            runningtrigtime = videoevent.m_triggertime +
                    static_cast<int>(m_nownextperiod / g_pbaseconfig->getFramerate());
        }


        while (runningtrigtime < (videoevent.m_triggertime + videoseconds))
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
            resultingEvent.m_childevents.push_back(cgparent);
            cgparent.m_childevents.clear();
        }
    }


}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        std::shared_ptr<EventProcessor_Show> plugtemp = std::make_shared<EventProcessor_Show>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}


