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
*   File Name   : EventProcessor_Fill.cpp
*   Version     : 1.0
*   Description : Implementation of an EventProcessor to generate events for
*   idents, trailers and continuity graphics to fill schedules. Also contains
*   a FillDB class to extend MemDB and provide a database backend.
*
*****************************************************************************/

#include <sstream>
#include <mutex>
#include <functional>

#include "pugixml.hpp"
#include "EventProcessor_Fill.h"
#include "PluginConfig.h"
#include "Misc.h"
#include "MouseCatcherCore.h"
#include "DateConversions.h"

/*****************************************************************************
 * FillDB Implementation
 ****************************************************************************/

/**
 * Default constructor. Creates/opens underlying SQLite database and sets up
 * queries based on given configuration
 *
 * @param databasefile Filename and path of database file.
 * @param weightpoints Map of time weighting points and values
 * @param fileweight   Scale factor to apply to weight column on item
 */
FillDB::FillDB (std::string databasefile, std::map<int, int>& weightpoints,
		int fileweight) :
		MemDB()
{

	// Table creation
	oneTimeExec("CREATE TABLE items (id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"name TEXT NOT NULL, device TEXT NOT NULL, type TEXT NOT NULL, "
			"duration INT NOT NULL, weight INT NOT NULL)");
	oneTimeExec("CREATE TABLE plays (id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"itemid INT NOT NULL, timestamp INT)");

	if (!databasefile.empty())
	{
		// Load from existing
	    try
	    {
            oneTimeExec("ATTACH \"" + databasefile + "\" AS disk");

            oneTimeExec("BEGIN TRANSACTION");

            oneTimeExec("INSERT INTO items (id, name, device, type, duration, weight) "
                    "SELECT id, name, device, type, duration, weight FROM disk.items");
            oneTimeExec("INSERT INTO plays (id, itemid, timestamp) "
                    "SELECT id, itemid, timestamp FROM disk.plays");
            oneTimeExec("DETACH disk");

            oneTimeExec("END TRANSACTION");
	    }
	    catch (std::exception &ex)
	    {
	    }
	}

	// Index speeds up some lookups
	oneTimeExec("CREATE INDEX itemid_index ON plays (itemid)");

	m_paddplay_query = prepare("INSERT INTO plays (itemid, timestamp) "
			"VALUES (?, ?);");
	m_paddfile_query = prepare("INSERT INTO items (name, device, type, duration, "
			"weight) VALUES (?, ?, ?, ?, ?);");

    m_pgetbestfile_query = NULL;
    updateWeightPoints(weightpoints, fileweight);

}

/**
 * Assemble the SQL query to get the best file based on configured weightings
 *
 * @param weightpoints Map of time weighting points and values
 * @param fileweight   Scale factor to apply to weight column on item
 */
void FillDB::updateWeightPoints(std::map<int, int>& weightpoints, int fileweight)
{
	// Assemble the start of the best file query
	std::stringstream newquery;
	newquery << "SELECT items.name, items.duration, items.id, total(CASE ";

	// Assemble the weightpoints query segment
	std::map<int, int>::iterator lastitem = weightpoints.begin();
	for (std::map<int, int>::iterator it = weightpoints.begin()++;
			it != weightpoints.end(); ++it)
	{

		newquery << "WHEN (?1 - timestamp) <= " << it->first << " ";

		if (it != weightpoints.begin())
		{
			newquery << "AND (?1 - timestamp) > " << lastitem->first << " ";
		}
		newquery << " THEN (?1 - timestamp) * " << it->second << " ";

		lastitem = it;
	}

    newquery << "END) + items.weight * " << fileweight << " AS weight FROM items ";
    newquery << " LEFT JOIN plays ON plays.itemid = items.id ";
    newquery << " WHERE device = ?2 AND duration < ?3 AND type = ?4 ";
    newquery << " GROUP BY items.id ORDER BY weight ASC";

    // Erase the old query
    delete m_pgetbestfile_query;

    // Replace the query
    m_pgetbestfile_query = prepare(newquery.str().c_str());
}

/**
 * Mark a played file into the database
 *
 * @param idnumber ID of the item played
 * @param time     Timestamp of the play to add
 */
void FillDB::addPlay(int idnumber, int time)
{
    m_paddplay_query->rmParams();

    m_paddplay_query->addParam(1, idnumber);
    m_paddplay_query->addParam(2, time);

    m_paddplay_query->bindParams();
    sqlite3_step(m_paddplay_query->getStmt());

}

