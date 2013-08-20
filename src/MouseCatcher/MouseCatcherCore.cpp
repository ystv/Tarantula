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
*   File Name   : MouseCatcherCore.cpp
*   Version     : 1.0
*   Description : The central functions for MouseCatcher
*
*****************************************************************************/


#include <dirent.h> //for reading the directories
#include <vector>
#include <algorithm>

#include "MouseCatcherCommon.h"
#include "MouseCatcherCore.h"
#include "TarantulaPlugin.h"
#include "TarantulaPluginLoader.h"
#include "PluginConfigLoader.h"
#include "PlaylistDB.h"
#include "Misc.h"
#include "VideoDevice.h"
#include "CGDevice.h"

extern std::vector<std::shared_ptr<MouseCatcherSourcePlugin>> g_mcsources;
extern std::map<std::string,
    std::shared_ptr<MouseCatcherProcessorPlugin>> g_mcprocessors;

namespace MouseCatcherCore
{
    std::vector<EventAction> *g_pactionqueue;

    /**
     * Encapsulates loading plugins, registering callbacks and logging activation
     *
     * @param sourcepath    Path to EventSource plugins
     * @param processorpath Path to EventProcessor plugins
     */
    void init (std::string sourcepath, std::string processorpath)
    {
        g_pactionqueue = new std::vector<EventAction>;
        g_logger.info("MouseCatcherCore", "Now initialising MouseCatcher core");
        loadAllPlugins(sourcepath, "EventSource");
        loadAllPlugins(processorpath, "EventProcessor");
        g_tickcallbacks.push_back(MouseCatcherCore::eventSourcePluginTicks);
        g_tickcallbacks.push_back(MouseCatcherCore::eventQueueTicks);
    }

    /**
     * Calls tick() handlers for all the EventProcessor plugins
     */
    void eventSourcePluginTicks ()
    {
        for (std::shared_ptr<MouseCatcherSourcePlugin> thisplugin : g_mcsources)
        {
            if (thisplugin)
            {
                thisplugin->tick(g_pactionqueue);
            }
        }

        // Delete all plugins that are unloaded
        g_mcsources.erase(std::remove_if(g_mcsources.begin(), g_mcsources.end(),
                        [](std::shared_ptr<MouseCatcherSourcePlugin> d){return d->getStatus() == UNLOAD;}),
                        g_mcsources.end());
    }

    /**
     * Process an individual incoming event
     *
     * @param thisevent MouseCatcherEvent
     * @param lastid int The id of the last inserted event, -1 for none
     * @param ischild bool
     * @return int The id of the inserted event for relative tails
     */
    int processEvent (MouseCatcherEvent event, int lastid, bool ischild,
            EventAction& action)
    {
        // Cursory check to make sure the given channel is actually real
        int channelid;
        try
        {
            channelid = Channel::getChannelByName(event.m_channel);
        }
        catch (std::exception&)
        {
            g_logger.warn("MouseCatcherCore", "Got event for bad channel: " + event.m_channel);
            action.returnmessage = "Channel " + event.m_channel + " not found";
            return -1;
        }

        // Process event through EventProcessors, then run this function on new events
        if (0 == g_devices.count(event.m_targetdevice))
        {
            g_logger.info("MouseCatcherCore", "Scanning Event Processors");

            // Run an EventProcessor
            if (1 == g_mcprocessors.count(event.m_targetdevice))
            {
                // Create a variable to hold original event
                MouseCatcherEvent originalevent = event;

                // Action fields do not apply to processors, set it to -1
                originalevent.m_action = -1;

                g_mcprocessors[event.m_targetdevice]->handleEvent(originalevent, event);
            }
            else
            {
                g_logger.warn("MouseCatcherCore", "Got event for bad device: " + event.m_targetdevice);

                // Return failure code
                action.returnmessage = "Device/Processor " + event.m_targetdevice + " not found!";
                return -1;
            }
        }
        else
        {
            // Check that we got a working event chain
            if ((lastid < 0) && (event.m_eventtype != EVENT_FIXED))
            {
                g_logger.warn("MouseCatcherCore", "An invalid event chain was detected");
                return -1;
            }
        }

        // Create and add a playlist event for the parent
        PlaylistEntry playlistevent;
        convertToPlaylistEvent(&event, lastid, &playlistevent);

        int eventid = g_channels[channelid]->createEvent(&playlistevent);

        // Loop over and handle children
        for (MouseCatcherEvent thischild : event.m_childevents)
        {
            processEvent(thischild, eventid, true, action);
        }

        return eventid;

    }

