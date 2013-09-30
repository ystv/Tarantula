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
*   File Name   : MouseCatcherCore.h
*   Version     : 1.0
*   Description : The central functions for MouseCatcher
*
*****************************************************************************/


#pragma once

#include "MouseCatcherSourcePlugin.h"
#include "MouseCatcherProcessorPlugin.h"
#include "TarantulaCore.h"
#include "Log.h"
#include "MouseCatcherCommon.h"
#include <map>
#include <vector>

//! All active source plugins
extern std::vector<std::shared_ptr<MouseCatcherSourcePlugin>> g_mcsources;
extern std::map<std::string, std::shared_ptr<MouseCatcherProcessorPlugin>> g_mcprocessors;

/**
 * The MouseCatcher tool for event retrieval, mapping and processing and eventual insertion
 * Not a class as the functions are all statics
 */
namespace MouseCatcherCore
{
    //! The queue of events to process
    extern std::vector<EventAction>* g_pactionqueue;

    void init (std::string sourcepath, std::string processorpath);
    void loadAllEventSourcePlugins (std::string path);
    void loadAllEventProcessorPlugins (std::string path);
    void eventSourcePluginTicks ();
    void eventQueueTicks ();

    // Playlist ActionQueue functions
    void removeEvent (EventAction& action);
    void editEvent (EventAction& action);
    void updateEvents (EventAction& action);
    void shuntEvents (EventAction& action);
    void triggerEvent (EventAction& action);
    int processEvent (MouseCatcherEvent event, int lastid, bool ischild,
            EventAction& action);

    // Devices ActionQueue functions
    void getLoadedDevices (EventAction& action);
    void getTypeActions (EventAction& action);

    // Convenience functions
    bool convertToPlaylistEvent (MouseCatcherEvent * const mcevent,
            int parentid, PlaylistEntry *playlistevent);
    bool convertToMCEvent (PlaylistEntry * const playlistevent,
            std::shared_ptr<Channel> channel, MouseCatcherEvent *generatedevent, Log *log);
    void getEvents (int channelid, time_t starttime, int length,
                std::vector<MouseCatcherEvent>& eventvector);
}
