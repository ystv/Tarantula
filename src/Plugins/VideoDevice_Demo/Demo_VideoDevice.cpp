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
*   File Name   : Demo_VideoDevice.cpp
*   Version     : 1.0
*   Description : Demonstration of a VideoDevice
*
*****************************************************************************/


#include "Demo_VideoDevice.h"
#include "Misc.h"
#include "PluginConfig.h"

/**
 * A demo video device
 *
 * @param config Configuration data for this plugin
 * @param h      A link back to the GlobalStuff structures
 */
VideoDevice_Demo::VideoDevice_Demo (PluginConfig config, Hook h) :
        VideoDevice(config, h)
{
    // Most of this is handled by parent class
    m_clipready = false;
    m_status = READY;

    getFiles();
}

VideoDevice_Demo::~VideoDevice_Demo ()
{
}

/**
 * Prepares device to play clip, provided name was valid, otherwise silently fails
 * @param clip Name of a clip to play
 */
void VideoDevice_Demo::cue (std::string clip)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for cue");
    }

    // Do nothing unless we got a valid clip
    if (clip == VDDEMO_CLIPNAME)
    {
        m_videostatus.filename = clip;
        m_clipready = true;
    }
}

/**
 * Set flags for playing clip if one is ready
 * @param eventid The event that this clip corresponds to
 */
void VideoDevice_Demo::play ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for play");
    }

    if (m_clipready)
    {
        m_videostatus.state = VDS_PLAYING;
        m_videostatus.remainingframes = VDDEMO_CLIPFRAMES;

        m_hook.gs->L->info("Demo Media", "Now playing " + m_videostatus.filename);

    }
}

/**
 * Create a list of sample video files
 */
void VideoDevice_Demo::getFiles ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for getFiles");
    }

    VideoFile samplefile;
    samplefile.m_filename = VDDEMO_CLIPNAME;
    samplefile.m_title = VDDEMO_CLIPNAME;
    samplefile.m_duration = VDDEMO_CLIPFRAMES;
    m_files[samplefile.m_filename] = samplefile;
}

/**
 * Reset flags to stop device
 */
void VideoDevice_Demo::stop ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for stop");
    }

    m_videostatus.state = VDS_STOPPED;
    m_videostatus.remainingframes = 0;
}

void VideoDevice_Demo::updateHardwareStatus ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateHardwareStatus");
    }

    //Just log that we got the request
    std::string logmessage =
            "Got a request to updateHardwareStatus. I am currently ";
    if (m_videostatus.state == VDS_PLAYING)
        logmessage.append(
                "playing with "
                        + ConvertType::intToString(m_videostatus.remainingframes)
                        + " frames remaining.");
    else
        logmessage.append("stopped");

    m_hook.gs->L->info("Demo Media", logmessage);
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<VideoDevice_Demo> plugtemp = std::make_shared<VideoDevice_Demo>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}