    /**
     * Gets all events the system knows about within a range of time
     *
     * @param channelid     Channel to fetch events for. -1 for all channels.
     * @param starttime     Only fetch events scheduled after this timestamp.
     * @param length		Length of range to fetch events for
     * @param eventvector   Vector to insert the events into.
     * @return              False if given channel was invalid, otherwise true.
     */
    void getEvents (int channelid, time_t starttime, int length,
            std::vector<MouseCatcherEvent>& eventvector)
    {
        std::vector<PlaylistEntry> playlistevents;
        std::vector<std::shared_ptr<Channel>>::iterator channelstart;
        std::vector<std::shared_ptr<Channel>>::iterator channelend;

        if (channelid != -1)
        {
            channelstart = g_channels.begin() + channelid;
            channelend = channelstart + 1;
        }
        else
        {
            channelstart = g_channels.begin();
            channelend = g_channels.end();
        }

        for (std::vector<std::shared_ptr<Channel>>::iterator it = channelstart;
                it != channelend; ++it)
        {
            playlistevents = (*it)->m_pl.getEventList(starttime, length);
            for (std::vector<PlaylistEntry>::iterator it2 =
                    playlistevents.begin(); it2 != playlistevents.end(); ++it2)
            {
                std::vector<PlaylistEntry> playlistchildren = g_channels.at(
                        it - channelstart)->m_pl.getChildEvents(it2->m_eventid);

                MouseCatcherEvent tempevent;
                MouseCatcherCore::convertToMCEvent(it2.base(), *it,
                        &tempevent, &g_logger);
                eventvector.push_back(tempevent);
            }
        }

    }

    /**
     * Remove an event from the playlist
     *
     * @param action EventAction containing data for this event
     */
    void removeEvent (EventAction& action)
    {
        int channelid;

        try
        {
            channelid = Channel::getChannelByName(action.event.m_channel);
        } catch (std::exception&)
        {
            action.returnmessage =
                    "Attempted to delete an event from a nonexistent channel";
            g_logger.warn("Event Queue", action.returnmessage);
            return;
        }

        g_channels.at(channelid)->m_pl.removeEvent(action.eventid);

    }

    /**
     * Erase a playlist event and create a new one in its place
     * @param action EventAction containing data for this event
     */
    void editEvent (EventAction& action)
    {
        removeEvent(action);

        if (!action.returnmessage.compare("Deleted successfully"))
        {
            processEvent(action.event, -1, false, action);
        }
        else
        {
            action.returnmessage = "Unable to locate event to edit";
        }

    }

    /**
     * Pull a list of playlist events.
     * Selects all events between m_triggertime and m_triggertime + m_duration,
     * and provides them to the EventSource using the updatePlaylist callback.
     *
     * @param action EventAction containing data for this event
     */
    void updateEvents (EventAction& action)
    {
        std::vector<MouseCatcherEvent> eventdata;
        int channelref;
        try
        {
            channelref = Channel::getChannelByName(action.event.m_channel);
        } catch (std::exception&)
        {
            // Log error
            g_logger.warn("MouseCatcherCore::updateEvents",
                    "Channel name supplied was not"
                            " found global channel list");
            action.returnmessage = "Invalid channel name supplied";
            return;
        }
        getEvents(channelref, action.event.m_triggertime,
        		action.event.m_duration, eventdata);

        action.thisplugin->updatePlaylist(eventdata, action.additionaldata);
    }

    /**
     * Gets a list of devices loaded into the system and provides it to the
     * updateDevices() callback.
     *
     * @param action EventAction containing data for this event
     */
    void getLoadedDevices (EventAction& action)
    {
        std::map<std::string, std::string> devices;
        for(std::pair<std::string, std::shared_ptr<Device>> currentdevice :  g_devices)
        {
            devices[currentdevice.first] =
                    playlist_device_type_vector.at(currentdevice.second->getType());
        }

        action.thisplugin->updateDevices(devices, action.additionaldata);
    }

    /**
     * Gets a list of actions for a specified device, and passes it to the
     * updateDeviceActions callback.
     *
     * @param action EventAction containing data for this event
     */
    void getTypeActions (EventAction& action)
    {
        std::vector<ActionInformation> typeactions;
        if (1 == g_devices.count(action.event.m_targetdevice))
        {
            for (const ActionInformation *pactiondata : *(g_devices[action.event.m_targetdevice]->m_actionlist))
            {
                typeactions.push_back(*pactiondata);
            }

            action.thisplugin->updateDeviceActions(action.event.m_targetdevice,
                    typeactions, action.additionaldata);
        }
        else
        {
            g_logger.warn("GetTypeActions", "Unable to get actions for nonexistent device " + action.event.m_targetdevice);
            action.returnmessage = "Unable to get actions for nonexistent device " + action.event.m_targetdevice;
        }
    }

