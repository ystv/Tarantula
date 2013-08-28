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
 *   File Name   : VideoDevice_PiVT.cpp
 *   Version     : 1.0
 *   Description : 
 *
 *****************************************************************************/

#include "boost/bind.hpp"

#include "Misc.h"
#include "VideoDevice_PiVT.h"

VideoDevice_PiVT::VideoDevice_PiVT (PluginConfig config, Hook h) :
    VideoDevice(config, h), m_io_service(), m_socket(m_io_service)
{
    // Pull server name and port from configuration data
    try
    {
        m_hostname = config.m_plugindata_map.at("Host");
        m_port = config.m_plugindata_map.at("Port");

        m_connecttimeout = ConvertType::stringToInt(config.m_plugindata_map.at("ConnectTimeout"));
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname, "Configuration data invalid or not supplied");
        m_status = FAILED;
        return;
    }

    h.gs->L->info(m_pluginname, "Connecting to " + m_hostname + ":" + m_port);

    /* Resolve the host */
    boost::asio::ip::tcp::resolver res(m_io_service);
    boost::asio::ip::tcp::resolver::query resq(m_hostname, m_port);
    boost::asio::ip::tcp::resolver::iterator resiter = res.resolve(resq);

    /* Spawn off an async connect operation */
    boost::asio::async_connect(m_socket, resiter,
            boost::bind(&VideoDevice_PiVT::cb_connect, this, boost::asio::placeholders::error));

    m_status = WAITING;

    getFiles();

}

VideoDevice_PiVT::~VideoDevice_PiVT ()
{
    // TODO Auto-generated destructor stub
}

/**
 * Request status update from server
 */
void VideoDevice_PiVT::updateHardwareStatus ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for updateHardwareStatus()");
        return;
    }

    // Queue prevents multiple commands running in parallel and confusion of results
    m_commandqueue.push("i\r\n");

    sendCommand();
}

/**
 * Request a list of files and durations from server
 */
void VideoDevice_PiVT::getFiles ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for getFiles()");
        return;
    }

    m_commandqueue.push("g\r\n");

    sendCommand();
}

/**
 * Load a file in the background on the server
 *
 * @param clip Name of file to load
 */
void VideoDevice_PiVT::cue (std::string clip)
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for cue()");
        return;
    }

    m_commandqueue.push("l " + clip + "\r\n");

    sendCommand();
}

/**
 * Play the loaded file on the server
 */
void VideoDevice_PiVT::play ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for play()");
        return;
    }

    m_commandqueue.push("p\r\n");

    sendCommand();
}

/**
 * Load and immediately play a file
 *
 * @param filename Name of file to play
 */
void VideoDevice_PiVT::immediatePlay (std::string filename)
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for immediatePlay()");
        return;
    }

    m_commandqueue.push("p " + filename + "\r\n");

    sendCommand();
}

/**
 * Stop playing and run the stop video
 */
void VideoDevice_PiVT::stop ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for stop()");
        return;
    }

    m_commandqueue.push("s\r\n");

    sendCommand();
}

/**
 * Run any ready communication jobs and check the socket is still up
 */
void VideoDevice_PiVT::poll ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for poll()");
        return;
    }

    if (m_socket.is_open() || WAITING == m_status)
    {
        try
        {
            m_io_service.poll();
            m_io_service.reset();
        }
        catch (std::exception &ex)
        {
            m_status = FAILED;
            m_hook.gs->L->error(m_pluginname, std::string("Error in network subsystem. Message was: ") +
                    std::string(ex.what()));
        }


        if (WAITING == m_status)
        {
            m_connecttimeout--;
        }

        if (0 == m_connecttimeout)
        {
            m_status = FAILED;
            m_hook.gs->L->error(m_pluginname, "Connection timed out.");
        }
    }
    else
    {
        m_status = CRASHED;
        m_hook.gs->L->error(m_pluginname, "Connection dropped.");
    }

    VideoDevice::poll();
}


/**
 * If there is only one command waiting in the queue, begin sending it
 * (If there are more commands, they will be sent by the callbacks)
 */
void VideoDevice_PiVT::sendCommand ()
{
    // If this is the only command, send it
    if (1 == m_commandqueue.size())
    {
        const char *buffer = m_commandqueue.back().c_str();

        boost::asio::async_write(m_socket, boost::asio::buffer(buffer, strlen(buffer)),
                boost::bind(&VideoDevice_PiVT::cb_write, this, boost::asio::placeholders::error));
    }

}

/**
 * Clean up last command and send another if ready
 */