/**
 * Add a file to the database.
 *
 * @param filename Name as known by VideoDevice
 * @param device   VideoDevice this file appears on.
 * @param type     A type as understood by the config file (ident, trailer, etc)
 * @param duration Length in seconds.
 * @param weight   Bias factor for selecting this file. Lower is more often.
 */
void FillDB::addFile (std::string filename, std::string device, std::string type,
        int duration, int weight)
{
    m_paddfile_query->rmParams();

    m_paddfile_query->addParam(1, DBParam(filename));
    m_paddfile_query->addParam(2, DBParam(device));
    m_paddfile_query->addParam(3, DBParam(type));
    m_paddfile_query->addParam(4, duration);
    m_paddfile_query->addParam(5, weight);

    m_paddfile_query->bindParams();
    sqlite3_step(m_paddfile_query->getStmt());
}

/**
 * Synchronise the in-memory database with the one on disk
 *
 * @param databasefile Name of database on disk
 */
void FillDB::syncDatabase (std::string databasefile)
{
	// Attach the file
	oneTimeExec("ATTACH " + databasefile + " AS disk");
	oneTimeExec("BEGIN TRANSACTION");

	// Remove orphaned plays where itemid has gone missing
	oneTimeExec("DELETE FROM plays WHERE plays.itemid NOT IN "
			"(SELECT items.id FROM items)");

	// Remove items missing from disk table
	oneTimeExec("DELETE FROM items WHERE items.id NOT IN "
			"(SELECT disk.items.id FROM disk.items)");

	// Add items missing from memory table
	oneTimeExec("INSERT INTO items (id, name, device, type, duration, weight) "
			"SELECT id, name, device, type, duration, weight FROM "
			"disk.items WHERE disk.items.id NOT IN (SELECT items.id FROM "
			"items)");

	// Sync new plays data
	oneTimeExec("INSERT INTO disk.plays (itemid, timestamp) SELECT  "
			"itemid, timestamp FROM plays WHERE plays.id > "
			"(SELECT COALESCE(MAX(disk.plays.id), 0) FROM disk.plays)");

	// Detach the database
	oneTimeExec("END TRANSACTION");
	oneTimeExec("DETACH disk");

}

/**
 * Start a transaction which other operations may be wrapped in
 */
void FillDB::beginTransaction ()
{
    oneTimeExec("BEGIN TRANSACTION;");
}

/**
 * Complete an active transaction
 */
void FillDB::endTransaction ()
{
    oneTimeExec("END TRANSACTION;");
}

/**
 * Get the lowest-weighted file shorter than a duration for the specified device
 *
 * @param filename		 Name of the file found
 * @param inserttime     The time at which the file will be inserted
 * @param duration       Maximum acceptable duration of file (frames).
 * @param device         Device to find a file for
 * @param type           Type of file to find (ident, trailer, etc)
 * @param resultduration Duration of returned file
 * @param excludeid		 Comma seperated list of RowIDs to exclude completely
 * @return               ID of best file, -1 for not found
 */
int FillDB::getBestFile(std::string& filename, int inserttime, int duration,
		std::string device, std::string type, int& resultduration,
		std::set<int>& excludeid)
{
    // Ensure that updateWeightPoints has been called at least once
    if (!m_pgetbestfile_query)
    {
        throw(INVALID_QUERY_POINTER);
    }

    m_pgetbestfile_query->rmParams();

    m_pgetbestfile_query->addParam(1, inserttime);
    m_pgetbestfile_query->addParam(2, DBParam(device));
    m_pgetbestfile_query->addParam(3, duration);
    m_pgetbestfile_query->addParam(4, DBParam(type));

    m_pgetbestfile_query->bindParams();
    sqlite3_stmt *stmt = m_pgetbestfile_query->getStmt();

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
    	int id = sqlite3_column_int(stmt, 2);

    	if (excludeid.count(id) == 0)
    	{
    		resultduration = sqlite3_column_int(stmt, 1);
			excludeid.insert(id);
			filename = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
			return sqlite3_column_int(stmt, 2);
    	}

    }

    // No valid items found, return failure
    resultduration = 0;
    return -1;
}


/*****************************************************************************
 * EventProcessor_Fill Implementation
 ****************************************************************************/

