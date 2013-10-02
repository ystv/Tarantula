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
*   File Name   : Channel.cpp
*   Version     : 1.0
*   Description : Class for running channels and all event handling
*
*****************************************************************************/


#include "Channel.h"
#include "CrosspointDevice.h"
#include "VideoDevice.h"
#include "CGDevice.h"
#include "Misc.h"
#include "Log.h"
#include "MouseCatcherCommon.h"
#include "MouseCatcherCore.h"

//this is here because if it were in core header, it would create a loop.
extern std::vector<std::shared_ptr<Channel>> g_channels;

int end = 0;

/**
 * Constructor when a name isn't explicitly specified
 */
Channel::Channel () : m_pl("Unknown")
{
    m_channame = "Unnamed Channel";
    m_xpport = "YSTV Stream";
    m_xpdevicename = "DemoXpointDefaultName";
    //try and get a more sensible default XP name
    for (std::pair<std::string, std::shared_ptr<Device>> currentdevice : g_devices)
    {
        if (currentdevice.second->getType() == EVENTDEVICE_CROSSPOINT)
        {
            m_xpdevicename = currentdevice.first;
        }
    }
    init();
}

/**
 * Constructor specifying some more channel details
 *
 * @param name   The name of the channel to initialize
 * @param xpname The name of the crosspoint for this channel
 * @param xport  The name of this channel's crosspoint port (as in crosspoint device file)
 */
Channel::Channel (std::string name, std::string xpname, std::string xport) : m_pl(name)
{
    m_channame = name;
    m_xpdevicename = xpname;
    m_xpport = xport;
    init();
}

Channel::~Channel ()
{

}

/**
 * Check that this channel's crosspoint details exist, called after device loading
 */
void Channel::init ()
{
    // Confirm this crosspoint device actually exists
    if (g_devices.find(m_xpdevicename) == g_devices.end())
    {
        g_logger.error("Channel " + m_channame,
                "Crosspoint device " + m_xpdevicename + " does not exist!!");

        throw (std::exception());
    }

    // And check the port is real too
    if (std::shared_ptr<CrosspointDevice> cpd = std::static_pointer_cast<CrosspointDevice> (g_devices[m_xpdevicename]))
    {
        if (cpd->getVideoPortfromName(m_xpport) < 0)
        {
            g_logger.error("Channel " + m_channame,
                    "Crosspoint port " + m_xpport + " does not exist!!");

            throw (std::exception());
        }
    }

    // Disable manual hold
    m_hold_event = -1;

    // Register the preprocessor
    g_preprocessorlist.emplace("Channel::manualHoldRelease", &Channel::manualHoldRelease);
}

/**
 * Queries events to take place this frame and executes them
 */
void Channel::tick ()
{
    // Update hold flag
    m_hold_event = m_pl.getActiveHold(time(NULL));

    //Pull all the time triggered events at the current time
    std::vector<PlaylistEntry> events = m_pl.getEvents(EVENT_FIXED, (time(NULL)));

    //Execute events on devices
    for (PlaylistEntry thisevent : events)
    {
        // Only run events if the channel is not in hold, or the event is a child of the hold
        if (0 == m_hold_event || thisevent.m_parent == m_hold_event)
        {
            runEvent(thisevent);
        }
        else
        {
            g_logger.info(m_channame + " Runner", std::string("Event ") + std::to_string(thisevent.m_eventid) +
                    std::string(" ignored due to active hold ") + std::to_string(m_hold_event));
        }
    }

    // Sync database
    if (m_sync_counter > g_pbaseconfig->getDatabaseSyncTime())
    {
        m_sync_counter = 0;

        g_async.newAsyncJob(
                std::bind(&Channel::periodicDatabaseSync, this, std::placeholders::_1, std::placeholders::_2), NULL,
                        NULL, 50, false);
    }
    else
    {
        m_sync_counter++;
    }
}

/**
 * Trigger a manual event and release hold on channel
 *
 * @param id Event ID to trigger
 */
void Channel::manualTrigger (int id)
{
    if (id == m_hold_event)
    {
        m_hold_event = 0;

        PlaylistEntry event;
        m_pl.getEventDetails(id, event);

        // Run the callback function
        if (!event.m_preprocessor.empty())
        {
            if (g_preprocessorlist.count(event.m_preprocessor) > 0)
            {
                g_preprocessorlist[event.m_preprocessor](event, this);
            }
            else
            {
                g_logger.warn("Channel Runner" + ERROR_LOC, "Ignoring invalid PreProcessor " + event.m_preprocessor);
            }
        }

        m_pl.processEvent(id);
    }
    else
    {
        g_logger.warn(m_channame + ERROR_LOC, "Got a manual trigger for an inactive hold, ignoring");
    }
}

/**
 * Processes child events from an event which has started
 *
 *  @param name The name of the device firing the callback
 *  @param id   The id of the event which has started playing
 */
void Channel::begunPlaying (std::string name, int id)
{
    std::vector<PlaylistEntry> childevents = m_pl.getChildEvents(id);
    // Run the child events
    for (PlaylistEntry thisevent : childevents)
    {
        runEvent(thisevent);
    }
}

/**
 * Doesn't really do anything since relative events were scrapped
 *
 *  @param name  The name of the device firing the callback
 *  @param id    The id of the event which has stopped playing
 */
