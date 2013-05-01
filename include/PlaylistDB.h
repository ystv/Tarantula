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
*   File Name   : PlaylistDB.h
*   Version     : 1.0
*   Description : Extends MemDB to use as the playlist database
*
*****************************************************************************/


#pragma once

#include <iostream>
#include <cstdlib>
#include <map>
#include "Log.h"
#include "MemDB.h" //parent class


enum playlist_event_type_t
{
    EVENT_FIXED = 0, EVENT_MANUAL = 1, EVENT_CHILD = 2
};

const std::map<playlist_event_type_t, std::string> playlist_event_type_vector =
        { { EVENT_FIXED, "fixed" }, { EVENT_MANUAL, "manual" }, { EVENT_CHILD,
                "child" } };

enum playlist_device_type_t
{
    EVENTDEVICE_CROSSPOINT,
    EVENTDEVICE_VIDEODEVICE,
    EVENTDEVICE_CGDEVICE,
    EVENTDEVICE_PROCESSOR
};

const std::map<playlist_device_type_t, std::string> playlist_device_type_vector =
        { { EVENTDEVICE_CROSSPOINT, "Crosspoint" }, { EVENTDEVICE_VIDEODEVICE,
                "Video" }, { EVENTDEVICE_CGDEVICE, "CG" }, {
                EVENTDEVICE_PROCESSOR, "Processor" } };

/**
 * Stores details about actions
 */
class ActionInformation
{
public:
    int actionid;
    std::string name;
    std::string description;
    std::map<std::string, std::string> extradata;
    operator int () const
    {
        return actionid;
    }

    inline bool operator== (const ActionInformation& rhs) const
    {
        return this->actionid == rhs.actionid;
    }
    inline bool operator< (const ActionInformation& rhs) const
    {
        return this->actionid < rhs.actionid;
    }
    // All other comparison overloads are trivial applications of the two above
    inline bool operator!= (const ActionInformation& rhs) const
    {
        return !operator==(rhs);
    }
    inline bool operator> (const ActionInformation& rhs) const
    {
        return operator<(rhs);
    }
    inline bool operator<= (const ActionInformation& rhs) const
    {
        return !operator>(rhs);
    }
    inline bool operator>= (const ActionInformation& rhs) const
    {
        return !operator<(rhs);
    }

    // Enable comparison with integers
    bool operator== (const int& rhs) const
    {
        return this->actionid == rhs;
    }
};

/**
 * Equivalent to a row in the playlist database, containing
 * all the data for a single event
 */
class PlaylistEntry
{
public:
    int m_eventid;
    playlist_event_type_t m_eventtype;
    //! May be either a unix timestamp or a manual event something (see #42)
    int m_trigger;
    std::string m_device;
    playlist_device_type_t m_devicetype;
    int m_action;
    //! Duration measured in seconds
    int m_duration;
    int m_parent;
    std::map<std::string, std::string> m_extras;
    PlaylistEntry ();
};

/**
 * This is an extension of MemDB to hold Playlist data in a playlist table,
 * with a structure corresponding to the playlist XML spec.
 */
class PlaylistDB: public MemDB
{
public:
    PlaylistDB ();
    int addEvent (PlaylistEntry *pobj);

    std::vector<PlaylistEntry> getEvents (playlist_event_type_t type,
            time_t trigger);
    std::vector<PlaylistEntry> getChildEvents (int parentid);
    std::vector<PlaylistEntry> getEventList (time_t starttime, int length);
    void processEvent (int eventID);
    void removeEvent (int eventID);

private:
    void populateEvent (sqlite3_stmt *pstmt, PlaylistEntry *pple);
    void getExtraData (PlaylistEntry *pple);
    DBQuery* m_addevent_query;
    DBQuery* m_getevent_query;
    DBQuery* m_getchildevents_query;
    DBQuery* m_removeevent_query;
    DBQuery* m_processevent_query;
    DBQuery* m_addextras_query;
    DBQuery* m_getextras_query;
    DBQuery* m_geteventlist_query;
    DBQuery* m_updateevent_query;
};
