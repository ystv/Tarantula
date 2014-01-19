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
*   File Name   : Caspar_VideoDevice.h
*   Version     : 1.0
*   Description : Caspar supports both CG and media. This plugin works with
*                 Media.
*
*****************************************************************************/


#pragma once

#include <cstring>
#include <sstream>
#include <cstdlib>
#include <queue>
#include <pthread.h>
#include <mutex>

#include "libCaspar/libCaspar.h"
#include "VideoDevice.h"
#include "PluginConfig.h"
#include "MemDB.h"

class threadCom
{
public:
    CasparConnection *m_pcaspcon;
    std::queue<CasparCommand> *m_pcommandqueue;
    std::queue<std::vector<std::string>> *m_presponsequeue;
    pthread_mutex_t *m_pqueuemutex;
    Log *m_plogger;
    plugin_status_t *m_pstatus;
};

/**
 * A disk database for persistent file list storage.
 */
class CasparFileList : public MemDB
{
public:
    CasparFileList (std::string database, std::string m_table);

    void readFileList (std::map<std::string, VideoFile> &filelist);
    void updateFileList (std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
        std::shared_ptr<std::vector<std::string>> deletedfiles, std::timed_mutex &core_lock);

private:
    std::shared_ptr<DBQuery> m_pgetfilelist_query;
    std::shared_ptr<DBQuery> m_pinsertfile_query;
    std::string m_table;
    std::mutex m_list_lock;
};

/**
 * Caspar supports both CG and media. This plugin works with Media.
 *
 * @param config Configuration data for this plugin
 * @param h      Link back to the GlobalStuff structures
 */
class VideoDevice_Caspar: public VideoDevice
{
public:
    VideoDevice_Caspar (PluginConfig config, Hook h);
    virtual ~VideoDevice_Caspar ();

    void updateHardwareStatus ();
    void getFiles ();
    void immediatePlay (std::string clip);
    void cue (std::string clip);
    void play ();
    void stop ();
    virtual void poll ();
private:
    std::shared_ptr<CasparConnection> m_pcaspcon;

    //! Configured hostname and port number
    std::string m_hostname;
    std::string m_port;

    //! Database file name
    std::shared_ptr<CasparFileList> m_pfiledb;

    void cb_info (std::vector<std::string>& resp);

    // Static functions for async files update job
    static void fileUpdateJob (std::shared_ptr<VideoDevice_Caspar> thisdev,
            std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
            std::shared_ptr<std::vector<std::string>> deletedfiles, std::shared_ptr<void> data,
            std::timed_mutex &core_lock, std::string hostname, std::string port,
            std::shared_ptr<std::vector<std::string>> transformed_files,
            std::shared_ptr<CasparFileList> pfiledb);
    static void fileUpdateComplete (std::shared_ptr<VideoDevice_Caspar> thisdev,
            std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
            std::shared_ptr<std::vector<std::string>> deletedfiles, std::shared_ptr<void> data);
    static void batchFileLengths (std::shared_ptr<VideoDevice_Caspar> thisdev,
            std::vector<std::string>& medialist, std::shared_ptr<CasparConnection> pccon,
            std::shared_ptr<std::map<std::string, VideoFile>> newfiles);

    static void cb_updatefiles (std::shared_ptr<VideoDevice_Caspar> thisdev,
            std::vector<std::string>& resp, std::shared_ptr<CasparConnection> pccon,
            std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
            std::shared_ptr<std::vector<std::string>> deletedfiles,
            std::shared_ptr<std::vector<std::string>> transformed_files);
    static void cb_updatelength (std::shared_ptr<VideoDevice_Caspar> thisdev,
            std::vector<std::string>& medialist, std::vector<std::string>& resp,
            std::shared_ptr<CasparConnection> pccon, std::shared_ptr<std::map<std::string, VideoFile>> newfiles);
};

