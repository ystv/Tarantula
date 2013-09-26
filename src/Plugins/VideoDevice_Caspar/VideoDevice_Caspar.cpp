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
*   File Name   : Caspar_VideoDevice.cpp
*   Version     : 1.0
*   Description : Caspar supports both CG and media. This plugin works with
*                 Media.
*
*****************************************************************************/


#include "VideoDevice_Caspar.h"
#include "Misc.h"
#include "PluginConfig.h"
#include <unistd.h>
#include <mutex>

/* File list disk database interface */

/**
 * Open a disk-side database for file listing
 *
 * @param database Path to core SQLite database file
 * @param instance Name of this plugin instance, used to name tables
 */
CasparFileList::CasparFileList (std::string database, std::string table) :
    MemDB(database.c_str())
{
    oneTimeExec("CREATE TABLE IF NOT EXISTS [" + table + "_files] (filename TEXT, path TEXT, duration INT64)");
    m_pgetfilelist_query = prepare("SELECT filename, path, duration FROM [" + table + "_files]");
    m_pinsertfile_query = prepare("INSERT INTO [" + table + "_files] (filename, path, duration) VALUES (?, ?, ?)");
    m_table = table;
}


/**
 * Read the list of files from the database
 * @param filelist
 */
void CasparFileList::readFileList (std::map<std::string, VideoFile>& filelist)
{
    m_pgetfilelist_query->rmParams();
    m_pgetfilelist_query->bindParams();

    sqlite3_stmt *liststmt = m_pgetfilelist_query->getStmt();

    while (SQLITE_ROW == sqlite3_step(liststmt))
    {
        VideoFile thisfile;
        thisfile.m_filename = reinterpret_cast<const char*>(sqlite3_column_text(liststmt, 0));
        thisfile.m_path = reinterpret_cast<const char*>(sqlite3_column_text(liststmt, 1));
        thisfile.m_duration = sqlite3_column_int64(liststmt, 2);

        filelist[thisfile.m_filename] = thisfile;
    }
}

void CasparFileList::updateFileList (
        std::shared_ptr<std::map<std::string, VideoFile> > newfiles,
        std::shared_ptr<std::vector<std::string> > deletedfiles,
        std::timed_mutex &core_lock)
{
    std::string deletedquery;
    std::string addquery;

    if (deletedfiles->size() > 0)
    {
        deletedquery = "DELETE FROM [" + m_table + "_files] "
                "WHERE filename IN ('" + deletedfiles->at(0) + "'";

        for (unsigned int i = 1; i < deletedfiles->size(); ++i)
        {
            deletedquery += ", '" + deletedfiles->at(i) + "'";
        }

        deletedquery += ");";
    }

    // Grab the lock for this bit as this function is async
    std::lock_guard<std::timed_mutex> lock(core_lock);

    oneTimeExec("BEGIN TRANSACTION;");

    if (!deletedquery.empty())
    {
        oneTimeExec(deletedquery);
    }

    if (newfiles->size() > 0)
    {
        for (auto thisfile : *newfiles)
        {
            m_pinsertfile_query->rmParams();
            m_pinsertfile_query->addParam(1, DBParam(thisfile.first));
            m_pinsertfile_query->addParam(2, DBParam(thisfile.second.m_path));
            m_pinsertfile_query->addParam(3, DBParam(static_cast<long long>(thisfile.second.m_duration)));
            m_pinsertfile_query->bindParams();
            sqlite3_step(m_pinsertfile_query->getStmt());
        }
    }

    oneTimeExec("END TRANSACTION;");
}

/**
 * Create a CasparCG Video Plugin
 *
 * @param config Configuration data for this plugin
 * @param h      A link back to the GlobalStuff structures
 */