/**
 * Default constructor. Reads XML configuration data and sets up database
 * connection.
 */
EventProcessor_Fill::EventProcessor_Fill (PluginConfig config, Hook h) :
		MouseCatcherProcessorPlugin(config, h)
{
    // Process configuration file
    readConfig(config);

    // Test if configuration processed cleanly
    if (FAILED == m_status)
    {
    	return;
    }

    // Create database connection
    m_pdb = std::shared_ptr<FillDB>(new FillDB(m_dbfile, m_weightpoints, m_fileweight));

    m_placeholderid = 0;

    // Populate information array
    m_processorinfo.data["duration"] = "int";

    m_status = READY;

    // Register the preprocessor
    g_preprocessorlist.emplace("EventProcessor_Fill::populateCGNowNext", &EventProcessor_Fill::populateCGNowNext);

}

/**
 * Destructor. Syncs the database in memory to disk and erases PP
 */
EventProcessor_Fill::~EventProcessor_Fill ()
{
    m_pdb->syncDatabase(m_dbfile);
    g_preprocessorlist.erase("EventProcessor_Fill::populateCGNowNext");
}

/**
 * Parse the XML configuration file for this plugin
 *
 * @param config Loaded configuration data
 */
void EventProcessor_Fill::readConfig (PluginConfig config)
{
    m_pluginname = config.m_instance;

    m_dbfile = config.m_plugindata_xml.child_value("DBFile");
    if (m_dbfile.empty())
    {
    	m_hook.gs->L->error(config.m_instance, "No database file set in config. Aborting.");
    	m_status = FAILED;
    	return;
    }

    m_fileweight = config.m_plugindata_xml.child("FileWeight").text().as_int(-1);
    if (-1 == m_fileweight)
    {
    	m_hook.gs->L->warn(config.m_instance, "FileWeight factor not set, assuming 0");
    	m_fileweight = 0;
    }

    m_jobpriority = config.m_plugindata_xml.child("JobPriority").text().as_int(-1);
    if (-1 == m_jobpriority)
    {
        m_hook.gs->L->warn(config.m_instance, "JobPriority factor not set, assuming 5");
        m_jobpriority = 5;
    }

    m_cyclesbeforesync = config.m_plugindata_xml.child("CyclesBeforeSync").text().as_int(-1);
    if (-1 == m_cyclesbeforesync)
    {
        m_hook.gs->L->warn(config.m_instance, "CyclesBeforeSync factor not set, assuming 2");
        m_cyclesbeforesync = 2;
    }
    m_cyclesremaining = m_cyclesbeforesync;

    // Parse the time-based weighting data
    for (pugi::xml_node weightpoint :
		config.m_plugindata_xml.child("WeightPoints").children())
	{
    	int time = weightpoint.child("Offset").text().as_int(-1);
    	int weight = weightpoint.child("Weight").text().as_int(-1);

    	if (-1 != time || -1 != weight)
    	{
    		m_weightpoints[time] = weight;
    	}
    }

    // Parse the video type structure data
    for (pugi::xml_node structureitem :
    		config.m_plugindata_xml.child("StructureData").children())
    {
    	std::string type = structureitem.child_value("Type");
    	std::string device = structureitem.child_value("Device");

    	if (!device.empty() && !type.empty())
    	{
    		m_structuredata.push_back(std::make_pair(type, device));
    	}
    }

    // Should the last structure item be repeated?
    m_filler = config.m_plugindata_xml.child("EnableFill").text().as_bool(false);

    // Continuity node events pad out the time after videos run out
    pugi::xml_node continuitynode = config.m_plugindata_xml.child("ContinuityFill");

    m_continuitymin = continuitynode.child("MinimumTime").text().as_int(-1);
	if (-1 == m_continuitymin)
	{
		m_hook.gs->L->warn(config.m_instance, "ContinuityFill MinimumTime not set, assuming 5 seconds");
		m_fileweight = 0;
	}

	// Configure the parent continuity filler event
	m_continuityfill.m_targetdevice = continuitynode.child_value("Device");
	if (m_continuityfill.m_targetdevice.empty())
	{
		m_hook.gs->L->warn(config.m_instance, "ContinuityFill device not set, disabling plugin");
		m_status = FAILED;
		return;
	}

	m_continuityfill.m_eventtype = EVENT_FIXED;

	// Create the "Add" event
	MouseCatcherEvent continuitychild = m_continuityfill;

	continuitychild.m_extradata["graphicname"] = continuitynode.child_value("GraphicName");
    if (continuitychild.m_extradata["graphicname"].empty())
    {
        m_hook.gs->L->warn(config.m_instance, "ContinuityFill graphic name not set, disabling plugin");
        m_status = FAILED;
        return;
    }

    // This is optional, a test is not needed
    continuitychild.m_preprocessor = continuitynode.child_value("PreProcessor");

    continuitychild.m_action_name = "Add";
    continuitychild.m_duration = 1;
    continuitychild.m_extradata["next"] = "ppfill";
    continuitychild.m_extradata["then"] = "ppfill";

    m_continuityfill.m_childevents.push_back(continuitychild);

    // Configure the "Remove" event
    continuitychild.m_action_name = "Remove";
    continuitychild.m_preprocessor = "";
    continuitychild.m_extradata.clear();
    m_continuityfill.m_childevents.push_back(continuitychild);

    m_continuityfill.m_action_name = "Parent";
}

