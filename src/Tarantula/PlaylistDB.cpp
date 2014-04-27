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
*   Description : Extends SQLiteDB to use as the playlist database
*
*****************************************************************************/


#include "PlaylistDB.h"
#include "TarantulaCore.h"
#include "Log.h"
#include "Misc.h"

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
    m_action = 0;
    m_devicetype = EVENTDEVICE_PROCESSOR;
    m_extras.clear();
}

/**
 * Constructor.
 * Generates a database structure and prepares queries for other functions
 *
 */
PlaylistDB::PlaylistDB (std::string channel_name) :
        SQLiteDB(g_pbaseconfig->getDatabasePath())
{
	// Identify db table names
	std::string evt = "\"" + channel_name + "_events\"";
	std::string edt = "\"" + channel_name + "_extradata\"";

    // Do the initial database setup
    oneTimeExec("CREATE TABLE IF NOT EXISTS " + evt + " (id INTEGER PRIMARY KEY AUTOINCREMENT, type INT, trigger INT64, "
    		"device TEXT, devicetype INT, action, duration INT, parent INT, processed INT, lastupdate INT64, "
    		"callback TEXT, description TEXT)");
    oneTimeExec("CREATE TABLE IF NOT EXISTS " + edt + " (eventid INT, key TEXT, value TEXT, processed INT)");
    oneTimeExec("CREATE INDEX IF NOT EXISTS \"" + channel_name + "_trigger_index\" ON " + evt + " (trigger)");

    // Queries used by other functions
    m_addevent_query = prepare("INSERT INTO " + evt + " (type, trigger, device, devicetype, action, duration, "
            "parent, processed, lastupdate, callback, description) "
            "VALUES (?,?,?,?,?,?,?,0, strftime('%s', 'now'),?,?)");

    m_getevent_query = prepare("SELECT id, type, trigger, device, devicetype, action, duration, "
            "parent, callback, description "
            "FROM " + evt + " "
            "WHERE type = ? AND trigger = ? AND processed = 0");

    m_getchildevents_query = prepare("SELECT id, type, trigger, device, devicetype, action, duration, "
            "parent, callback, description "
            "FROM " + evt + " "
            "WHERE parent = ? AND processed = 0 "
            "ORDER BY trigger ASC");

    m_getparentevent_query = prepare("SELECT ev.id FROM " + evt + " AS ev "
            "LEFT JOIN " + evt + " as cev ON ev.id = cev.parent "
            "WHERE cev.id = ? AND ev.processed >= 0");

    m_geteventdetails_query = prepare("SELECT id, type, trigger, device, devicetype, action, duration, "
            "parent, callback, description "
            "FROM " + evt + " "
            "WHERE id = ? AND processed >= 0");

    m_removeevent_query = prepare("UPDATE " + evt + " SET processed = -1 WHERE id = ?; "
    		"DELETE FROM " + edt + " WHERE eventid = ?");

    m_processevent_query = prepare("UPDATE " + evt + " SET processed = 1, lastupdate = strftime('%s', 'now') WHERE "
            "id = ? AND processed >= 0; "
            "UPDATE " + edt + " SET processed = 1 WHERE eventid = ?");

    m_addextras_query = prepare("INSERT INTO " + edt + " VALUES (?,?,?,0)");

    m_getextras_query = prepare("SELECT key,value FROM " + edt + " WHERE eventid = ?");

    m_gethold_query = prepare("SELECT id FROM " + evt + " WHERE trigger <= ? AND processed = 0 AND type = ? "
            "ORDER BY trigger DESC LIMIT 1");

    // Queries used by EventSource interface
    m_geteventlist_query = prepare("SELECT DISTINCT events.id, events.type, events.trigger, events.device, "
    		"events.devicetype, events.action, events.duration, events.parent, "
    		"events.callback, events.description, extradata.key, extradata.value "
    		"FROM " + evt + " AS events LEFT JOIN " + edt + " AS extradata ON extradata.eventid = events.id "
    		"WHERE trigger >= ? AND trigger < ? AND parent = 0 AND events.processed >= 0 "
    		"ORDER BY trigger ASC, events.id ASC");

    m_getcurrent_toplevel_query = prepare ("SELECT events.id, events.type, events.trigger, events.device, "
    		"events.devicetype, events.action, events.duration, events.parent,  "
    		"events.callback, events.description "
    		"FROM " + evt + " AS events "
    		"WHERE parent = 0 AND processed > 0 "
            "AND (trigger + duration) < strftime('%s', 'now') "
            "ORDER BY trigger DESC, duration ASC");

    m_getnext_toplevel_query = prepare ("SELECT events.id, events.type, events.trigger, events.device, "
    		"events.devicetype, events.action, events.duration, events.parent,  "
    		"events.callback, events.description "
    		"FROM " + evt + " AS events "
    		"WHERE parent = 0 AND processed = 0 "
    		"AND trigger > strftime('%s', 'now')");

    // Queries used by playlist sync system
    m_getdeletelist_query = prepare("SELECT id FROM " + evt + " WHERE processed = -1; "
            "DELETE FROM " + evt + " WHERE processed = -1");

    m_getupdatelist_query = prepare("SELECT id, type, trigger, device, devicetype, action, duration, parent, "
            "lastupdate, callback, description FROM " + evt + " WHERE lastupdate > ? AND processed >= 0");

    m_getextradata_query = prepare("SELECT * FROM " + edt + " AS extradata LEFT JOIN " + evt + " AS events "
            "ON extradata.eventid = events.id WHERE events.processed >= 0 AND events.lastupdate > ?");

    // Queries used by Shunt command
    m_shunt_eventcount_query = prepare("SELECT trigger, duration FROM " + evt + " "
    		"WHERE parent = 0 AND processed >= 0 AND trigger >= ? AND trigger < ? "
    		"ORDER BY trigger ASC, duration DESC LIMIT 1");

    m_shunt_eventupdate_query = prepare("UPDATE " + evt + " SET trigger = trigger + ?, lastupdate = strftime('%s', 'now') "
            "WHERE trigger >= ? AND trigger < ?");
}

