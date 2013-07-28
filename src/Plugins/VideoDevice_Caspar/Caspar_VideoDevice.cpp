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
    long int connecttimeout = 10;
    try
    {
        m_hostname = config.m_plugindata_map.at("Host");
        m_port = config.m_plugindata_map.at("Port");

        connecttimeout = ConvertType::stringToInt(config.m_plugindata_map.at("ConnectTimeout"));
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname, "Configuration data invalid or not supplied");
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


    m_status = WAITING;

    getFiles();
}

VideoDevice_Caspar::~VideoDevice_Caspar ()
{
}

/**
 * Read and process the responseQueue
 */
void VideoDevice_Caspar::poll ()
{
    if (m_status != READY && m_status != WAITING)
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

    CasparCommand query(CASPAR_COMMAND_CLS, boost::bind(&VideoDevice_Caspar::cb_updatefiles, this, _1));
    m_pcaspcon->sendCommand(query);

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
 * Callback to handle files update
 *
 * @param resp Vector filled with lines from CasparCG
 */
void VideoDevice_Caspar::cb_updatefiles (std::vector<std::string>& resp)
{
    std::vector<std::string> medialist;

    //Call the processor
    CasparQueryResponseProcessor::getMediaList(resp, medialist);

    //Iterate over media list getting lengths and adding to files list
    for (std::string item : medialist)
    {
        VideoFile thisfile;
        thisfile.m_filename = item;
        thisfile.m_title = item;

        // To grab duration from CasparCG we have to load the file and pull info
        CasparCommand durationquery(CASPAR_COMMAND_LOADBG);
        durationquery.addParam("1");
        durationquery.addParam("5");
        durationquery.addParam(item);
        m_pcaspcon->sendCommand(durationquery);

        CasparCommand infoquery(CASPAR_COMMAND_INFO, boost::bind(&VideoDevice_Caspar::cb_updatelength, this, thisfile, _1));
        infoquery.addParam("1");
        infoquery.addParam("5");
        m_pcaspcon->sendCommand(infoquery);
    }

}

/**
 * Callback for file length in frames, will push file back in to list
 *
 * @param filedata Rest of file data to go in list
 * @param resp     Lines of data from CasparCG
 */
void VideoDevice_Caspar::cb_updatelength (VideoFile filedata, std::vector<std::string>& resp)
{
    filedata.m_duration = CasparQueryResponseProcessor::readFileFrames(resp);
    m_files[filedata.m_filename] = filedata;
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