VideoDevice_Caspar::VideoDevice_Caspar (PluginConfig config, Hook h) :
        VideoDevice(config, h)
{
    // Pull server name and port from configuration data
    long int connecttimeout = 10;
    try
    {
        m_hostname = config.m_plugindata_map.at("Host");
        m_port = config.m_plugindata_map.at("Port");

        connecttimeout = ConvertType::stringToInt(config.m_plugindata_map.at("ConnectTimeout"));
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname + ERROR_LOC, "Configuration data invalid or not supplied");
        m_status = FAILED;
        return;
    }

    h.gs->L->info("Caspar Media", "Connecting to " + m_hostname + ":" + m_port);

    try
    {
        m_pcaspcon = std::make_shared<CasparConnection>(m_hostname, m_port, connecttimeout);
    }
    catch (...)
    {
        m_hook.gs->L->error(m_pluginname, "Unable to connect to CasparCG server"
                " at " + m_hostname);
        m_status = FAILED;
        return;
    }

    CasparFileList listb("test", "test");
    m_pfiledb = std::shared_ptr<CasparFileList>(new CasparFileList(g_pbaseconfig->getOfflineDatabasePath(), m_pluginname));

    m_pfiledb->readFileList(m_files);

    m_status = WAITING;
}

VideoDevice_Caspar::~VideoDevice_Caspar ()
{
}

/**
 * Read and process the responseQueue
 */
void VideoDevice_Caspar::poll ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for poll()");
        return;
    }

    if (!(m_pcaspcon->tick()) || (m_pcaspcon->m_errorflag))
    {
        m_status = CRASHED;
        m_hook.gs->L->error(m_pluginname, "CasparCG connection not active");
        m_pcaspcon->m_errorflag = false;
    }

    if (m_pcaspcon->m_badcommandflag)
    {
        m_hook.gs->L->error(m_pluginname, "CasparCG command error");
        m_pcaspcon->m_badcommandflag = false;
    }

    VideoDevice::poll();
}

/**
 * Send a command to retrieve CasparCG status
 */
void VideoDevice_Caspar::updateHardwareStatus ()
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateHardwareStatus");
        return;
    }

    CasparCommand statuscom(CASPAR_COMMAND_INFO, boost::bind(&VideoDevice_Caspar::cb_info, this, _1));
    statuscom.addParam("1");
    statuscom.addParam("1");
    m_pcaspcon->sendCommand(statuscom);
}

/**
 * Get a list of files available to play
 */
void VideoDevice_Caspar::getFiles ()
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for getFiles");
        return;
    }

    // Get a list of current filenames in a vector
    std::shared_ptr<std::vector<std::string>> transformed_files = std::make_shared<std::vector<std::string>>();

    std::transform(m_files.begin(), m_files.end(), std::back_inserter(*transformed_files),
            [] (const std::pair<std::string, VideoFile> &item) { return item.first; });

    // Schedule async update job
    std::shared_ptr<std::map<std::string, VideoFile>> pnewfiles = std::make_shared<std::map<std::string, VideoFile>>();
    std::shared_ptr<std::vector<std::string>> pdeletedfiles = std::make_shared<std::vector<std::string>>();

    std::shared_ptr<VideoDevice_Caspar> pthisdev =
            std::dynamic_pointer_cast<VideoDevice_Caspar>(m_hook.gs->Devices->at(m_pluginname));

    m_hook.gs->Async->newAsyncJob(
            std::bind(&VideoDevice_Caspar::fileUpdateJob, pthisdev, pnewfiles, pdeletedfiles,
                    std::placeholders::_1, std::placeholders::_2, m_hostname, m_port, transformed_files,
                    m_pfiledb),
            std::bind(&VideoDevice_Caspar::fileUpdateComplete, pthisdev, pnewfiles, pdeletedfiles,
                    std::placeholders::_1),
            NULL, 10, false);
}

/**
 * Load a video ready for playback
 *
 * @param clip The video to play
 */
void VideoDevice_Caspar::cue (std::string clip)
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for cue");
        return;
    }

    //Update internal status
    m_videostatus.filename = clip;

    CasparCommand cc1(CASPAR_COMMAND_LOADBG);
    cc1.addParam("1");
    cc1.addParam("1");
    cc1.addParam(clip);
    m_pcaspcon->sendCommand(cc1);
}

/**
 * Play a loaded video
 */
void VideoDevice_Caspar::play ()
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for play");
        return;
    }

    //Get filename and update status
    m_videostatus.remainingframes = m_files[m_videostatus.filename].m_duration;
    m_videostatus.state = VDS_PLAYING;

    CasparCommand cc1(CASPAR_COMMAND_PLAY);
    cc1.addParam("1");
    cc1.addParam("1");
    m_pcaspcon->sendCommand(cc1);

}

/**
 * Stop this layer
 */