    /**
     * Get a list of all files on a device
     *
     * @param action
     */
    void getDeviceFiles (EventAction& action)
    {
        if (1 == g_devices.count(action.event.m_targetdevice))
        {
        	std::vector<std::pair<std::string, int>> files;

            if (EVENTDEVICE_VIDEODEVICE == g_devices[action.event.m_targetdevice]->getType())
            {
                std::shared_ptr<VideoDevice> dev =
                        std::static_pointer_cast<VideoDevice>(g_devices[action.event.m_targetdevice]);
                dev->getFileList(files);
            }
            else if (EVENTDEVICE_CGDEVICE == g_devices[action.event.m_targetdevice]->getType())
            {
                std::shared_ptr<CGDevice> dev =
                        std::static_pointer_cast<CGDevice>(g_devices[action.event.m_targetdevice]);
                dev->getTemplateList(files);
            }
            else
            {
                g_logger.warn("GetTypeActions", "Unable to get files for invalid device " + action.event.m_targetdevice);
                action.returnmessage = "Unable to get files for invalid device " + action.event.m_targetdevice;
                return;
            }
            action.thisplugin->updateFiles(action.event.m_targetdevice, files,
            		action.additionaldata);
        }
        else
        {
            g_logger.warn("GetTypeActions", "Unable to get files for nonexistent device " + action.event.m_targetdevice);
            action.returnmessage = "Unable to get files for nonexistent device " + action.event.m_targetdevice;
        }
    }

    /**
     * Gets a list of eventprocessors.
     *
     * @param action EventAction containing data for this event
     */
    void getEventProcessors (EventAction& action)
    {
        std::map<std::string, ProcessorInformation> processors;

        for (std::pair<std::string, std::shared_ptr<MouseCatcherProcessorPlugin>> thisprocessor : g_mcprocessors)
        {
            if (UNLOAD != thisprocessor.second->getStatus())
            {
                processors[thisprocessor.first] = thisprocessor.second->getProcessorInformation();
            }
            else
            {
                g_mcprocessors.erase(thisprocessor.first);
            }
        }

        action.thisplugin->updateEventProcessors(processors, action.additionaldata);
    }

    /**
     * Converts and sets up new events from an EventSource, running EventProcessors.
     */
    void eventQueueTicks ()
    {
        for (EventAction& thisaction : *g_pactionqueue)
        {
            EventAction* p_action = &thisaction;

            if (p_action->isprocessed != true)
            {
                try
                {
                    switch (thisaction.action)
                    {
                        case ACTION_ADD:
                        {
                            thisaction.eventid = processEvent(thisaction.event, -1, false, *p_action);
                        }
                        break;
                        case ACTION_REMOVE:
                        {
                            removeEvent(thisaction);
                        }
                        break;
                        case ACTION_EDIT:
                        {
                            editEvent(thisaction);
                        }
                        break;
                        case ACTION_UPDATE_PLAYLIST:
                        {
                            if (thisaction.thisplugin)
                            {
                                updateEvents(thisaction);
                            }
                        }
                        break;
                        case ACTION_UPDATE_DEVICES:
                        {
                            if (thisaction.thisplugin)
                            {
                                getLoadedDevices(thisaction);
                            }
                        }
                        break;
                        case ACTION_UPDATE_ACTIONS:
                        {
                            if (thisaction.thisplugin)
                            {
                                getTypeActions(thisaction);
                            }
                        }
                        break;
                        case ACTION_UPDATE_PROCESSORS:
                        {
                            if (thisaction.thisplugin)
                            {
                                getEventProcessors(thisaction);
                            }
                        }
                        break;
                        case ACTION_UPDATE_FILES:
                        {
                        	if (thisaction.thisplugin)
                        	{
                        		getDeviceFiles(thisaction);
                        	}
                        }
                        break;
                        default:
                        {
                            throw std::exception();
                        }
                        break;
                    }
                }
                catch (std::exception&)
                {
                    g_logger.warn("MouseCatcherCore::eventQueueTicks", "Unknown Action returned from g_pactionqueue");
                    thisaction.returnmessage = "Unknown Action type found";
                }
                thisaction.isprocessed = true;
            }
        }
    }