/**
 * Set up an async job to fill the schedule automatically by generating events.
 * Will return a placeholder event with an ID marker, to be populated when the
 * async job completes in populatePlaceholderEvent().
 *
 * @param originalEvent  Event handle to use a base and duration
 * @param resultingEvent Placeholder event to be populated with generated children
 */
void EventProcessor_Fill::handleEvent(MouseCatcherEvent originalEvent,
        MouseCatcherEvent& resultingEvent)
{
    // Sort out durations
    if (originalEvent.m_extradata.count("duration") > 0)
    {
        try
        {
        	originalEvent.m_duration = 0;
        	int lastfind = 0;
        	int i = 0;
        	int found;

        	do
        	{
        		found = originalEvent.m_extradata["duration"].find(':', lastfind);
        		if (std::string::npos == found)
        		{
        			found = originalEvent.m_extradata["duration"].length();
        		}

        		std::string sub = originalEvent.m_extradata["duration"].substr(lastfind, found - lastfind);
        		lastfind = found + 1;

        		originalEvent.m_duration = originalEvent.m_duration * 60 +
        				ConvertType::stringToInt(sub);
        		i++;
        	}
        	while (found != originalEvent.m_extradata["duration"].length() && i < 3);
        }
        catch (std::exception &ex)
        {
            m_hook.gs->L->warn(m_pluginname, "Bad duration of " + originalEvent.m_extradata["duration"] +
                    " selecting 10s instead");
            originalEvent.m_duration = 10;
        }

        // Convert duration from seconds to frames
        originalEvent.m_duration *= g_pbaseconfig->getFramerate();

        originalEvent.m_extradata.clear();
    }
    else if (originalEvent.m_duration <= 0)
    {
        m_hook.gs->L->warn(m_pluginname, "No duration given, selecting 10s instead");
        originalEvent.m_duration = 10 * g_pbaseconfig->getFramerate();
    }

    // Copy the input event onto the output placeholder
    resultingEvent = originalEvent;

    // Add a placeholder ID to identify this event later
    resultingEvent.m_extradata["placeholderID"] = ConvertType::intToString(m_placeholderid);

    // Create a new async job to actually run the processor
    std::shared_ptr<MouseCatcherEvent> pfilledevent = std::make_shared<MouseCatcherEvent>(resultingEvent);

    m_hook.gs->Async->newAsyncJob(
            std::bind(&EventProcessor_Fill::generateFilledEvents, pfilledevent, m_pdb, m_structuredata, m_filler,
                    m_continuityfill, m_continuitymin, g_pbaseconfig->getFramerate(),
                    std::placeholders::_1, std::placeholders::_2),
            std::bind(&EventProcessor_Fill::populatePlaceholderEvent, this, pfilledevent, m_placeholderid,
                    std::placeholders::_1),
            pfilledevent, m_jobpriority, false);

    // Prepare to update the database if needed
    if (m_cyclesremaining < 1)
    {
        m_cyclesremaining = m_cyclesbeforesync;
        m_hook.gs->Async->newAsyncJob(
                std::bind(&EventProcessor_Fill::periodicDatabaseSync, this, std::placeholders::_1,
                        std::placeholders::_2), NULL,
                        NULL, m_jobpriority + 50, false);
    }
    else
    {
        m_cyclesremaining--;
    }
}

