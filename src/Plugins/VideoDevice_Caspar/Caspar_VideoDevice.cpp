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


#include "Caspar_VideoDevice.h"
#include "Misc.h"
#include "PluginConfig.h"
#include <unistd.h>

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
    m_hostname = config.m_plugindata_map.at("Host");
    m_port = ConvertType::stringToInt(config.m_plugindata_map.at("Port"));

    h.gs->L->info("Caspar Media",
            "Connecting to " + m_hostname + ":" + ConvertType::intToString(m_port));

    try
    {
        m_pcaspcon = new CasparConnection(m_hostname);
    }
    catch (...)
    {
        m_hook.gs->L->error(m_pluginname, "Unable to connect to CasparCG server"
                " at " + m_hostname);
        m_status = FAILED;
        return;
    }


    // Setup the thread to use the connection
    pthread_mutex_init(&m_queuemutex, NULL);
    threadCom *ptc = new threadCom;
    ptc->m_pcaspcon = m_pcaspcon;

    ptc->m_pcommandqueue = &m_commandqueue;
    ptc->m_presponsequeue = &m_responsequeue;
    ptc->m_pqueuemutex = &m_queuemutex;
    ptc->m_plogger = h.gs->L;
    ptc->m_pstatus = &m_status;

    pthread_create(&m_netthread, NULL, NetworkThread, static_cast<void*> (ptc));
    getFiles();
}

VideoDevice_Caspar::~VideoDevice_Caspar ()
{
    if (m_pcaspcon != NULL)
        delete m_pcaspcon;
}

/**
 * Read and process the responseQueue
 */
void VideoDevice_Caspar::poll ()
{
    while (!m_responsequeue.empty())
    {
        pthread_mutex_lock(&m_queuemutex);
        std::vector<std::string> resp = m_responsequeue.front();
        m_responsequeue.pop();
        pthread_mutex_unlock(&m_queuemutex);

        // Detect response type and process appropriately
        if (resp[1] == "201 INFO OK")
        {
            std::string filename;
            int frames = CasparQueryResponseProcessor::readLayerStatus(resp,
                    filename);
            if (frames <= 0)
                m_videostatus.state = VDS_STOPPED;
            else
            {
                m_videostatus.state = VDS_PLAYING;
                m_videostatus.remainingframes = frames;
                m_hook.gs->L->info("Caspar Media",
                        "Got back a filename of " + filename + " with "
                                + ConvertType::intToString(frames)
                                + " frames left");
            }
        }

    }
    VideoDevice::poll();
}

/**
 * Send a command to retrieve CasparCG status
 */
void VideoDevice_Caspar::updateHardwareStatus ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateHardwareStatus");
        return;
    }

    pthread_mutex_lock(&m_queuemutex);
    CasparCommand statuscom(CASPAR_COMMAND_INFO);
    statuscom.addParam("1");
    statuscom.addParam("1");
    m_commandqueue.push(statuscom);
    pthread_mutex_unlock(&m_queuemutex);
}

/**
 * Get a list of files available to play
 * Blocks for now until it gets a response
 */
void VideoDevice_Caspar::getFiles ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for getFiles");
        return;
    }

    CasparCommand query(CASPAR_COMMAND_CLS);
    std::vector<std::string> resp;
    m_pcaspcon->sendCommand(query, resp);
    std::map<std::string, long> medialist;

    //Call the processor
    CasparQueryResponseProcessor::getMediaList(resp, medialist);

    //Iterate over media list adding to files list
    for (std::pair<std::string, long> item : medialist)
    {
        VideoFile thisfile;
        thisfile.m_filename = item.first;
        thisfile.m_duration = item.second;
        thisfile.m_title = item.first;
        m_files[thisfile.m_filename] = thisfile;
    }
}

/**
 * Load a video ready for playback
 *
 * @param clip The video to play
 */
void VideoDevice_Caspar::cue (std::string clip)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for cue");
        return;
    }

    //Update internal status
    m_videostatus.filename = clip;
    pthread_mutex_lock(&m_queuemutex);
    CasparCommand cc1(CASPAR_COMMAND_LOADBG);
    cc1.addParam("1");
    cc1.addParam("1");
    cc1.addParam(clip);
    m_commandqueue.push(cc1);
    pthread_mutex_unlock(&m_queuemutex);
}

/**
 * Play a loaded video
 */
void VideoDevice_Caspar::play ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for play");
        return;
    }

    //Get filename and update status
    m_videostatus.remainingframes = m_files[m_videostatus.filename].m_duration;
    m_videostatus.state = VDS_PLAYING;

    pthread_mutex_lock(&m_queuemutex);
    CasparCommand cc1(CASPAR_COMMAND_PLAY);
    cc1.addParam("1");
    cc1.addParam("1");
    m_commandqueue.push(cc1);
    pthread_mutex_unlock(&m_queuemutex);

}

/**
 * Stop this layer
 */
void VideoDevice_Caspar::stop ()
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for stop");
        return;
    }

    pthread_mutex_lock(&m_queuemutex);
    CasparCommand cc1(CASPAR_COMMAND_STOP);
    cc1.addParam("1");
    cc1.addParam("1");
    m_commandqueue.push(cc1);
    pthread_mutex_unlock(&m_queuemutex);
}

#include <cstdio>

/**
 * A non-blocking way to handle command and response queues
 * Designed to be spawned as a thread with pthread_create
 *
 * @param ptr void* Pointer to the class' threadCom object.
 */
void *NetworkThread (void *ptr)
{
    threadCom *ptc;
    ptc = static_cast<threadCom*> (ptr);

    // Extract from threadCom to make it a touch easier
    CasparConnection *pcaspcon = ptc->m_pcaspcon;
    std::queue<CasparCommand> *pcommandqueue = ptc->m_pcommandqueue;
    std::queue<std::vector<std::string>> *presponsequeue = ptc->m_presponsequeue;
    pthread_mutex_t *pqueuemutex = ptc->m_pqueuemutex;
    Log *plogger = ptc->m_plogger;
    plugin_status_t *pstatus = ptc->m_pstatus;

    while (1)
    {
        // Get the entry and pop it in one move to avoid locking the mutex for too long
        pthread_mutex_lock(pqueuemutex);
        if (pcommandqueue->empty())
        {
            // Unlock the mutex and try again soon.
            pthread_mutex_unlock(pqueuemutex);
            usleep(1000);
            continue;
        }

        CasparCommand toProc = pcommandqueue->front();
        pcommandqueue->pop();
        pthread_mutex_unlock(pqueuemutex);

        // Send the command and collect the response
        std::vector<std::string> resp;
        try
        {
            pcaspcon->sendCommand(toProc, resp);
        }
        catch (...)
        {
            plogger->error("Caspar VideoDevce", "Error sending a command");
            *pstatus = CRASHED;
            break;
        }

        if (4 == resp[1].c_str()[0])
        {
            plogger->error("Caspar Media", "Got Error: " + resp[0]);
        }
        else
        {
            plogger->info("Caspar Media", "Got Command Status: " + resp[0]);
        }

        // Put the response onto the queue
        pthread_mutex_lock(pqueuemutex);
        presponsequeue->push(resp);
        pthread_mutex_unlock(pqueuemutex);
        usleep(1000);
    }
    return 0;
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new VideoDevice_Caspar(config, h);
    }
}