    /**
     * Converts a PlaylistEntry to a MouseCatcher event.
     *
     * @param pplaylistevent  The event to convert.
     * @param pchannel        Pointer to this event's channel
     * @param pgeneratedevent Pointer to a MouseCatcherEvent for output
     * @param plog            Pointer to global logging instance
     * @return                True on success, false on failure
     */
    bool convertToMCEvent (PlaylistEntry *pplaylistevent, std::shared_ptr<Channel> pchannel,
            MouseCatcherEvent *pgeneratedevent, Log *plog)
    {
        try
        {
            pgeneratedevent->m_channel = pchannel->m_channame;
            pgeneratedevent->m_targetdevice = pplaylistevent->m_device;
            pgeneratedevent->m_duration = pplaylistevent->m_duration;
            pgeneratedevent->m_eventtype = pplaylistevent->m_eventtype;
            pgeneratedevent->m_triggertime = pplaylistevent->m_trigger;
            pgeneratedevent->m_action = pplaylistevent->m_action;
            pgeneratedevent->m_extradata = pplaylistevent->m_extras;
            pgeneratedevent->m_eventid = pplaylistevent->m_eventid;
            pgeneratedevent->m_preprocessor = pplaylistevent->m_preprocessor;

            // Check the device/processor is real and remains active
            if ((0 == g_devices.count(pgeneratedevent->m_targetdevice)) &&
                    (0 == g_mcprocessors.count(pgeneratedevent->m_targetdevice)))
            {
                g_logger.warn("convertToMCEvent", "Got event for non-existent or unloaded device or processor: " +
                        pgeneratedevent->m_targetdevice);
                throw std::exception();
            }

            // Get an action name if not an EP
            if (pgeneratedevent->m_action > -1)
            {
                try
                {
                    pgeneratedevent->m_action_name = g_devices[pgeneratedevent->m_targetdevice]->m_actionlist->
                            at(pgeneratedevent->m_action)->name;
                }
                catch (std::exception &ex)
                {
                    g_logger.warn("convertToMCEvent", "Unable to locate action with index " +
                            ConvertType::intToString(pgeneratedevent->m_action) + " on device " +
                            pgeneratedevent->m_targetdevice);
                }

            }

            // Recursively grab child events
            std::vector<PlaylistEntry> eventchildren =
                    pchannel->m_pl.getChildEvents(pplaylistevent->m_eventid);

            for (PlaylistEntry thischild : eventchildren)
            {
                MouseCatcherEvent tempchild;
                MouseCatcherCore::convertToMCEvent(&thischild, pchannel, &tempchild,
                        plog);
                pgeneratedevent->m_childevents.push_back(tempchild);
            }
        }
        catch (...)
        {
            plog->error("convert_to_mc_event", "Failed to convert event " +
                    ConvertType::intToString(pplaylistevent->m_eventid) +
                    " to MouseCatcher event");
            return false;
        }

        return true;
    }

    /**
     * Convert a MouseCatcher event to a playlist event
     * @param pmcevent       Pointer to the event to convert
     * @param parentid       ID of the parent event if this is a child, -1 otherwise
     * @param pplaylistevent A pointer to a PlaylistEntry for output
     *
     * @return               True on success, false on failure.
     */
    bool convertToPlaylistEvent (MouseCatcherEvent *pmcevent, int parentid,
            PlaylistEntry *pplaylistevent)
    {
        if (1 == g_devices.count(pmcevent->m_targetdevice))
        {
            pplaylistevent->m_devicetype =
                    g_devices[pmcevent->m_targetdevice]->getType();
        }
        else
        {
            if (1 == g_mcprocessors.count(pmcevent->m_targetdevice))
            {
                pplaylistevent->m_devicetype = EVENTDEVICE_PROCESSOR;
            }
            else
            {
                g_logger.warn("convertToPlaylistEvent",
                        "Unable to convert as device " + pmcevent->m_targetdevice
                                + " does not exist.");
                return false;
            }
        }

        pplaylistevent->m_device = pmcevent->m_targetdevice;
        pplaylistevent->m_duration = pmcevent->m_duration;
        pplaylistevent->m_trigger = pmcevent->m_triggertime;
        pplaylistevent->m_action = pmcevent->m_action;
        pplaylistevent->m_extras = pmcevent->m_extradata;
        pplaylistevent->m_preprocessor = pmcevent->m_preprocessor;

        if (parentid > -1)
        {
            pplaylistevent->m_parent = parentid;
        }
        else
        {
            pplaylistevent->m_eventtype = pmcevent->m_eventtype;
        }

        return true;
    }

}