/**
 * Adds a new event to the database.
 *
 * @param &obj Address of the event to add to the playlist database
 * @return     The ID of the added event
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
    m_addevent_query->addParam(8, DBParam(pobj->m_preprocessor));
    m_addevent_query->addParam(9, DBParam(pobj->m_description));

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
    pple->m_device =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text (pstmt, 3)));
    pple->m_devicetype = static_cast<playlist_device_type_t>(sqlite3_column_int(
            pstmt, 4));
    pple->m_action = sqlite3_column_int(pstmt, 5);
    pple->m_duration = sqlite3_column_int(pstmt, 6);
    pple->m_parent = sqlite3_column_int(pstmt, 7);
    pple->m_preprocessor =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text (pstmt, 8)));
    pple->m_description =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text (pstmt, 9)));
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
    	// Append to the last entry if ids match
    	if (eventlist.size() > 0 &&
    			sqlite3_column_int(stmt, 0) == eventlist.back().m_eventid)
    	{
    		eventlist.back().m_extras[reinterpret_cast<const char*>(
            		sqlite3_column_text (stmt, 10))] =
                    reinterpret_cast<const char*>(sqlite3_column_text (stmt, 11));
    	}
    	else
    	{

			PlaylistEntry ple;
			populateEvent(stmt, &ple);

			ple.m_extras[reinterpret_cast<const char*>(
				sqlite3_column_text (stmt, 10))] =
			    reinterpret_cast<const char*>(
			    	sqlite3_column_text (stmt, 11));

			eventlist.push_back(ple);
    	}
    }

    // Now go and get the extradata array


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
 * Look up the parent event from a child's ID
 *
 * @param eventID    ID of child event for which to find parent
 * @return           Parent event ID or -1 if none found
 */
int PlaylistDB::getParentEventID (int eventID)
{
    m_getparentevent_query->rmParams();
    m_getparentevent_query->addParam(1, DBParam(eventID));
    m_getparentevent_query->bindParams();
    sqlite3_stmt *stmt = m_getparentevent_query->getStmt();

    if (SQLITE_ROW == sqlite3_step(stmt))
    {
        return sqlite3_column_int(stmt, 0);
    }
    else
    {
        return -1;
    }
}

/**
 * Get the details on an event by ID
 *
 * @param eventID    ID of event to grab details for
 * @param foundevent Event to populate with details
 * @return           True if an event was found
 */
