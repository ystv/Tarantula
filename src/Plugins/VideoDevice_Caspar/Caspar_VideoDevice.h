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

#include "libCaspar/libCaspar.h"
#include "VideoDevice.h"
#include "PluginConfig.h"

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
    void cue (std::string clip);
    void play ();
    void stop ();
    virtual void poll ();
private:
    CasparConnection *m_pcaspcon;

    //!So we can run the sockets in a background thread and avoid blocking
    std::queue<CasparCommand> m_commandqueue;

    //!Holds the responses from commands
    std::queue<std::vector<std::string>> m_responsequeue;

    //!Mutex for commandQueue
    pthread_mutex_t m_queuemutex;

    //! Thread for main command/response queue
    pthread_t m_netthread;

    //! Configured hostname and port number
    std::string m_hostname;
    int m_port;
};

//thread
void *NetworkThread (void *ptr);