void Channel::endPlaying (std::string name, int id)
{

}

/**
 * Runs a specified event.
 *
 * @param event The event to run
 */
void Channel::runEvent (PlaylistEntry& event)
{
    // Run the callback function
    if (!event.m_preprocessor.empty())
    {
        if (g_preprocessorlist.count(event.m_preprocessor) > 0)
        {
            g_preprocessorlist[event.m_preprocessor](event, this);
        }
        else
        {
            g_logger.warn("Channel Runner" + ERROR_LOC, "Ignoring invalid PreProcessor " + event.m_preprocessor);
        }
    }

    if ((0 == g_devices.count(event.m_device)) && (event.m_devicetype != EVENTDEVICE_PROCESSOR))
    {
        g_logger.warn("Channel Runner",
                "Device " + event.m_device + " not found for event ID " + ConvertType::intToString(event.m_eventid));

        // Mark event as processed
        m_pl.processEvent(event.m_eventid);

        return;
    }

    switch (event.m_devicetype)
    {
        case EVENTDEVICE_CROSSPOINT:
        {
            CrosspointDevice::runDeviceEvent(g_devices[event.m_device], event);
            break;
        }
        case EVENTDEVICE_VIDEODEVICE:
        {
            VideoDevice::runDeviceEvent(g_devices[event.m_device], event);
            break;
        }
        case EVENTDEVICE_CGDEVICE:
        {
            CGDevice::runDeviceEvent(g_devices[event.m_device], event);
            break;
        }
        case EVENTDEVICE_PROCESSOR:
            break;
    }

    // Marks event as processed
    m_pl.processEvent(event.m_eventid);
}

int Channel::createEvent (PlaylistEntry *pev)
{
    int ret = m_pl.addEvent(pev);
    return ret;
}

/**
 * Lock the core and sync the database in memory with the one on disk
 *
 * @param data Unused
 */
void Channel::periodicDatabaseSync (std::shared_ptr<void> data, std::timed_mutex &core_lock)
{
    m_pl.writeToDisk(g_pbaseconfig->getOfflineDatabasePath(), m_channame, core_lock);
}

/**
 *  Because you cannot add member function pointers to the callbacks, here is a workaround:
 *  We go through and call the tick function of each Channel class in the host's stack.
 */
void channelTick ()
{
    for (std::shared_ptr<Channel> pthischannel : g_channels)
    {
        pthischannel->tick();
    }
}

/**
 *  Because you cannot add member function pointers to the callbacks, here is a workaround:
 *  We go through and call the BegunPlaying function of each Channel class in the host's stack.
 *
 *  @param name string The name of the device firing the callback
 *  @param id int The id of the event which has started playing
 */
void channelBegunPlaying (std::string name, int id)
{
    for (std::shared_ptr<Channel> pthischannel : g_channels)
    {
        pthischannel->begunPlaying(name, id);
    }
}

/**
 *  Because you cannot add member function pointers to the callbacks, here is a workaround:
 *  We go through and call the EndPlaying function of each Channel class in the host's stack.
 *
 *  @param name string The name of the device firing the callback
 *  @param id int The id of the event which has started playing
 */
void channelEndPlaying (std::string name, int id)
{
    for (std::shared_ptr<Channel> pthischannel : g_channels)
    {
        pthischannel->endPlaying(name, id);
    }
}

/**
 * Loop over the channel list and return the index of a channel from its name.
 *
 * @param channelname Name of the channel to find
 *
 * @return Index of the located channel
 */
int Channel::getChannelByName (std::string channelname)
{
    for (std::vector<std::shared_ptr<Channel>>::iterator it = g_channels.begin();
            it != g_channels.end(); ++it)
    {
        if (*it)
        {
            if (!channelname.compare((*it)->m_channame))
            {
                return it - g_channels.begin();
            }
        }
    }

    // At this point no channel was found with that name
    throw std::exception();
    return -1;
}

/**
 * Callback function for a LiveShow EP manual trigger. Unfortunately it has to go here as the plugin can't access
 * the SQL database directly.
 */
void Channel::manualHoldRelease (PlaylistEntry &event, Channel *pchannel)
{
    // Erase any remaining children of this event
    std::vector<PlaylistEntry> children = pchannel->m_pl.getChildEvents(event.m_eventid);

    for (PlaylistEntry thischild : children)
    {
        pchannel->m_pl.removeEvent(thischild.m_eventid);
    }

    // Perform the shunt
    time_t starttime = event.m_trigger + static_cast<int>(event.m_duration / g_pbaseconfig->getFramerate());
    int length = time(NULL) - starttime;
    pchannel->m_pl.shunt(starttime, length);

    // Add xpoint switch

    MouseCatcherEvent xpswitch;
    xpswitch.m_action_name = "Switch";
    xpswitch.m_channel = pchannel->m_channame;
    xpswitch.m_duration = 1;
    xpswitch.m_eventtype = EVENT_FIXED;
    xpswitch.m_targetdevice = pchannel->m_xpdevicename;
    xpswitch.m_triggertime = time(NULL);
    xpswitch.m_extradata["output"] = pchannel->m_xpport;
    xpswitch.m_extradata["input"] = event.m_extras["switchchannel"];

    EventAction action;
    MouseCatcherCore::processEvent(xpswitch, event.m_parent, true, action);

}
