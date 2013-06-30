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
*   File Name   : HTTPConnection.cpp
*   Version     : 1.0
*   Description : A simple HTTP server class
*
*****************************************************************************/

#include <fstream>

#include "DateConversions.h"

#include "HTTPConnection.h"
#include "Misc.h"

namespace WebSource
{

/**
 * Constructor. Sets up the Boost.Asio stuff.
 *
 * @param io_service
 * @param pevents   Pointer to a sorted list of events in the playlist
 * @param psnippets Pointer to a set of HTML snippets for page generation
 * @param config    The plugin configuration
 */
HTTPConnection::HTTPConnection (boost::asio::io_service& io_service,
		std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> pevents,
		std::shared_ptr<ShareData> psnippets,
		configdata& config) :
        m_socket(io_service),
        m_strand(io_service)
{
    http::server3::request_parser m_parser;
    m_pevents = pevents;
    m_sharedata = psnippets;
    m_config = config;
}

HTTPConnection::~HTTPConnection ()
{
}

std::shared_ptr<HTTPConnection> HTTPConnection::create (
        boost::asio::io_service& io_service,
        std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> pevents,
        std::shared_ptr<ShareData> psnippets,
		configdata& config)
{
    return std::shared_ptr<HTTPConnection>(
            new HTTPConnection(io_service, pevents, psnippets, config));
}

boost::asio::ip::tcp::socket& HTTPConnection::socket ()
{
    return m_socket;
}

/**
 * Begin an asynchronous read from an incoming connection
 */
void HTTPConnection::start ()
{
	m_socket.async_read_some(boost::asio::buffer(m_buffer),
	      m_strand.wrap(
	        boost::bind(&HTTPConnection::handleIncomingData, shared_from_this(),
	          boost::asio::placeholders::error,
	          boost::asio::placeholders::bytes_transferred)));
}

/**
 * Close the connection after data is written.
 *
 * @param e Boost system error code (just checked for presence)
 */
void HTTPConnection::handleWrite (const boost::system::error_code& e)
{
	if (!e)
	{
		// Initiate graceful connection closure.
		boost::system::error_code ignored_ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	}
}

/**
 * Insert a request for the playlist to be updated and sent back to a client.
 *
 * @param requesteddate Date to grab playlist for, blank for today
 * @param req           Pointer to a waiting request if one is available.
 */
void HTTPConnection::requestPlaylistUpdate (std::string requesteddate, std::shared_ptr<WaitingRequest> req)
{
    EventAction playlistupdate;

    playlistupdate.action = ACTION_UPDATE_PLAYLIST;

    // Generate a trigger time
    if (requesteddate.empty())
    {
        requesteddate = boost::gregorian::to_simple_string(boost::gregorian::day_clock::local_day());
        playlistupdate.event.m_triggertime = DateConversions::datetimeToTimeT(requesteddate, "%Y-%b-%d");
    }
    else
    {
        try
        {
            playlistupdate.event.m_triggertime = DateConversions::datetimeToTimeT(requesteddate, "%Y-%b-%d");
        }
        catch (std::exception&)
        {
            m_reply = http::server3::reply::stock_reply(
                    http::server3::reply::internal_server_error);
            commitResponse("text/plain");
            return;
        }
    }

    // Make window last for one day
    playlistupdate.event.m_duration = 60*60*24;

    // Set channel from global configuration file
    playlistupdate.event.m_channel = m_config.m_channel;

    // Prepare the additional data structures
    std::shared_ptr<EventActionData> ead = std::make_shared<EventActionData>();
    ead->complete = false;
    ead->connection = shared_from_this();
    ead->attachedrequest.reset();

    if (!req)
    {
        ead->type = WEBACTION_PLAYLIST;
    }
    else
    {
        ead->type = WEBACTION_ALL;
        ead->attachedrequest = std::shared_ptr<WaitingRequest>(req);
        req->actions.push_back(ead);
    }

    // Assemble the XHTML node that will form the accordion
    pugi::xml_node datanode = ead->data.append_child("div");
    datanode.append_attribute("id").set_value("scheduledata");
    datanode.append_attribute("class").set_value("accordion");

    // Add additional data to the request
    playlistupdate.additionaldata.reset();
    playlistupdate.additionaldata = std::shared_ptr<EventActionData>(ead);

    // Add to local queue (to be added globally in tick())
    m_sharedata->m_localqueue.push_back(playlistupdate);
}

/**
 * Insert a request for a list of files to be grabbed and sent to the client
 * @param device The device to grab files for
 */
void HTTPConnection::requestFilesUpdate (std::string device)
{
    if (device.empty())
    {
        m_reply = http::server3::reply::stock_reply(
                http::server3::reply::internal_server_error);
        commitResponse("text/plain");
        return;
    }

    EventAction filesupdate;

    filesupdate.action = ACTION_UPDATE_FILES;
    filesupdate.event.m_targetdevice = device;

    // Prepare additional data
    std::shared_ptr<EventActionData> ead = std::make_shared<EventActionData>();
    ead->complete = false;
    ead->connection = shared_from_this();
    ead->type = WEBACTION_FILES;
    ead->attachedrequest.reset();

    // Assemble the XHTML node to go back to the client
    pugi::xml_node rootnode = ead->data.append_child("select");
    rootnode.append_attribute("name").set_value(
            std::string("action-" + device + "-filename").c_str());
    rootnode.append_attribute("class").set_value("action-filename action-data-input");

    // Add additional data to the request
    filesupdate.additionaldata.reset();
    filesupdate.additionaldata = std::shared_ptr<EventActionData>(ead);

    // Add to local queue (to be added globally in tick())
    m_sharedata->m_localqueue.push_back(filesupdate);

}

/**
 * Decode a string encoded using URL encoding.
 *
 * @param src Input string to decode
 * @return    Decoded result
 */
static std::string urlDecode (std::string& src)
{
    std::string ret;
    char ch;
    unsigned int i, ii;
    for (i=0; i<src.length(); i++) {
        if (int(src[i])==37) {
            sscanf(src.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=src[i];
        }
    }
    return (ret);
}

/**
 * Finalise and send the response held in m_reply, then close the connection
 *
 * @param mime_type Optional MIME type, otherwise defaults to "text/plain"
 */
void HTTPConnection::commitResponse (const std::string& mime_type /*= "text/plain"*/)
{
    m_reply.headers.resize(4);
    m_reply.headers[0].name = "Content-Length";
    m_reply.headers[0].value = boost::lexical_cast<std::string>(m_reply.content.size());
    m_reply.headers[1].name = "Content-Type";
    m_reply.headers[1].value = mime_type;

    // Send the reply
    boost::asio::async_write(m_socket, m_reply.to_buffers(), m_strand.wrap(
        boost::bind(&HTTPConnection::handleWrite, shared_from_this(),
                boost::asio::placeholders::error)));
}

/**
 * Process the incoming HTTP request
 *
 * @param error             Error code generated in request
 * @param bytes_transferred Number of bytes received from the host
 */
void HTTPConnection::handleIncomingData (
        const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		boost::tribool result;
		boost::tie(result, boost::tuples::ignore) = m_parser.parse(
				m_request, m_buffer.data(), m_buffer.data() + bytes_transferred);

		if (result)
		{
			std::string path = http::server3::request_handler::handle_request(m_request);

			if (path.empty())
			{
				// Send bad request response
				m_reply = http::server3::reply::stock_reply(http::server3::reply::bad_request);
			}
			else
			{
				// Split up the path
				size_t baseend = path.find_first_of('/', 1);

				std::string data("");
				std::string base("");

				if (baseend != std::string::npos)
				{
					base = path.substr(1, baseend-1);
					data = path.substr(baseend+1, path.length()-baseend);

				}
				else
				{
					base = path.substr(1, path.length());
				}

				// Identify the action requested
				if (!base.compare("add"))
				{
					data = data.substr(1, data.length());
					try
					{
						data = urlDecode(data);

						pugi::xml_document incomingdoc;

						pugi::xml_parse_result parsefind = incomingdoc.load(data.c_str());

						if (pugi::status_ok == parsefind.status)
						{
							pugi::xml_node newdata = incomingdoc.document_element();

							EventAction newevent;
							newevent.action = ACTION_ADD;
							newevent.event.m_channel = m_config.m_channel;
							newevent.event.m_targetdevice = newdata.child_value("device");

							std::string timedata = newdata.child_value("time");
							tm starttime = boost::posix_time::to_tm(
									boost::posix_time::time_from_string(timedata.c_str()));
							newevent.event.m_triggertime = mktime(&starttime);

							newevent.event.m_action = newdata.child("action").text().as_int();

							newevent.event.m_eventtype = EVENT_FIXED;

							newevent.event.m_duration = newdata.child("extradata").
									child("filename").attribute("duration").as_int(0);

							for (pugi::xml_node piece : newdata.child("extradata").children())
							{
								newevent.event.m_extradata[piece.name()] = piece.child_value();
							}

							m_sharedata->m_localqueue.push_back(newevent);

							m_reply.content = "Added successfully!";
							m_reply.status = http::server3::reply::ok;
							commitResponse();
						}
						else
						{
							throw (std::exception());
						}
					}
					catch (std::exception&)
					{
						m_reply.content = "Error!";
						m_reply.status = http::server3::reply::ok;
						commitResponse();
					}
				}
				else if (!base.compare("files"))
				{
					std::string device = urlDecode(data);

                    requestFilesUpdate(device);
				}
				else if (!base.compare("edit"))
				{

				}
				else if (!base.compare("remove"))
				{
					try
					{
						int eventid = ConvertType::stringToInt(data);
						EventAction removeaction;
						removeaction.event.m_eventid = eventid;
						removeaction.action = ACTION_REMOVE;
						removeaction.event.m_channel = m_config.m_channel;
						m_sharedata->m_localqueue.push_back(removeaction);

						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::moved_temporarily);
						m_reply.headers.resize(3);
						m_reply.headers[2].name = "Location";
						m_reply.headers[2].value = "/index.html";
						commitResponse();
					}
					catch (std::exception&)
					{
						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::bad_request);
						commitResponse();
					}
				}
				else if (!base.compare("update"))
				{
				    requestPlaylistUpdate(data);
				}
				else if (!base.compare("index.html"))
				{
				    // Set up the request
				    std::shared_ptr<WaitingRequest> req = std::make_shared<WaitingRequest>();
				    req->complete = false;
				    req->connection = shared_from_this();

				    req->requesteddate = boost::gregorian::day_clock::local_day();
			        std::string requesteddate = boost::gregorian::to_simple_string(req->requesteddate);

				    // Create EventActions for devices and processors
				    EventAction deviceaction;
				    deviceaction.action = ACTION_UPDATE_DEVICES;

				    // Prepare the additional data structures
                    std::shared_ptr<EventActionData> ead = std::make_shared<EventActionData>();
                    ead->complete = false;
                    ead->connection = shared_from_this();
                    ead->type = WEBACTION_ALL;
                    ead->attachedrequest.reset();
                    ead->attachedrequest = std::shared_ptr<WaitingRequest>(req);
                    req->actions.push_back(ead);

                    // Add additional data to the request
                    deviceaction.additionaldata.reset();
                    deviceaction.additionaldata = std::shared_ptr<EventActionData>(ead);

                    m_sharedata->m_localqueue.push_back(deviceaction);

                    // Create EventActions for devices and processors
                    EventAction processoraction;
                    processoraction.action = ACTION_UPDATE_PROCESSORS;

                    // Prepare the additional data structures
                    std::shared_ptr<EventActionData> ead2 = std::make_shared<EventActionData>();
                    ead2->complete = false;
                    ead2->connection = shared_from_this();
                    ead2->type = WEBACTION_ALL;
                    ead2->attachedrequest.reset();
                    ead2->attachedrequest = std::shared_ptr<WaitingRequest>(req);
                    req->actions.push_back(ead2);

                    // Add additional data to the request
                    processoraction.additionaldata.reset();
                    processoraction.additionaldata = std::shared_ptr<EventActionData>(ead2);

                    m_sharedata->m_localqueue.push_back(processoraction);

                    // Request the playlist
                    requestPlaylistUpdate(requesteddate, std::shared_ptr<WaitingRequest>(req));

                    // Add the waiting request to the queue
                    m_sharedata->m_requests.push_back(req);
				}
				else if (!base.compare("tarantula.css"))
				{
					// Open the stylesheet and send it
					std::ifstream is(std::string(
							m_config.m_webpath + "/tarantula.css").c_str(),
							std::ios::in | std::ios::binary);

					if (!is)
					{
						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::not_found);
						commitResponse();
					}
					else
					{
						std::stringstream buffer;
						buffer << is.rdbuf();
						m_reply.content = buffer.str();
						is.close();

						m_reply.status = http::server3::reply::ok;
						commitResponse("text/css");
					}
				}
				// Assume the path was a date
				else if (!base.empty())
				{
					try
					{
						requestPlaylistUpdate(base);
					}
					catch (std::exception&)
					{
						// Return the 404 handler
						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::not_found);
						commitResponse();
					}
				}
				else
				{
					// Return the 404 handler
					m_reply = http::server3::reply::stock_reply(
							http::server3::reply::not_found);
					commitResponse();
				}

			}
		}
		else if (!result)
		{
			m_reply = http::server3::reply::stock_reply(http::server3::reply::bad_request);
			boost::asio::async_write(m_socket, m_reply.to_buffers(),
					m_strand.wrap(
							boost::bind(&HTTPConnection::handleWrite,
									shared_from_this(),
									boost::asio::placeholders::error)));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
					m_strand.wrap(
							boost::bind(&HTTPConnection::handleIncomingData,
									shared_from_this(),
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred)));
		}
	}
}

}
