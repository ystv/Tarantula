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
*   Description : Definition of an EventProcessor to generate events for
*   idents, trailers and continuity graphics to fill schedules. Also contains
*   a FillDB class to extend SQLiteDB and provide a database backend.
*
*****************************************************************************/

#pragma once

#define INVALID_QUERY_POINTER -1

#include <map>
#include <set>
#include <mutex>

#include "MouseCatcherProcessorPlugin.h"
#include "PluginConfig.h"
#include "SQLiteDB.h"

class FillDB : public SQLiteDB
{
public:
    FillDB(std::string databasefile, std::map<int, int>& weightpoints,
            int fileweight);

    void updateWeightPoints (std::map<int, int>& weightpoints, int fileweight);
    void addPlay (int idnumber, int time);
    void addFile (std::string filename, std::string device, std::string type,
            int duration, int weight);
    int getBestFile (std::string& filename, int inserttime, int duration,
    		std::string device, std::string type, int& resultduration, std::string& description,
    		std::string& excludeid);

    void beginTransaction ();
    void endTransaction ();


private:
    std::shared_ptr<DBQuery> m_pgetbestfile_query;
    std::shared_ptr<DBQuery> m_paddplay_query;
    std::shared_ptr<DBQuery> m_paddfile_query;
};

/**
 * EventProcessor to automatically fill time with idents, trailers and
 * continuity blocks.
 */
class EventProcessor_Fill : public MouseCatcherProcessorPlugin {
public:
    EventProcessor_Fill (PluginConfig config, Hook h);
    ~EventProcessor_Fill ();
    void readConfig (PluginConfig config);
    void handleEvent (MouseCatcherEvent originalEvent,
            MouseCatcherEvent& resultingEvent);
    void addFile (std::string filename, std::string device, std::string type,
            int duration, int weight);

    static void populateCGNowNext (PlaylistEntry& event, Channel *pchannel);

    static void singleShotMode (PlaylistEntry &event, Channel *pchannel,
            std::vector<std::pair<std::string, std::string>> structuredata, bool filler,
            MouseCatcherEvent continuityfill, int continuitymin, int offset,
            int jobpriority, std::shared_ptr<FillDB> pdb, std::string pluginname);

    std::shared_ptr<FillDB> m_pdb;
private:
    static void generateFilledEvents (std::shared_ptr<MouseCatcherEvent> event, std::shared_ptr<FillDB> db,
            std::vector<std::pair<std::string, std::string>> structuredata, bool singleshot, bool gencontinuity,
            MouseCatcherEvent continuityfill, int continuitymin, float framerate, std::shared_ptr<void> data,
            std::timed_mutex &core_lock, int offset, std::string pluginname);
    static void populatePlaceholderEvent (std::shared_ptr<MouseCatcherEvent> event, int placeholder_id,
            std::shared_ptr<void> data);

    // Data from configuration file
    std::string m_dbfile;
    std::map<int, int> m_weightpoints;
    int m_fileweight;
    int m_jobpriority;

    std::vector<std::pair<std::string, std::string>> m_structuredata; ///< An example could be {"ident", "device1"}
    bool m_filler; ///< Whether to fill remaining time with the last item.
    bool m_singleshot; ///< Should events be generated in bulk or on-the-fly?
    int m_offset; ///< Should each trigger time be offset?

    MouseCatcherEvent m_continuityfill; ///< Event to tack on the end to fill remaining time
    int m_continuitymin; ///< Minimum length for continuity fill

    int m_placeholderid; //!< Next ID of a placeholder event
};
