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
*   File Name   : CrosspointDevice.cpp
*   Version     : 1.0
*   Description : Base device for crosspoints
*
*****************************************************************************/


#include "CrosspointDevice.h"
#include "PluginConfig.h"
#include "PluginConfigLoader.h"
#include "pugixml.hpp"
#include "Misc.h"
#include "PlaylistDB.h"

/**
 * Base device for crosspoints
 *
 * @param config Configuration data for this plugin
 * @param h      A link back to the globalStuff structures
 */
CrosspointDevice::CrosspointDevice (PluginConfig config, Hook h) :
        Device(config, EVENTDEVICE_CROSSPOINT, h)
{
    m_actionlist = &crosspoint_device_action_list;

    m_readConfig(config);
}

CrosspointDevice::~CrosspointDevice ()
{
}

/**
 *  Get a list of available inputs stream names
 *
 * @return vector of available input names
 */
std::vector<CrosspointChannel> CrosspointDevice::getAvailableInputs ()
{
    std::vector<CrosspointChannel> ret;

    for (std::pair<int, std::string> thisinput : m_inputvideotoname)
    {
        ret.push_back(m_channeldata[thisinput.second]);
    }

    return ret;
}

/**
 * Get a list of available out stream names
 *
 * @return vector of available output names
 */
std::vector<CrosspointChannel> CrosspointDevice::getAvailableOutputs ()
{
    std::vector<CrosspointChannel> ret;

    for (std::pair<int, std::string> thisoutput : m_outputvideotoname)
    {
        ret.push_back(m_channeldata[thisoutput.second]);
    }

    return ret;
}

/**
 * Get the stream name based on the current audio port associated with it
 *
 * @param audioport
 * @param direction
 * @return string containing stream name or NULL if not found
 */
std::string CrosspointDevice::getNameFromAudioPort (int audioport,
        crosspoint_direction_t direction)
{
    if (direction == CROSSPOINT_INPUT && m_inputaudiotoname.count(audioport))
    {
        return m_inputaudiotoname[audioport];
    }
    else if (direction == CROSSPOINT_OUTPUT
            && m_outputaudiotoname.count(audioport))
    {
        return m_outputaudiotoname[audioport];
    }
    else
    {
        m_hook.gs->L->error("CrosspointDevice::getNameFromAudioPort",
                "Unable to find audio port number as either CROSSPOINT_INPUT "
                        "or CROSSPOINT_OUTPUT");
        return "";
    }
}

/**
 * Get the stream name based on the current video port associated with it
 *
 * @param videoport
 * @param direction
 * @return string containing stream name or NULL if not found
 */
std::string CrosspointDevice::getNameFromVideoPort (int videoport,
        crosspoint_direction_t direction)
{
    if (direction == CROSSPOINT_INPUT && m_inputvideotoname.count(videoport))
    {
        return m_inputvideotoname[videoport];
    }
    else if (direction == CROSSPOINT_OUTPUT
            && m_outputvideotoname.count(videoport))
    {
        return m_outputvideotoname[videoport];
    }
    else
    {
        m_hook.gs->L->error("CrosspointDevice::getNameFromVideoPort",
                "Unable to find audio port number as either CROSSPOINT_INPUT "
                        "or CROSSPOINT_OUTPUT");
        return "";
    }
}

/**
 * Get the current audio port associated with the stream name
 *
 * @param  name
 * @return the audio port number currently associated with the stream name
 */
int CrosspointDevice::getAudioPortFromName (std::string name)
{
    return m_channeldata[name].audioport;
}

/**
 * Get the current video port associated with the stream name
 *
 * @param  name
 * @return the video port number currently associated with the stream name
 */
int CrosspointDevice::getVideoPortfromName (std::string name)
{
    return m_channeldata[name].videoport;
}

/**
 * Get the direction of a channel.
 *
 * @param  name
 * @return Whether the given channel is input or output
 */
crosspoint_direction_t CrosspointDevice::getDirectionFromName (std::string name)
{
    return m_channeldata[name].direction;
}

/**
 * Get the current output mapping of this crosspoint
 *
 * @return Map of output - input channel names
 */
std::map<std::string, std::string> CrosspointDevice::getStatus ()
{
    return m_connectionmap;
}

/**
 * Parse the XML configuration file for this device
 *
 * @param config Config from XML file
 */
void CrosspointDevice::m_readConfig (PluginConfig config)
{
    m_pluginname = config.m_instance;
    pollperiod = config.m_plugindata_xml.child("PollPeriod").text().as_int(100);

    for (pugi::xml_node stream =
            config.m_plugindata_xml.child("Streams").first_child(); stream;
            stream = stream.next_sibling())
    {
        CrosspointChannel new_channel;
        new_channel.name = stream.attribute("name").as_string();

        if (new_channel.name.empty())
        {
            m_hook.gs->L->error(m_pluginname + " Loader",
                    "Unnamed stream in config file");
            continue;
        }

        new_channel.audioport = stream.child("RouterPortAudio").text().as_int(
                -1);
        new_channel.videoport = stream.child("RouterPortVideo").text().as_int(
                -1);

        std::string direction = stream.attribute("direction").as_string();

        if (!direction.compare("in"))
        {
            new_channel.direction = CROSSPOINT_INPUT;
            m_inputaudiotoname[new_channel.audioport] = new_channel.name;
            m_inputvideotoname[new_channel.videoport] = new_channel.name;
        }
        else
        {
            new_channel.direction = CROSSPOINT_OUTPUT;
            m_outputaudiotoname[new_channel.audioport] = new_channel.name;
            m_outputvideotoname[new_channel.videoport] = new_channel.name;
        }

        m_channeldata[new_channel.name] = new_channel;
    }
}

/**
 * Static function to run an event in the playlist.
 *
 * @param device Pointer to the device the event relates to
 * @param event Pointer to the event to be run
 */
void CrosspointDevice::runDeviceEvent (std::shared_ptr<Device> pdevice, PlaylistEntry& event)
{
    std::shared_ptr<CrosspointDevice> peventdevice =
            std::dynamic_pointer_cast<CrosspointDevice>(pdevice);

    if (event.m_action == CROSSPOINTACTION_SWITCH)
    {
        if (1 == event.m_extras.count("input") && 1 == event.m_extras.count("output"))
        {
            try
            {
                g_logger.info(event.m_device, "Now switching output " + event.m_extras["output"] + " to input"
                        + event.m_extras["input"]);
                peventdevice->switchOP(event.m_extras["output"], event.m_extras["input"]);
            } catch (...)
            {
                g_logger.warn(event.m_device + ERROR_LOC,
                        "An error occurred switching output " + event.m_extras["output"] + " to input "
                                + event.m_extras["input"]);
            }
        }
        else
        {
            g_logger.warn("Crosspoint Device RunDeviceEvent" + ERROR_LOC,
                    "Event " + ConvertType::intToString(event.m_eventid) + " malformed, no output/input specified");
        }
    }
    else
    {
        g_logger.error("Crosspoint Device RunDeviceEvent" + ERROR_LOC,
                "Event " + ConvertType::intToString(event.m_eventid) + " has a non-existent action");

    }
}