bool PlaylistDB::getEventDetails (int eventID, PlaylistEntry &foundevent)
{
    m_geteventdetails_query->rmParams();
    m_geteventdetails_query->addParam(1, DBParam(eventID));
    m_geteventdetails_query->bindParams();
    sqlite3_stmt *stmt = m_geteventdetails_query->getStmt();

    if (SQLITE_ROW == sqlite3_step(stmt))
    {
        populateEvent(stmt, &foundevent);
        getExtraData(&foundevent);
        return true;
    }
    else
    {
        return false;
    }
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
    m_processevent_query->addParam(2, eventID);
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
    m_removeevent_query->addParam(2, eventID);
    m_removeevent_query->bindParams();
    sqlite3_step(m_removeevent_query->getStmt());
}

/**
 * Return the event ID of the most-recently activated manual hold
 *
 * @param bytime The time at which to search (usually now)
 * @return       EventID of hold event, or 0 for no hold
 */
int PlaylistDB::getActiveHold(time_t bytime)
{
    m_gethold_query->rmParams();
    m_gethold_query->addParam(1, DBParam(bytime));
    m_gethold_query->addParam(2, DBParam(EVENT_MANUAL));
    m_gethold_query->bindParams();

    sqlite3_stmt* stmt = m_gethold_query->getStmt();
    if (SQLITE_ROW == sqlite3_step(stmt))
    {
        return sqlite3_column_int(stmt, 0);
    }
    else
    {
        return 0;
    }
}

/**
 * Move all connected events in or out by a delay factor to delay entire schedule.
 *
 * @param starttime   Time in current playlist to shunt from. Shunt will catch events after this.
 * @param shuntlength Number of seconds forward or back to shunt
 */
void PlaylistDB::shunt(time_t starttime, int shuntlength)
{
    int searchdelay = 0;
    // When the shunt is backwards, the delay should not be accounted for
    if (shuntlength >= 0)
    {
        searchdelay = shuntlength;
    }

    // Increase to catch not-quite overlapping events
    int fudgefactor = 5;

    time_t startmark = starttime;
    time_t endmark = starttime + searchdelay + fudgefactor;

    // Find the edges of the time boundary to shunt
    while (1)
    {
        m_shunt_eventcount_query->rmParams();
        m_shunt_eventcount_query->addParam(1, DBParam(startmark));
        m_shunt_eventcount_query->addParam(2, DBParam(endmark));
        m_shunt_eventcount_query->bindParams();

        sqlite3_stmt *stmt = m_shunt_eventcount_query->getStmt();
        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            break;
        }

        startmark = sqlite3_column_int(stmt, 0) + 1;
        int newdelay = sqlite3_column_int(stmt, 1);
        endmark = startmark + newdelay + searchdelay + fudgefactor;
    }

    // Apply the shunt
    m_shunt_eventupdate_query->rmParams();
    m_shunt_eventupdate_query->addParam(1, DBParam(shuntlength));
    m_shunt_eventupdate_query->addParam(2, DBParam(starttime));
    m_shunt_eventupdate_query->addParam(3, DBParam(endmark));
    m_shunt_eventupdate_query->bindParams();
    sqlite3_step(m_shunt_eventupdate_query->getStmt());

}

/**
 * Get event all currently running top level events
 */
std::vector<PlaylistEntry> PlaylistDB::getExecutingEvents()
{
	std::vector<PlaylistEntry> eventlist;

	m_getcurrent_toplevel_query->rmParams();
	m_getcurrent_toplevel_query->bindParams();

	sqlite3_stmt* stmt = m_getcurrent_toplevel_query->getStmt();
	while (SQLITE_ROW == sqlite3_step(stmt))
	{
		PlaylistEntry ple;
		populateEvent(stmt, &ple);

		getExtraData(&ple);

		eventlist.push_back(ple);
	}

	return eventlist;
}

/**
 * Get the next top-level event to execute
 */
PlaylistEntry PlaylistDB::getNextEvent()
{
	m_getnext_toplevel_query->rmParams();
	m_getnext_toplevel_query->bindParams();

	sqlite3_stmt* stmt = m_getnext_toplevel_query->getStmt();

	if (SQLITE_ROW == sqlite3_step(stmt))
	{
		PlaylistEntry ple;
		populateEvent(stmt, &ple);

		getExtraData(&ple);

		return ple;
	}
	else
	{
		throw std::exception();
	}
}
