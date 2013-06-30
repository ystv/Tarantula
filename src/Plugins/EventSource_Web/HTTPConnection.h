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
*   File Name   : HTTPConnection.h
*   Version     : 1.0
*   Description : A simple HTTP server class
*
*****************************************************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <ctime>
#include <set>
#include <sstream>
#include <memory>

#include "pugixml.hpp"
#include "request.hpp"
#include "reply.hpp"
#include "request_parser.hpp"
#include "request_handler.hpp"
#include "MouseCatcherCore.h"

namespace WebSource
{

/**
 * Type of request held by an EventActionData
 */
enum WebActionType
{
	WEBACTION_PLAYLIST,//!< WEBACTION_PLAYLIST Request for playlist from client
	WEBACTION_FILES,   //!< WEBACTION_FILES    Request for device files
	WEBACTION_ALL      //!< WEBACTION_ALL	   Request for a full update of playlist, actions, devices, etc
};


struct configdata
{
	std::string m_webpath;
	std::string m_channel;
	long int m_pollperiod;
	int m_port;
	Hook m_hook;
};

/**
 * A set of data and HTML snippets for a device type
 */
struct typedata
{
	std::vector<ActionInformation> m_actions;
	bool m_isprocessor;
	std::shared_ptr<pugi::xml_document> m_pdevices; //!< HTML snippet
	std::shared_ptr<pugi::xml_document> m_pactionsnippet;
};

// Forward declaration required for WaitingRequest, EventActionData
class HTTPConnection;

// Forward declaration to clear up circular dependency
struct EventActionData;

/**
 * Container so that a request can spin off multiple EventActions and wait for
 * their completion
 */
struct WaitingRequest
{
	boost::gregorian::date requesteddate;
	std::shared_ptr<HTTPConnection> connection;
	std::vector<std::shared_ptr<EventActionData>> actions;
	bool complete;
};

/**
 * Storage of HTML snippets generated in update callbacks
 */
struct ShareData
{
    std::vector<EventAction> m_localqueue;

    // Requests currently waiting for callbacks from core
    std::vector<std::shared_ptr<WaitingRequest> > m_requests;
};

/**
 * Supporting data so update callbacks are processed correctly
 */
struct EventActionData
{
	bool complete;
	std::shared_ptr<HTTPConnection> connection;
	WebActionType type;
	std::string error;
	pugi::xml_document data;
	std::shared_ptr<WaitingRequest> attachedrequest;
};



/**
 * Comparison operator to sort playlist efficiently by start time
 */
struct MCE_compare
{
	inline bool operator() (const MouseCatcherEvent& lhs,
			const MouseCatcherEvent& rhs) const
	{
		return lhs.m_triggertime < rhs.m_triggertime;
	}
};

class HTTPConnection: public std::enable_shared_from_this<HTTPConnection>
{
public:
	HTTPConnection (boost::asio::io_service& io_service,
			std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> pevents,
			std::shared_ptr<ShareData> psnippets,
			configdata& config);
    ~HTTPConnection ();

    static std::shared_ptr<HTTPConnection> create (
            boost::asio::io_service& io_service,
            std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> pevents,
            std::shared_ptr<ShareData> psnippets,
			configdata& config);

    boost::asio::ip::tcp::socket& socket ();

    // Event handlers for the socket system
    void start ();
    void handleWrite (const boost::system::error_code& e);
    void handleIncomingData (const boost::system::error_code& error,
            size_t bytes_transferred);
    void commitResponse(const std::string& mime_type = "text/plain");

    boost::asio::ip::tcp::socket m_socket;
    boost::array<char, 8192> m_buffer;
    boost::asio::io_service::strand m_strand;
    http::server3::request_parser m_parser;
    http::server3::request m_request;
    http::server3::reply m_reply;

    // Pointer to the temporary event store held by the plugin
    std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> m_pevents;
    std::shared_ptr<ShareData> m_sharedata;

    configdata m_config;

private:
	void requestPlaylistUpdate (std::string requesteddates, std::shared_ptr<WaitingRequest> req = NULL);
	void requestFilesUpdate (std::string device);
};
}