void VideoDevice_Caspar::stop ()
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for stop");
        return;
    }

    CasparCommand cc1(CASPAR_COMMAND_STOP);
    cc1.addParam("1");
    cc1.addParam("1");
    m_pcaspcon->sendCommand(cc1);
}

/**
 * Asynchronous task to kickoff a new CG connection and update the file list
 *
 * @param thisdev           Pointer to the calling class, shared_ptr variant of this keyword
 * @param newfiles          Pointer to a map of new media files
 * @param deletedfiles      Pointer to a vector of deleted file names
 * @param data              Unused.
 * @param core_lock         Mutex to lock exclusive access to Tarantula core.
 * @param hostname          CasparCG server hostname
 * @param port              CasparCG server port
 * @param transformed_files File list in vector form
 */
void VideoDevice_Caspar::fileUpdateJob (std::shared_ptr<VideoDevice_Caspar> thisdev,
        std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
        std::shared_ptr<std::vector<std::string>> deletedfiles, std::shared_ptr<void> data,
        std::timed_mutex &core_lock, std::string hostname, std::string port,
        std::shared_ptr<std::vector<std::string>> transformed_files,
        std::shared_ptr<CasparFileList> pfiledb)
{
    // Start a new connection to the server
    std::shared_ptr<CasparConnection> pccon;
    try
    {
        pccon = std::make_shared<CasparConnection>(hostname, port, 10);
    }
    catch (...)
    {
        return;
    }

    // Send the query
    CasparCommand query(CASPAR_COMMAND_CLS, boost::bind(&VideoDevice_Caspar::cb_updatefiles, thisdev, _1, pccon,
            newfiles, deletedfiles, transformed_files));
    pccon->sendCommand(query);

    pccon->run(-1);

    if (newfiles->size() > 0 || deletedfiles->size() > 0)
    {
        pfiledb->updateFileList(newfiles, deletedfiles, core_lock);
    }
}

/**
 * File update job completion callback - updates the file list with results from the update
 *
 * @param thisdev           Pointer to the calling class, shared_ptr variant of this keyword
 * @param newfiles     Map of new files to be inserted into the map
 * @param deletedfiles Names of deleted files to be removed
 * @param data         Unused.
 */
void VideoDevice_Caspar::fileUpdateComplete(std::shared_ptr<VideoDevice_Caspar> thisdev,
        std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
  std::shared_ptr<std::vector<std::string>> deletedfiles, std::shared_ptr<void> data)
{
    if (READY != thisdev->m_status && WAITING != thisdev->m_status)
    {
        return;
    }

    for (std::string thisfile : *deletedfiles)
    {
        thisdev->m_files.erase(thisfile);
    }

    for (std::pair<std::string, VideoFile> thisfile : *newfiles)
    {
        thisdev->m_files[thisfile.first] = thisfile.second;
    }

    thisdev->m_hook.gs->L->info("Caspar File Update", "Got " + ConvertType::intToString(newfiles->size()) +
            " additions and " + ConvertType::intToString(deletedfiles->size()) + " deletions.");
}

/**
 * Batch updating the list of file lengths to grab 10 at a time
 *
 * @param thisdev           Pointer to the calling class, shared_ptr variant of this keyword
 * @param medialist List of files from a cb_updatefiles call
 * @param pccon     CasparCG connection to run this async
 */
void VideoDevice_Caspar::batchFileLengths (std::shared_ptr<VideoDevice_Caspar> thisdev,
        std::vector<std::string>& medialist, std::shared_ptr<CasparConnection> pccon,
        std::shared_ptr<std::map<std::string, VideoFile>> newfiles)
{
    std::vector<std::string>::iterator iter = medialist.end();

    iter--;

    for (int i = 0; i < 10; i++)
    {
        CasparCommand durationquery(CASPAR_COMMAND_LOADBG);
        durationquery.addParam("1");
        durationquery.addParam(ConvertType::intToString(i + 10));
        durationquery.addParam(*iter);
        pccon->sendCommand(durationquery);

        if (iter != medialist.begin())
        {
            --iter;
        }
        else
        {
            break;
        }
    }

    CasparCommand infoquery(CASPAR_COMMAND_INFO_EXPANDED, std::bind(&VideoDevice_Caspar::cb_updatelength,
            thisdev, medialist, std::placeholders::_1, pccon, newfiles));
    infoquery.addParam("1");
    pccon->sendCommand(infoquery);


}

