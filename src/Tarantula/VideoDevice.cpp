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
*   File Name   : VideoDevice.cpp
*   Version     : 1.0
*   Description : Base class for playing video
*
*****************************************************************************/

#include "VideoDevice.h"
#include "PluginConfig.h"
#include "pugixml.hpp"
#include "PlaylistDB.h"
#include "Misc.h"

/**
 * Details about what a Video Device is up to
 */
VideoDeviceStatus::VideoDeviceStatus ()
{
    state = VDS_STOPPED;
    remainingframes = 0;
    filename = "";
}

/**
 * Base class for video device control
 *
 * @param config Configuration data for this plugin
 * @param h      A link back to the GlobalStuff structures
 */
VideoDevice::VideoDevice (PluginConfig config, Hook h) :
        Device(config, EVENTDEVICE_VIDEODEVICE, h)
{
    m_actionlist = &video_device_action_list;

    try
    {
        m_mediaupdatecycles = ConvertType::stringToInt(config.m_plugindata_map.at("MediaUpdateCycles"));
    }
    catch (std::exception& ex)
    {
        h.gs->L->warn(m_pluginname, "MediaUpdateCycles not set, assumming 10");
        m_mediaupdatecycles = 10;
    }
    m_mediaupdateremaining = 10;

}

/**
 * Default destructor
 */
VideoDevice::~VideoDevice()
{

}

/**
 * Load a file and play it immediately
 *
 * @param filename ustring The filename to play
 * @param eventid int The event playing, to fire the correct end callback
 */
void VideoDevice::immediatePlay (std::string filename)
{
    cue(filename);
    play();
}

/**
 * Static function to run an event in the playlist.
 *
 * @param device Pointer to the device the event relates to
 * @param event Pointer to the event to be run
 */
void VideoDevice::runDeviceEvent (std::shared_ptr<Device> pdevice, PlaylistEntry& event)
{
    std::shared_ptr<VideoDevice> peventdevice = std::dynamic_pointer_cast<VideoDevice>(pdevice);

    if (VIDEOACTION_LOAD == event.m_action)
    {
        if (1 == event.m_extras.count("filename"))
        {
            try
            {
                peventdevice->cue(event.m_extras["filename"]);
            } catch (...)
            {
                g_logger.error(event.m_device, "An error occurred cueing file " + event.m_extras["filename"]);
            }
        }
        else
        {
            g_logger.warn("Video Device runDeviceEvent",
                    "Event " + ConvertType::intToString(event.m_eventid) + " malformed, no filename");
        }
    }
    else if (VIDEOACTION_PLAY == event.m_action)
    {
        if (1 == event.m_extras.count("filename"))
        {
            try
            {
                peventdevice->immediatePlay(event.m_extras["filename"]);
            } catch (...)
            {
                g_logger.error(event.m_device,
                        "An error occurred playing file " + event.m_extras["filename"]);
            }
        }
        else
        {
            g_logger.warn("Video Device runDeviceEvent",
                    "Event " + ConvertType::intToString(event.m_eventid) + " malformed, no filename");
        }
    }
    else if (VIDEOACTION_PLAYLOADED == event.m_action)
    {
        try
        {
            peventdevice->play();
        } catch (...)
        {
            g_logger.error(event.m_device, "An error occurred playing");
        }
    }
    else if (VIDEOACTION_STOP == event.m_action)
    {
        try
        {
            peventdevice->stop();
        } catch (...)
        {
            g_logger.error(event.m_device, "An error occurred stopping playback");
        }
    }
    else
    {
        g_logger.error("Video Device runDeviceEvent",
                "Event " + ConvertType::intToString(event.m_eventid) + " has a non-existent action");

    }
}

/**
 * Returns the status of this video device
 * @return VideoDeviceStatus Current device status
 */
VideoDeviceStatus VideoDevice::getStatus ()
{
    return m_videostatus;
}

/**
 * Check if we are scheduled to pull a new status
 * Also checks if we are due to update device files (after a number of status pulls)
 */
void VideoDevice::poll ()
{
    if (m_videostatus.state == VDS_PLAYING)
    {
        m_videostatus.remainingframes--;
        if (m_videostatus.remainingframes < 1)
        {
            m_videostatus.state = VDS_STOPPED;
        }
    }
    if (time(NULL) > m_polltime)
    {
        updateHardwareStatus();

        // Come up with a poll time a few seconds in the future
        m_polltime = time(NULL) + pollperiod;

        // Handle counter for periodic file list update
        if (0 == m_mediaupdateremaining)
        {
            getFiles();
            m_mediaupdateremaining = m_mediaupdatecycles;
        }
        else
        {
            --m_mediaupdateremaining;
        }
    }
}

void VideoDevice::getFileList (std::vector<std::pair<std::string, int>>& filelist)
{
    for (auto file : m_files)
    {
        filelist.push_back(std::make_pair(file.first, file.second.m_duration));
    }
}
