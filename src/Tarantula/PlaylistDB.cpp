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
*   File Name   : PlaylistDB.cpp
*   Version     : 1.0
*   Description : Extends MemDB to use as the playlist database
*
*****************************************************************************/


#include "PlaylistDB.h"

/**
 * Equivalent to a row in the playlist database, containing
 * all the data for a single event
 *
 */
PlaylistEntry::PlaylistEntry ()
{
    m_eventid = -1;
    m_eventtype = EVENT_FIXED;
    m_trigger = 0;
    m_device.clear();
    m_duration = 0;
    m_parent = 0;
    m_extras.clear();
}

/**
 * Constructor.
 * Generates a database structure and prepares queries for other functions
 *
 */
PlaylistDB::PlaylistDB () :
        MemDB()
{
    // Do the initial database setup
    oneTimeExec("CREATE TABLE events (type INT, trigger INT64, device TEXT, devicetype INT, action, duration INT, parent INT, processed INT, lastupdate INT64)");
    oneTimeExec("CREATE TABLE extradata (eventid INT, key TEXT, value TEXT, processed INT)");

    // Queries used by other functions
    m_addevent_query = prepare("INSERT INTO events VALUES (?,?,?,?,?,?,?,0, strftime('%s', 'now'))");
    m_getevent_query = prepare("SELECT RowID,* FROM events WHERE type = ? AND trigger = ? AND processed = 0");
    m_getchildevents_query = prepare("SELECT RowID,* FROM events WHERE parent = ? AND processed = 0");
    m_removeevent_query = prepare("DELETE FROM events WHERE rowid = ?; DELETE FROM extradata WHERE eventid = ?");
    m_processevent_query = prepare("UPDATE events SET processed = 1, lastupdate = strftime('%s', 'now') WHERE rowid = ?");
    m_addextras_query = prepare("INSERT INTO extradata VALUES (?,?,?,?)");
    m_getextras_query = prepare("SELECT key,value FROM extradata WHERE eventid = ?");

    // Queries used by EventSource interface
    m_geteventlist_query = prepare("SELECT RowID, events.* FROM events WHERE trigger > ? AND trigger < ? AND parent = 0");
    m_updateevent_query = prepare("UPDATE events SET type = ?, trigger = ?, filename = ?, device = ?, devicetype = ?, duration = ?, lastupdate = strftime('%s', 'now')");
}

/**
 * Adds a new event to the database.
 *
 * @param &obj Address of the event to add to the playlist database
 * @return     The SQLite ROWID of the added event
 */
int PlaylistDB::addEvent (PlaylistEntry *pobj)
{
    m_addevent_query->rmParams();
    m_addevent_query->addParam(1, DBParam(pobj->m_eventtype));
    m_addevent_query->addParam(2, DBParam(pobj->m_trigger));
    m_addevent_query->addParam(3, DBParam(pobj->m_device));
    m_addevent_query->addParam(4, DBParam(pobj->m_devicetype));
    m_addevent_query->addParam(5, DBParam(pobj->m_action));
    m_addevent_query->addParam(6, DBParam(pobj->m_duration));
    m_addevent_query->addParam(7, DBParam(pobj->m_parent));

    m_addevent_query->bindParams();
    int eventid = -1;
    int result = sqlite3_step(m_addevent_query->getStmt());

    if (result == SQLITE_DONE)
    {
        eventid = getLastRowID();
        //Now store all the extradata stuff
        for (std::map<std::string, std::string>::iterator it =
                pobj->m_extras.begin(); it != pobj->m_extras.end(); it++)
        {
            m_addextras_query->rmParams();
            m_addextras_query->addParam(1, DBParam(eventid));
            m_addextras_query->addParam(2, DBParam((*it).first));
            m_addextras_query->addParam(3, DBParam((*it).second));
            m_addextras_query->bindParams();
            sqlite3_step(m_addextras_query->getStmt());
        }
    }

    return eventid;
}

/**
 * Extract event data from a SELECT * FROM events type query.
 *
 * @param pstmt Pointer to a sqlite statement containing the event data
 * @param pple  Pointer to a PlaylistEntry to populate.
 */
