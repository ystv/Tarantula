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
*   File Name   : EventSource_Web.h
*   Version     : 1.0
*   Description : An EventSource to run an HTTP interface on port 9816
*
*****************************************************************************/


#pragma once

#define MAX_MESSAGE_LEN 4096

#include "MouseCatcherSourcePlugin.h"
#include "MouseCatcherCore.h"
#include "HTTPConnection.h"
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <queue>
#include <memory>
#include <set>

class EventSource_Web: public MouseCatcherSourcePlugin
{
public:
	EventSource_Web (PluginConfig config, Hook h);
    ~EventSource_Web ();

    void tick (std::vector<EventAction> *ActionQueue);

    // Callbacks used to update plugin from the core
    void updatePlaylist (std::vector<MouseCatcherEvent>& playlist,
            std::shared_ptr<void> additionaldata);
    void updateDevices (std::map<std::string, std::string>& devices,
            std::shared_ptr<void> additionaldata);
    void updateDeviceActions (std::string device,
            std::vector<ActionInformation>& actions,
            std::shared_ptr<void> additionaldata);
    void updateEventProcessors (
            std::map<std::string, ProcessorInformation>& processors,
            std::shared_ptr<void> additionaldata);
    void updateFiles (std::string device, std::vector<std::pair<std::string, int>>& files,
            std::shared_ptr<void> additionaldata);

private:
    std::shared_ptr<boost::asio::io_service> m_io_service;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;

    // Webserver handlers
    void startAccept ();
    void handleAccept (std::shared_ptr<WebSource::HTTPConnection> new_connection,
            const boost::system::error_code& error);

    // Page generation functions
    void generateScheduleSegment (MouseCatcherEvent& targetevent, pugi::xml_node& parent);
    void generateSchedulePage (std::shared_ptr<WebSource::WaitingRequest> req);

    // Storage for page snippets from callbacks
    std::shared_ptr<std::set<MouseCatcherEvent, WebSource::MCE_compare>> m_pevents;

    // HTML snippets for other callback data
    std::shared_ptr<WebSource::ShareData> m_sharedata;

    WebSource::configdata m_config;

    // Counters to handle playlist updates
    long int m_polltime;
    long int m_last_update_time;


};

