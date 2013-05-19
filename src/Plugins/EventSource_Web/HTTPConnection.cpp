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
		std::shared_ptr<HTMLSnippets> psnippets,
		configdata& config) :
        m_socket(io_service),
        m_strand(io_service)
{
    http::server3::request_parser m_parser;
    m_pevents = pevents;
    m_psnippets = psnippets;
    m_config = config;
}

HTTPConnection::~HTTPConnection ()
{
}

std::shared_ptr<HTTPConnection> HTTPConnection::create (
        boost::asio::io_service& io_service,
        std::shared_ptr<std::set<MouseCatcherEvent, MCE_compare>> pevents,
        std::shared_ptr<HTMLSnippets> psnippets,
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
 * @param e
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
 * Generate the page showing the schedule for a particular day
 *
 * @param date Day to generate items for
 * @param rep  Reply to fill with resulting webpage
 */
void HTTPConnection::generateSchedulePage (boost::gregorian::date date,
		http::server3::reply& rep)
{
	pugi::xml_document pagedocument;

	pugi::xml_parse_result result = pagedocument.load_file(std::string(
			m_config.m_webpath + "/index.html").c_str());

	// Return an error if parsing fails
	if (pugi::status_ok != result.status)
	{
		rep = http::server3::reply::stock_reply(http::server3::reply::internal_server_error);
		return;
	}

	// Generate the event type related items
	for (auto eventtype : m_psnippets->m_deviceactions)
	{
		// Add the Add button
		pugi::xml_node addbutton = pagedocument.select_single_node(
				"//div[@id='add-event-bounding']").node().append_child("button");
		addbutton.append_attribute("class").set_value("button-big margin-bottom addbutton");
		addbutton.append_attribute("id").set_value(eventtype.first.c_str());
		addbutton.text().set(eventtype.first.c_str());

		// Add the event type drop-down option
		pugi::xml_node dropdownoption = pagedocument.select_single_node(
				"//select[@name='type']").node().append_child("option");
		dropdownoption.append_attribute("value").set_value(eventtype.first.c_str());
		dropdownoption.text().set(eventtype.first.c_str());

		// Generate a sub-form for this type
		pugi::xml_node subform = pagedocument.select_single_node(
				"//div[@id='add-form']/form").node().append_child("div");
		subform.append_attribute("id").set_value(std::string("add-" +
				eventtype.first).c_str());
		subform.append_attribute("class").set_value("add-sub formitem");

		// Add the Device drop-down
		pugi::xml_node para = subform.append_child("p");
		para.append_attribute("class").set_value("form-line");
		pugi::xml_node label = para.append_child("label");

		if (!eventtype.second.m_isprocessor)
		{
			label.append_attribute("for").set_value("device");
			label.text().set("Device");
			para.append_copy(eventtype.second.m_pdevices->document_element());
		}

		// Add the time field
		para = subform.append_child("p");
		para.append_attribute("class").set_value("form-line");

		label = para.append_child("label");
		label.append_attribute("for").set_value("time");
		label.text().set("Time (HH:MM:SS)");
		pugi::xml_node input = para.append_child("input");
		input.append_attribute("class").set_value("form-input");
		input.append_attribute("name").set_value("time");

		// Add the action stuff (drop-down and a set of sub-forms)
		subform.append_copy(eventtype.second.m_pactionsnippet->document_element());

	}

	// Set the date in the header and datepicker
	boost::gregorian::date_facet* facet(new boost::gregorian::date_facet("%A %d %B %Y"));
	std::stringstream ss;
	ss.imbue(std::locale(ss.getloc(), facet));
	ss << date;
	pagedocument.select_single_node("//h2[@id='dateheader']").node().text()
			.set(ss.str().c_str());

	boost::gregorian::date_facet* facet2(new boost::gregorian::date_facet("%Y, %M, %D"));
	std::stringstream ss2;
	ss.imbue(std::locale(ss.getloc(), facet2));
	ss2 << date;
	pagedocument.select_single_node("//div[@id='currentdate']").node().text()
			.set(ss2.str().c_str());

	// Grab all the events in the specified day
	std::set<MouseCatcherEvent, MCE_compare>::iterator firstevent =
			m_pevents->end();
	std::set<MouseCatcherEvent, MCE_compare>::iterator lastevent =
			m_pevents->end();

	tm timemarker = boost::posix_time::to_tm(
	            boost::posix_time::ptime(date));
	long int starttime = mktime(&timemarker);
	timemarker = boost::posix_time::to_tm(boost::posix_time::ptime(
			date + boost::gregorian::days(1)));
	long int endtime = mktime(&timemarker);

	for (std::set<MouseCatcherEvent, MCE_compare>::iterator it = m_pevents->begin();
			it != m_pevents->end(); ++it)
	{
		if (firstevent == m_pevents->end() && (*it).m_triggertime > starttime
				&& (*it).m_triggertime < endtime)
		{
			firstevent = it;
		}

		if ((*it).m_triggertime > endtime)
		{
			if (firstevent != m_pevents->end())
			{
				lastevent = it;
				lastevent++;
			}
			break;
		}
	}

	std::set<MouseCatcherEvent, MCE_compare> dayevents(firstevent, lastevent);

	// Find the node to insert into
	pugi::xml_node datanode = pagedocument.select_single_node("//div[@id='scheduledata']").node();

	// Generate each item in the list
	for (auto currentevent : dayevents)
	{
		generateScheduleSegment(currentevent, datanode);
	}


	// Add the snippets data for the add button etc.

	// Dump the HTML to the reply
	std::stringstream html;
	pagedocument.save(html, "", pugi::format_no_declaration);
	rep.content = html.str();
	rep.status = http::server3::reply::ok;
}

void rowgenerate(pugi::xml_node& parent, std::string key, std::string value)
{
	pugi::xml_node row = parent.append_child("tr");

	row.append_attribute("class").set_value(std::string("event-" + key).c_str());
	row.append_child("td").text().set(key.c_str());
	row.append_child("td").text().set(value.c_str());
}

/**
 * Generate a single accordion item for an event. Recurses through children and
 * adds result to an xml_node.
 *
 * @param targetevent Event to generate item for
 * @param parent      XML node for resulting HTML
 */
void HTTPConnection::generateScheduleSegment(MouseCatcherEvent& targetevent,
		pugi::xml_node& parent)
{
	struct tm * eventstart_tm = localtime(&targetevent.m_triggertime);
	char eventstart_buffer[10];
	strftime(eventstart_buffer, 10, "%H:%M:%S", eventstart_tm);

	long int eventend_int = targetevent.m_triggertime + (targetevent.m_duration/25);
	struct tm * eventend_tm = localtime(&eventend_int);
	char eventend_buffer[10];
	strftime(eventend_buffer, 10, "%H:%M:%S", eventend_tm);

	std::string devicetype = m_psnippets->m_devices[targetevent.m_targetdevice];

	// Headings and setup
	pugi::xml_node headingnode = parent.append_child("h3");
	headingnode.append_attribute("id").set_value(std::string("eventhead-" +
			ConvertType::intToString(targetevent.m_eventid)).c_str());
	headingnode.text().set(std::string(std::string(eventstart_buffer) + " - " +
			std::string(eventend_buffer) + "  " + devicetype).c_str());

	pugi::xml_node datadiv = parent.append_child("div");
	pugi::xml_node datatable = datadiv.append_child("table");

	// Fill table contents for parent event
	rowgenerate(datatable, "Channel", targetevent.m_channel);
	rowgenerate(datatable, "Device", targetevent.m_targetdevice);

	// Get the name of the action
	if (-1 == targetevent.m_action)
	{
		rowgenerate(datatable, "Action", "Event Processor");
	}
	else
	{
		std::string actionname = "Unknown";
		for (auto thisaction :
				m_psnippets->m_deviceactions[devicetype].m_actions)
		{
			if (thisaction.actionid == targetevent.m_action)
			{
				actionname = thisaction.name;
				break;
			}
		}
		rowgenerate(datatable, "Action", actionname);
	}

	rowgenerate(datatable, "Duration", ConvertType::intToString(targetevent.m_duration));

	std::string eventtype;
	switch (targetevent.m_eventtype)
	{
		case EVENT_FIXED:
			eventtype = "Fixed";
			break;
		case EVENT_CHILD:
			eventtype = "Child";
			break;
		case EVENT_MANUAL:
			eventtype = "Manual";
			break;
		default:
			eventtype = "Unknown";
			break;
	}

	rowgenerate(datatable, "Type", eventtype);
	rowgenerate(datatable, "EventID", ConvertType::intToString(targetevent.m_eventid));

	// Generate the additional data table
	datadiv.append_child("h4").text().set("Additional Data");
	pugi::xml_node additionaltable = datadiv.append_child("table");

	for (auto dataline : targetevent.m_extradata)
	{
		rowgenerate(additionaltable, dataline.first, dataline.second);
	}

	// Generate child events
	pugi::xml_node childevents = datadiv.append_child("div");
	childevents.append_attribute("class").set_value("accordion");

	for (auto child : targetevent.m_childevents)
	{
		generateScheduleSegment(child, childevents);
	}

	// Generate remove event link
	pugi::xml_node remove_event = datadiv.append_child("p").append_child("a");
	remove_event.append_attribute("href").set_value(
			std::string("/remove/" + ConvertType::intToString(targetevent.m_eventid)).c_str());
	remove_event.text().set("Remove Event");

	// Generate add-after link and time data
	pugi::xml_node add_after = datadiv.append_child("p");

	pugi::xml_node add_after_data = add_after.append_child("span");
	add_after_data.append_attribute("style").set_value("display: none;");
	add_after_data.append_attribute("class").set_value("data-endtime");
	add_after_data.text().set(eventend_buffer);

	add_after_data = add_after.append_child("span");
	add_after_data.append_attribute("style").set_value("display: none;");
	add_after_data.append_attribute("class").set_value("data-devicetype");
	add_after_data.text().set(devicetype.c_str());

	pugi::xml_node add_after_link = add_after.append_child("a");
	add_after_link.append_attribute("href").set_value("");
	add_after_link.append_attribute("class").set_value("add-after-link");
	add_after_link.text().set("Add event after this");

}

std::string urlDecode(std::string& src)
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

			std::string mime_type = "text/plain";

			if (path.empty())
			{
				// Send bad request response
				m_reply = http::server3::reply::stock_reply(http::server3::reply::bad_request);
			}
			else
			{
				// Split up the path
				unsigned int baseend = path.find_first_of('/', 1);

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

							m_psnippets->m_localqueue.push_back(newevent);

							m_reply.content = "Added successfully!";
							m_reply.status = http::server3::reply::ok;
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
					}
				}
				else if (!base.compare("files"))
				{
					std::string device = urlDecode(data);

					pugi::xml_document filedata;
					pugi::xml_node rootnode = filedata.append_child("select");
					rootnode.append_attribute("name").set_value(
							std::string("action-" + device + "-filename").c_str());
					rootnode.append_attribute("class").set_value("action-filename action-data-input");

					for (auto fileitem : m_psnippets->m_files[device])
					{
						pugi::xml_node thisfile = rootnode.append_child("option");
						thisfile.append_attribute("value").set_value(fileitem.first.c_str());
						thisfile.append_attribute("tar-length").set_value(fileitem.second);
						thisfile.text().set(std::string(fileitem.first + " - " +
								ConvertType::intToString(fileitem.second/25) +
								" seconds").c_str());
					}

					std::stringstream fileout;
					rootnode.print(fileout);

					m_reply.content = fileout.str();
					m_reply.status = http::server3::reply::ok;
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
						m_psnippets->m_localqueue.push_back(removeaction);

						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::moved_temporarily);
						m_reply.headers.resize(3);
						m_reply.headers[2].name = "Location";
						m_reply.headers[2].value = "/index.html";
					}
					catch (std::exception&)
					{
						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::bad_request);
					}
				}
				else if (!base.compare("index.html"))
				{
					// Grab the schedule page for today
					generateSchedulePage(boost::gregorian::date
							(boost::gregorian::day_clock::local_day()),
							m_reply);
					mime_type = "application/xhtml+xml";
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
					}
					else
					{
						std::stringstream buffer;
						buffer << is.rdbuf();
						m_reply.content = buffer.str();
						is.close();
						mime_type = "text/css";
						m_reply.status = http::server3::reply::ok;
					}
				}
				// Assume the path was a date
				else if (!base.empty())
				{
					try
					{
						generateSchedulePage(boost::gregorian::date(
								boost::gregorian::from_undelimited_string(base)),
								m_reply);
						mime_type = "application/xhtml+xml";
					}
					catch (std::exception&)
					{
						// Return the 404 handler
						m_reply = http::server3::reply::stock_reply(
								http::server3::reply::not_found);
					}
				}
				else
				{
					// Return the 404 handler
					m_reply = http::server3::reply::stock_reply(
							http::server3::reply::not_found);
				}

				m_reply.headers.resize(4);
				m_reply.headers[0].name = "Content-Length";
				m_reply.headers[0].value = boost::lexical_cast<std::string>(m_reply.content.size());
				m_reply.headers[1].name = "Content-Type";
				m_reply.headers[1].value = mime_type;

				// Send the reply
				boost::asio::async_write(m_socket, m_reply.to_buffers(),
					m_strand.wrap(
							boost::bind(&HTTPConnection::handleWrite,
									shared_from_this(),
									boost::asio::placeholders::error)));
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