void PlaylistDB::populateEvent (sqlite3_stmt *pstmt, PlaylistEntry *pple)
{
    pple->m_eventid = sqlite3_column_int(pstmt, 0);
    pple->m_eventtype = static_cast<playlist_event_type_t>(sqlite3_column_int(
            pstmt, 1));
    pple->m_trigger = sqlite3_column_int(pstmt, 2);
    pple->m_device = std::string(
            reinterpret_cast<const char*>(sqlite3_column_text (pstmt, 3)));
    pple->m_devicetype = static_cast<playlist_device_type_t>(sqlite3_column_int(
            pstmt, 4));
    pple->m_action = sqlite3_column_int(pstmt, 5);
    pple->m_duration = sqlite3_column_int(pstmt, 6);
    pple->m_parent = sqlite3_column_int(pstmt, 7);
}

/**
 * Get the key-value map of extra data for the event in ple
 *
 * @param pple Pointer to a playlist event already populated by populateEvent
 */
void PlaylistDB::getExtraData (PlaylistEntry *pple)
{
    m_getextras_query->rmParams();
    m_getextras_query->addParam(1, DBParam(pple->m_eventid));
    m_getextras_query->bindParams();
    sqlite3_stmt *extrastmt = m_getextras_query->getStmt();
    while (sqlite3_step(extrastmt) == SQLITE_ROW)
    {
        pple->m_extras[reinterpret_cast<const char*>(sqlite3_column_text (
                extrastmt, 0))] =
                reinterpret_cast<const char*>(sqlite3_column_text (extrastmt,
                        1));
    }
}

/**
 * Gets an event of the specified type and trigger from the database
 *
 * @param type    The event type, using one of the EVENT_ values
 * @param trigger The event trigger, either a Unix timestamp or the previous event's ID
 * @return        The event found at this time
 */
std::vector<PlaylistEntry> PlaylistDB::getEvents (playlist_event_type_t type,
        time_t trigger)
{
    std::vector<PlaylistEntry> eventlist;
    m_getevent_query->rmParams();
    m_getevent_query->addParam(1, DBParam(type));
    m_getevent_query->addParam(2, DBParam(trigger));
    m_getevent_query->bindParams();

    sqlite3_stmt *stmt = m_getevent_query->getStmt();
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PlaylistEntry ple;
        populateEvent(stmt, &ple);

        getExtraData(&ple);

        eventlist.push_back(ple);
    }
    return eventlist;
}

/**
 * Gets all events changed since a specified time
 *
 * @param starttime Start of the time period to fetch events for
 * @param length	Length of the time period to fetch events for
 * @return          A vector full of matching events
 */
std::vector<PlaylistEntry> PlaylistDB::getEventList (time_t starttime,
		int length)
{
    std::vector<PlaylistEntry> eventlist;
    m_geteventlist_query->rmParams();
    m_geteventlist_query->addParam(1, DBParam(starttime));
    time_t endtime = starttime + length;
    m_geteventlist_query->addParam(2, DBParam(endtime));
    m_geteventlist_query->bindParams();

    sqlite3_stmt *stmt = m_geteventlist_query->getStmt();
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PlaylistEntry ple;
        populateEvent(stmt, &ple);

        getExtraData(&ple);

        eventlist.push_back(ple);
    }
    return eventlist;
}

/**
 * Gets child events from the given Parent ID
 *
 * @param parentid The parent event to search for children of
 * @return         The events with this parent
 */
std::vector<PlaylistEntry> PlaylistDB::getChildEvents (int parentid)
{
    std::vector<PlaylistEntry> eventlist;
    m_getchildevents_query->rmParams();
    m_getchildevents_query->addParam(1, DBParam(parentid));
    m_getchildevents_query->bindParams();
    sqlite3_stmt *stmt = m_getchildevents_query->getStmt();
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        PlaylistEntry ple;
        populateEvent(stmt, &ple);

        getExtraData(&ple);

        eventlist.push_back(ple);
    }
    return eventlist;
}

/**
 * Mark event as processed in the database
 *
 * @param eventid The ID of the event to mark
 */
void PlaylistDB::processEvent (int eventID)
{
    m_processevent_query->rmParams();
    m_processevent_query->addParam(1, eventID);
    m_processevent_query->bindParams();
    sqlite3_step(m_processevent_query->getStmt());
}

/**
 * Removes event with the specified ID from the database, along with children (recursive)
 *
 * @param eventid The ID of the event to remove
 */
void PlaylistDB::removeEvent (int eventID)
{
    //Remove children recursively
    std::vector<PlaylistEntry> children = getChildEvents(eventID);
    for (PlaylistEntry child : children)
    {
        removeEvent(child.m_eventid);
    }

    m_removeevent_query->rmParams();
    m_removeevent_query->addParam(1, eventID);
    m_removeevent_query->bindParams();
    sqlite3_step(m_removeevent_query->getStmt());
}