/**
 * Add a new file to the database
 *
 * @param filename Name of the file to the device it belongs to
 * @param device   Name of the device that can play the file
 * @param type     To match types in the config file
 * @param duration Length of the new file
 * @param weight   Hard weight to apply to file (to prevent scheduling Checkmate!)
 */
void EventProcessor_Fill::addFile (std::string filename, std::string device, std::string type,
        int duration, int weight)
{
    m_pdb->addFile(filename, device, type, duration, weight);
}

/**
 * Generates a series of events based on configuration file, to fill a chunk
 * of time in the schedule. Uses the Duration field of the original event but
 * devices to operate on and the types to form are based on config file.
 *
 * Designed to run as an async job so takes copies of all the class members it needs
 * to save on a long core lock
 *
 * @param event          Pointer to an event which will be filled with generated data
 * @param db             Pointer to m_pdb
 * @param structuredata  m_structuredata from the calling class
 * @param filler         m_filler from the calling class
 * @param continuityfill m_continuityfill from the calling class
 * @param continuitymin  m_continuitymin from the calling class
 * @param framerate      System frame rate
 * @param data           Unused pointer to async data struct
 * @param core_lock      Reference to core locking mutex
 */
void EventProcessor_Fill::generateFilledEvents (std::shared_ptr<MouseCatcherEvent> event, std::shared_ptr<FillDB> db,
        std::vector<std::pair<std::string, std::string>> structuredata, bool filler, MouseCatcherEvent continuityfill,
        int continuitymin, float framerate, std::shared_ptr<void> data, std::timed_mutex &core_lock)
{
    int duration = event->m_duration;

    // Knock off a few seconds for minimum continuity time
    if (duration > continuitymin)
    {
        duration = duration - continuitymin;
    }
    else
    {
        // Just a continuity fill
        duration = 0;
    }

    // Assemble a template event to populate with filename
    MouseCatcherEvent templateevent;
    templateevent.m_action = 0;
    templateevent.m_channel = event->m_channel;
    templateevent.m_triggertime = event->m_triggertime;
    templateevent.m_eventtype = EVENT_FIXED;

    // Temporary storage for played file data
    std::vector<std::pair<int, int> > playdata;

    int resultduration;

    std::set<int> pastids;
    std::string filename;
    int id;

    // Loop over each type in m_structuredata and generate an event
    for (std::pair<std::string, std::string> thistype : structuredata)
    {
        id = db->getBestFile(filename, event->m_triggertime,
                duration, thistype.second, thistype.first, resultduration, pastids);

        if (id > 0)
        {
            // Add an event for this file
            templateevent.m_extradata["filename"] = filename;
            templateevent.m_duration = resultduration;
            templateevent.m_targetdevice = thistype.second;

            event->m_childevents.push_back(templateevent);

            // Mark the play
            playdata.push_back(std::make_pair(id, templateevent.m_triggertime));

            // Update template triggertime
            templateevent.m_triggertime += static_cast<int>(resultduration / framerate);

            // Reduce remaining duration
            duration -= resultduration;

        }
        else
        {
            resultduration = -1;
        }
    }

    // Now fill the remaining time with whatever type was tagged as filler
    if (filler)
    {
        while (duration > 0)
        {
            id = db->getBestFile(filename, event->m_triggertime,
                    duration, structuredata.back().second,
                    structuredata.back().first, resultduration, pastids);

            if (id > 0)
            {
                // Add an event for this file
                templateevent.m_extradata["filename"] = filename;
                templateevent.m_duration = resultduration;
                templateevent.m_targetdevice = structuredata.front().second;

                event->m_childevents.push_back(templateevent);

                // Mark the play
                playdata.push_back(std::make_pair(id, templateevent.m_triggertime));

                // Update template triggertime
                templateevent.m_triggertime += static_cast<int>(resultduration / framerate);

                // Reduce remaining duration
                duration -= resultduration;

            }
            else
            {
                // No more files, break out and do continuity
                break;
            }
        }
    }

    // This writes to the database, so we should hold the core lock
    {
    	std::lock_guard<std::timed_mutex> lock(core_lock);

		db->beginTransaction();
		for (std::pair<int, int> thisplay : playdata)
		{
			db->addPlay(thisplay.first, thisplay.second);
		}
		db->endTransaction();
    }

    // Generate the continuity event for the rest
    continuityfill.m_channel = event->m_channel;
    continuityfill.m_duration = static_cast<int>((continuitymin + duration) / framerate);
    continuityfill.m_triggertime = templateevent.m_triggertime;

    continuityfill.m_childevents[0].m_extradata["now"] = event->m_description;
    continuityfill.m_childevents[0].m_channel = event->m_channel;
    continuityfill.m_childevents[1].m_channel = event->m_channel;
    continuityfill.m_childevents[0].m_triggertime = templateevent.m_triggertime;
    continuityfill.m_childevents[1].m_triggertime = templateevent.m_triggertime +
            static_cast<int>(duration / framerate);
    event->m_childevents.push_back(continuityfill);
}

