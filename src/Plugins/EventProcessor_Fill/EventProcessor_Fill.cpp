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

#include "pugixml.hpp"
#include "EventProcessor_Fill.h"
#include "PluginConfig.h"

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
		oneTimeExec("ATTACH " + databasefile + " AS disk");

		oneTimeExec("BEGIN TRANSACTION");

		oneTimeExec("INSERT INTO items (id, name, device, type, duration, weight) "
				"SELECT id, name, device, type, duration, weight FROM disk.items");
		oneTimeExec("INSERT INTO plays (id, itemid, timestamp) "
				"SELECT id, itemid, timestamp FROM disk.plays");
		oneTimeExec("DETACH disk");

		oneTimeExec("END TRANSACTION");
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
 * Mark a set of played files into the database
 *
 * @param playdata Vector of file IDs and times
 */
void FillDB::addPlayData(std::vector<std::pair<int, int>>& playdata)
{
    oneTimeExec("BEGIN TRANSACTION;");
    for (std::pair<int, int> thisdata : playdata)
    {
        m_paddplay_query->rmParams();

        m_paddplay_query->addParam(1, thisdata.first);
        m_paddplay_query->addParam(2, thisdata.second);

        m_paddplay_query->bindParams();
        sqlite3_step(m_paddplay_query->getStmt());
    }
    oneTimeExec("END TRANSACTION;");
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
 * @todo Neaten this up to account for deleted rows on disk.
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
 * Get the lowest-weighted file shorter than a duration for the specified device
 *
 * @param filename		 Name of the file found
 * @param inserttime     The time at which the file will be inserted
 * @param duration       Maximum acceptable duration of file.
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
EventProcessor_Fill::EventProcessor_Fill(PluginConfig config, Hook h) :
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

    m_status = READY;

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
    	m_hook.gs->L->error(config.m_instance, "No database file set in config."
    			" Aborting.");
    	m_status = FAILED;
    	return;
    }

    m_fileweight = config.m_plugindata_xml.child("FileWeight").text().as_int(-1);
    if (-1 == m_fileweight)
    {
    	m_hook.gs->L->warn(config.m_instance, "FileWeight factor not set, "
    			"assuming 0");
    	m_fileweight = 0;
    }

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
		m_hook.gs->L->warn(config.m_instance, "ContinuityFill MinimumTime not "
				"set, assuming 5 seconds");
		m_fileweight = 0;
	}

	m_continuityfill.m_action = continuitynode.child("EventAction").text().as_int(-1);
	if (-1 == m_continuityfill.m_action)
	{
		m_hook.gs->L->warn(config.m_instance, "ContinuityFill EventAction not "
				"set, assuming 0");
		m_continuityfill.m_action = 0;
	}

	std::string eventtype = continuitynode.child_value("EventType");
	if (!eventtype.compare("Fixed"))
	{
		m_continuityfill.m_eventtype = EVENT_FIXED;
	}
	else if (!eventtype.compare("Child"))
	{
		m_continuityfill.m_eventtype = EVENT_CHILD;
	}
	else if (!eventtype.compare("Manual"))
	{
		m_continuityfill.m_eventtype = EVENT_MANUAL;
	}
	else
	{
		m_hook.gs->L->warn(config.m_instance, "ContinuityFill EventType not "
				"set, assuming fixed");
		m_continuityfill.m_eventtype = EVENT_FIXED;
	}

	for (pugi::xml_node node : continuitynode.child("EventData").children())
	{
		m_continuityfill.m_extradata[node.name()] = node.child_value();
	}

}

/**
 * Generates a series of events based on configuration file, to fill a chunk
 * of time in the schedule. Uses the Duration field of the originalEvent but
 * devices to operate on and the types to form are based on config file.
 *
 * @param originalEvent  Event handle to use a base and duration
 * @param resultingEvent A number of idents/trailers, followed by a CG event
 */
void EventProcessor_Fill::handleEvent(MouseCatcherEvent originalEvent,
        MouseCatcherEvent& resultingEvent)
{
    int duration = originalEvent.m_duration;

    // Knock off a few seconds for minimum continuity time
    duration = duration - m_continuitymin;

    // Assemble a template event to populate with filename
    MouseCatcherEvent templateevent;
    templateevent.m_action = 1;
    templateevent.m_channel = originalEvent.m_channel;
    templateevent.m_triggertime = originalEvent.m_triggertime;
    templateevent.m_eventtype = EVENT_FIXED;

    // Temporary storage for played file data
    std::vector<std::pair<int, int> > playdata;

    int resultduration;

    std::set<int> pastids;
    std::string filename;
    int id;

    // Loop over each type in m_structuredata and generate an event
    for (std::pair<std::string, std::string> thistype : m_structuredata)
    {
        id = m_pdb->getBestFile(filename, originalEvent.m_triggertime,
                duration, thistype.second, thistype.first, resultduration, pastids);

        if (id > 0)
        {
            // Add an event for this file
            templateevent.m_extradata["filename"] = filename;
            templateevent.m_duration = resultduration;
            templateevent.m_targetdevice = thistype.second;

            resultingEvent.m_childevents.push_back(templateevent);

            // Mark the play
            playdata.push_back(std::make_pair(id, templateevent.m_triggertime));

            // Update template triggertime
            templateevent.m_triggertime += resultduration;

            // Reduce remaining duration
            duration -= resultduration;

        }
        else
        {
            resultduration = -1;
        }
    }

    // Now fill the remaining time with whatever type was tagged as filler
    if (m_filler)
    {
        while (duration > 0)
        {
            id = m_pdb->getBestFile(filename, originalEvent.m_triggertime,
            		duration, m_structuredata.front().second,
            		m_structuredata.front().first, resultduration, pastids);

            if (id > 0)
            {
                // Add an event for this file
                templateevent.m_extradata["m_filename"] = filename;
                templateevent.m_duration = resultduration;
                templateevent.m_targetdevice = m_structuredata.front().second;

                resultingEvent.m_childevents.push_back(templateevent);

                // Mark the play
                playdata.push_back(std::make_pair(id, templateevent.m_triggertime));

                // Update template triggertime
                templateevent.m_triggertime += resultduration;

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

    m_pdb->addPlayData(playdata);

    // Generate the continuity event for the rest (+5 seconds)
    resultingEvent.m_childevents.push_back(m_continuityfill);
    resultingEvent.m_childevents.back().m_duration = m_continuitymin + duration;

}

void EventProcessor_Fill::addFile (std::string filename, std::string device, std::string type,
        int duration, int weight)
{
    m_pdb->addFile(filename, device, type, duration, weight);
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new EventProcessor_Fill(config, h);
    }
}