void VideoDevice_PiVT::sendQueuedCommand ()
{
    m_recvdata.consume(m_recvdata.size());

    // Remove the command from the queue
    m_commandqueue.pop();

    // Send a new command
    if (m_commandqueue.size() > 0)
    {
        const char *buffer = m_commandqueue.front().c_str();

        boost::asio::async_write(m_socket, boost::asio::buffer(buffer, strlen(buffer)),
                boost::bind(&VideoDevice_PiVT::cb_write, this, boost::asio::placeholders::error));
    }
}

/**
 * Callback after connection has completed, resets plugin state to READY
 *
 * @param err Boost error received during connect
 */
void VideoDevice_PiVT::cb_connect (const boost::system::error_code& err)
{
    if (!err)
    {
        m_status = READY;
    }
    else
    {
        m_status = FAILED;
        m_hook.gs->L->error(m_pluginname, "Error connecting to server. Error was " + err.message());
    }

}

/**
 * Callback after a write has completed, sets up a read
 *
 * @param err System error code if one ocurred
 */
void VideoDevice_PiVT::cb_write (const boost::system::error_code& err)
{
    if (!err)
    {
        boost::asio::async_read_until(m_socket, m_recvdata, "\n",
                boost::bind(&VideoDevice_PiVT::cb_read, this, boost::asio::placeholders::error));
    }
    else
    {
        m_status = CRASHED;
        m_hook.gs->L->error(m_pluginname, "Error communicating with server. Error was " + err.message());
    }
}

/**
 * Callback from a read-after-write - handles incoming response
 *
 * @param err System error code if one occurred
 */
void VideoDevice_PiVT::cb_read (const boost::system::error_code& err)
{
    if (!err)
    {
        std::istream is(&m_recvdata);
        std::string line;

        while (std::getline(is, line))
        {
            std::string respcode = line.substr(0, 3);
            std::string resptext = line.substr(3, line.size() - 3);
            int responsecode = ConvertType::stringToInt(respcode.c_str());

            // Check and handle the response code
            switch (responsecode)
            {
                case RESP_MORELIST:
                {
                    // Add this file
                    int pos = resptext.find(' ', 1);
                    VideoFile newfile;
                    newfile.m_filename = resptext.substr(1, pos - 1);
                    newfile.m_title = newfile.m_filename;
                    newfile.m_duration =
                            g_pbaseconfig->getFramerate() *
                            ConvertType::stringToInt(resptext.substr(pos + 1, resptext.find(' ', pos + 1)));
                    m_files[newfile.m_filename] = newfile;

                    // Kick off another read for the next one
                    boost::asio::async_read_until(m_socket, m_recvdata, "\n",
                            boost::bind(&VideoDevice_PiVT::cb_read, this, boost::asio::placeholders::error));

                    break;
                }
                case RESP_INFO:
                {
                    // Parse the info string and update internal status
                    std::istringstream is(resptext);
                    std::vector<std::string> tokens;
                    std::string s;
                    std::getline(is, s, ',');

                    if (!s.substr(1, 7).compare("Playing"))
                    {
                        m_videostatus.state = VDS_PLAYING;
                        m_videostatus.filename = s.substr(9, s.size() - 9);

                        // Get the time remaining section
                        std::getline(is, s, ',');
                        std::getline(is, s, ',');

                        m_videostatus.remainingframes =
                                g_pbaseconfig->getFramerate() *
                                ConvertType::stringToInt(s.substr(1, s.find(' ', 2)));
                    }
                    else
                    {
                        m_videostatus.state = VDS_STOPPED;
                    }
                    break;
                }
                case RESP_ERROR:
                {
                    // Log the error
                    m_hook.gs->L->error(m_pluginname, "Got an error from command: " + resptext);
                    break;
                }
                case RESP_BADCOMMAND:
                {
                    // Log the error
                    m_hook.gs->L->error(m_pluginname, "Got a bad command error. This should not happen. Text was: " +
                            resptext);
                    m_status = CRASHED;
                    break;

                }
                case RESP_PLAYOK:
                case RESP_STOPOK:
                case RESP_LOADOK:
                case RESP_LISTOK:
                default:
                {
                    // Nothing to do
                    break;
                }
            }
        }

        // Send another command if present
        if (m_commandqueue.size() > 0)
        {
            sendQueuedCommand();
        }
    }
    else
    {
        m_status = CRASHED;
    }
}



extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<VideoDevice_PiVT> plugtemp = std::make_shared<VideoDevice_PiVT>(config, h);

        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}