/**
 * Adds child events generated by async job to the playlist,
 * attached to the appropriate parent.
 *
 * @param event          Pointer to top level event containing children to add
 * @param placeholder_id ID number inside placeholder event to identify it
 * @param data           Unused data struct
 */
void EventProcessor_Fill::populatePlaceholderEvent (std::shared_ptr<MouseCatcherEvent> event,
        int placeholder_id, std::shared_ptr<void> data)
{
    // Find the events in the playlist matching this time
	int channelid = Channel::getChannelByName(event->m_channel);

    std::vector<PlaylistEntry> eventlist;

    eventlist = g_channels[channelid]->m_pl.getEvents(event->m_eventtype, event->m_triggertime);

    // Look for the correct data tag
    int eventid = -1;

    for (PlaylistEntry &ev : eventlist)
    {
        if (ev.m_extras.count("placeholderID"))
        {
            if (!ev.m_extras["placeholderID"].compare(ConvertType::intToString(m_placeholderid)))
            {
                eventid = ev.m_eventid;
                break;
            }
        }
    }

    // Add each child to the playlist using the normal processEvent() mechanism
    if (eventid > -1)
    {
        EventAction action;

        for (MouseCatcherEvent child : event->m_childevents)
        {
            MouseCatcherCore::processEvent(child, eventid, true, action);
        }
    }
    else
    {
        m_hook.gs->L->error("populatePlaceholderEvent", "Got a non-existent playlist event. Failing silently.");
    }

}

/**
 * Lock the core and sync the database in memory with the one on disk
 *
 * @param data Unused
 */
void EventProcessor_Fill::periodicDatabaseSync (std::shared_ptr<void> data, std::timed_mutex &core_lock)
{
    // Lock is not technically needed, but a precaution against future multithreaded async jobs
    std::lock_guard<std::timed_mutex> lock(core_lock);
    m_pdb->syncDatabase(m_dbfile);
}

/**
 * Preprocessor function to fill in a CG graphic from the schedule
 *
 * @param event    Reference to playlist event that called the processor
 * @param pchannel Pointer to calling channel
 */
void EventProcessor_Fill::populateCGNowNext (PlaylistEntry &event, Channel *pchannel)
{
    // Find the top-level parent event
    int parentid = event.m_eventid;
    int lastid;

    do
    {
        lastid = parentid;
        parentid = pchannel->m_pl.getParentEventID(lastid);
    }
    while (parentid != -1);

    PlaylistEntry parent;
    pchannel->m_pl.getEventDetails(lastid, parent);

    // Find the next event after the end of this one
    std::vector<PlaylistEntry> followlist;
    followlist = pchannel->m_pl.getEventList(
            parent.m_trigger + static_cast<int>(parent.m_duration / g_pbaseconfig->getFramerate()), 5);

    if (!followlist.empty())
    {
        // Populate "next" from the description of next event if required
        if (!event.m_extras["next"].compare("ppfill"))
        {
            event.m_extras["next"] = followlist[0].m_description;
        }

        // Do the same for "then" if required
        if (!event.m_extras["then"].compare("ppfill"))
        {
            int trig = followlist[0].m_trigger;
            int duration = followlist[0].m_duration;
            followlist = pchannel->m_pl.getEventList(
                    trig + static_cast<int>(duration / g_pbaseconfig->getFramerate()), 5);

            if (!followlist.empty())
            {
                event.m_extras["then"] = followlist[0].m_description;
            }
        }
    }
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        std::shared_ptr<EventProcessor_Fill> plugtemp = std::make_shared<EventProcessor_Fill>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}