/**
 * Callback to handle files update
 *
 * @param thisdev           Pointer to the calling class, shared_ptr variant of this keyword
 * @param resp              Vector filled with lines from CasparCG
 * @param pccon             Pointer to new CG connection
 * @param newfiles          Pointer to a map of new media files
 * @param deletedfiles      Pointer to a vector of deleted file names
 * @param transformed_files Current file list in vector form
 */
void VideoDevice_Caspar::cb_updatefiles (std::shared_ptr<VideoDevice_Caspar> thisdev,
        std::vector<std::string>& resp, std::shared_ptr<CasparConnection> pccon,
        std::shared_ptr<std::map<std::string, VideoFile>> newfiles,
                std::shared_ptr<std::vector<std::string>> deletedfiles,
                std::shared_ptr<std::vector<std::string>> transformed_files)
{
    std::vector<std::string> medialist;

    // Call the processor
    CasparQueryResponseProcessor::getMediaList(resp, medialist);

    // Identify the files to remove from the main
    std::sort(medialist.begin(), medialist.end());

    std::vector<std::string> newmedialist;

    std::set_difference(medialist.begin(), medialist.end(), transformed_files->begin(), transformed_files->end(),
            std::inserter(newmedialist, newmedialist.begin()));

    std::set_difference(transformed_files->begin(), transformed_files->end(), medialist.begin(), medialist.end(),
            std::inserter(*deletedfiles, deletedfiles->begin()));

    // Get lengths for all the new files if needed

    if (newmedialist.size() > 0)
    {
        batchFileLengths(thisdev, newmedialist, pccon, newfiles);
    }
}

/**
 * Callback for file length in frames, will push file back in to list
 *
 * @param thisdev           Pointer to the calling class, shared_ptr variant of this keyword
 * @param filedata      Rest of file data to go in list
 * @param resp          Lines of data from CasparCG
 * @param pccon         Pointer to CG connection
 * @param newfiles      Pointer to a map of new media files
 */
void VideoDevice_Caspar::cb_updatelength (std::shared_ptr<VideoDevice_Caspar> thisdev,
        std::vector<std::string>& medialist, std::vector<std::string>& resp,
        std::shared_ptr<CasparConnection> pccon, std::shared_ptr<std::map<std::string, VideoFile>> newfiles)
{
    std::vector<std::string>::iterator iter = medialist.end();

    iter--;

    int i;
    for (i = 0; i < 10; ++i)
    {
        VideoFile filedata;
        filedata.m_filename = *iter;
        filedata.m_title = *iter;
        filedata.m_duration = CasparQueryResponseProcessor::readFileFrames(resp, i + 10);
        newfiles->insert(std::make_pair(filedata.m_filename, filedata));

        if (iter != medialist.begin())
        {
            --iter;
        }
        else
        {
            i++;
            break;
        }
    }

    medialist.erase(medialist.end() - i, medialist.end());

    if (medialist.size() > 0)
    {
        batchFileLengths(thisdev, medialist, pccon, newfiles);
    }
    else
    {

        for (int i = 0; i < 10; ++i)
        {
            CasparCommand clearquery(CASPAR_COMMAND_CLEAR);
            clearquery.addParam("1");
            clearquery.addParam(ConvertType::intToString(i + 10));
            pccon->sendCommand(clearquery);
        }
    }
}

/**
 * Callback for info commands
 *
 * @param resp  Lines of data from CasparCG
 */
void VideoDevice_Caspar::cb_info (std::vector<std::string>& resp)
{
    // Detect response type and process appropriately
    if (resp[0] == "201 INFO OK")
    {
        std::string filename;
        int frames = CasparQueryResponseProcessor::readLayerStatus(resp, filename);
        if (frames <= 0)
            m_videostatus.state = VDS_STOPPED;
        else
        {
            m_videostatus.state = VDS_PLAYING;
            m_videostatus.remainingframes = frames;
            m_hook.gs->L->info("Caspar Media", "Got back a filename of "
                    + filename + " with " + ConvertType::intToString(frames)
                    + " frames left");
        }
    }
}


extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<VideoDevice_Caspar> plugtemp = std::make_shared<VideoDevice_Caspar>(config, h);

        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}
