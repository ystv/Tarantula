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
*   File Name   : EventSource_XML_Network.h
*   Version     : 1.0
*   Description : An EventSource to run a sockets interface on port 9815,
*                 accepting XML formatted commands and events.
*
*****************************************************************************/


#pragma once

#define MAX_MESSAGE_LEN 4096

#include "MouseCatcherSourcePlugin.h"
#include "MouseCatcherCore.h"
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <queue>
#include <memory>

class XML_Incoming;

class TCPConnection: public std::enable_shared_from_this<TCPConnection>
{
public:
    TCPConnection (boost::asio::io_service& io_service,
            std::vector<XML_Incoming> *m_incoming);
    ~TCPConnection ();

    static std::shared_ptr<TCPConnection> create (
            boost::asio::io_service& io_service,
            std::vector<XML_Incoming> *m_incoming);
    boost::asio::ip::tcp::socket& socket ();

    void start ();

    void handleWrite ();
    void handleIncomingData (const boost::system::error_code& error,
            size_t bytes_transferred);

    boost::asio::ip::tcp::socket m_socket;
    std::vector<XML_Incoming> *m_data_vector;

    boost::asio::streambuf m_messagedata;
};

class XML_Incoming
{
public:
    XML_Incoming ()
    {
    }
    XML_Incoming (std::shared_ptr<TCPConnection> c, std::string d)
    {
        m_conn = c;
        m_xmldata = d;
    }
    std::shared_ptr<TCPConnection> m_conn;
    std::string m_xmldata;
};

class EventSource_XML_Network: public MouseCatcherSourcePlugin
{
public:
    EventSource_XML_Network (PluginConfig config, Hook h);
    ~EventSource_XML_Network ();

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
    int m_port;

    std::vector<XML_Incoming> m_incoming;
    bool processIncoming (XML_Incoming& newdata, 
		EventAction& newaction);
    bool parseEvent (const pugi::xml_node xmlnode, 
		MouseCatcherEvent& outputevent);
    void startAccept ();
    void handleAccept (std::shared_ptr<TCPConnection> new_connection,
            const boost::system::error_code& error);

};

