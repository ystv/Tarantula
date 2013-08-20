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
 *   File Name   : VideoDevice_PiVT.h
 *   Version     : 1.0
 *   Description : Device for playing video files using PiVT
 *                 (see http://ystv.co.uk/PiVT)
 *
 *****************************************************************************/

#pragma once

#include <queue>

#include "boost/asio.hpp"
#include "VideoDevice.h"
#include "PluginConfig.h"

#define RESP_INFO       200
#define RESP_PLAYOK     202
#define RESP_LOADOK     203
#define RESP_STOPOK     204
#define RESP_LISTOK     205
#define RESP_MORELIST   206
#define RESP_ERROR      404
#define RESP_BADCOMMAND 400

/**
 * Class to communicate with a PiVT video server
 */
class VideoDevice_PiVT: public VideoDevice
{
public:
    VideoDevice_PiVT (PluginConfig config, Hook h);
    virtual ~VideoDevice_PiVT ();

    void updateHardwareStatus ();
    void getFiles ();
    void cue (std::string clip);
    void play ();
    void immediatePlay (std::string filename);
    void stop ();
    virtual void poll ();

private:
    void sendCommand ();
    void sendQueuedCommand ();

    void cb_connect (const boost::system::error_code& err);
    void cb_read (const boost::system::error_code& err);
    void cb_write (const boost::system::error_code& err);

    std::string m_hostname;
    std::string m_port;
    std::queue <std::string> m_commandqueue;
    int m_connecttimeout;

    boost::asio::io_service m_io_service;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_recvdata;
};

