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
*   File Name   : EventSource_XML_Network.cpp
*   Version     : 1.0
*   Description : An EventSource to run a sockets interface on port 9815,
*                 accepting XML formatted commands and events.
*
*****************************************************************************/


#include "EventSource_XML_Network.h"
#include "pugixml.hpp"
#include "PlaylistDB.h"
#include "Misc.h"
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
#include <sstream>

TCPConnection::TCPConnection (boost::asio::io_service& io_service,
        std::vector<XML_Incoming> *m_incoming) :
        m_socket(io_service)
{
    m_data_vector = m_incoming;
}

TCPConnection::~TCPConnection ()
{
}

std::shared_ptr<TCPConnection> TCPConnection::create (
        boost::asio::io_service& io_service,
        std::vector<XML_Incoming> *m_pincoming)
{
    return std::shared_ptr<TCPConnection>(
            new TCPConnection(io_service, m_pincoming));
}

boost::asio::ip::tcp::socket& TCPConnection::socket ()
{
    return m_socket;
}

/**
 * Send a welcome message to a new client and set up a read.
 */
void TCPConnection::start ()
{
    std::string message = "Welcome to Tarantula.\r\n";

    boost::asio::write(m_socket, boost::asio::buffer(message));

    boost::asio::async_read_until(m_socket, m_messagedata, '\n',
            boost::bind(&TCPConnection::handleIncomingData,
                    shared_from_this(), boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred()));
}

void TCPConnection::handleWrite ()
{
}

/**
 * Read a line of data into the queue to be processed. Also starts another read
 *
 * @param error
 * @param bytes_transferred
 */
void TCPConnection::handleIncomingData (
        const boost::system::error_code& error, size_t bytes_transferred)
{
    if (error)
    {
        return;
    }
    XML_Incoming newdata;

    std::istream is(&m_messagedata);
    std::getline(is, newdata.m_xmldata);

    // Handle connection close request
    if (!newdata.m_xmldata.substr(0, 4).compare("quit") || !newdata.m_xmldata.substr(0, 4).compare("exit"))
    {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        m_socket.close();

        return;
    }

    newdata.m_conn = shared_from_this();

    m_data_vector->push_back(newdata);

    boost::asio::async_read_until(m_socket, m_messagedata, '\n',
            boost::bind(&TCPConnection::handleIncomingData,
                    shared_from_this(), boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred()));
}

/**
 * Constructor. Starts listener and reads configuration
 *
 * @param config Data loaded from configuration file
 * @param h		 Link back to GlobalStuff structures
 */
EventSource_XML_Network::EventSource_XML_Network (PluginConfig config, Hook h) :
        MouseCatcherSourcePlugin(config, h), m_io_service(new boost::asio::io_service),
        m_acceptor(*m_io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9815))
{
    // create a new tcp server
    try
    {
        startAccept();
    } catch (std::exception& e)
    {
        m_status = FAILED;
        h.gs->L->error(m_pluginname,
                std::string("Initialisation failed, error was: ") + e.what());
        return;
    }

    m_status = READY;
}


EventSource_XML_Network::~EventSource_XML_Network ()
{
    // close the connection to the relevant socket as it's cleaner than just
    // relying on destroying the object

    //destroy the IOservice object
    while (m_io_service != NULL)
    {
        m_io_service.reset();
    };
}

/**
 * Create a listener and wait for a client.
 */
void EventSource_XML_Network::startAccept ()
{
    std::shared_ptr<TCPConnection> new_connection = TCPConnection::create(
            m_acceptor.get_io_service(), &m_incoming);

    m_acceptor.async_accept(new_connection->socket(),
            boost::bind(&EventSource_XML_Network::handleAccept, this,
                    new_connection, boost::asio::placeholders::error));

    m_status = READY;
}

/**
 * Handle an incoming connection by starting the connection handler and running
 * another receive.
 *
 * @param new_connection Pointer to the incoming connection
 * @param error
 */
void EventSource_XML_Network::handleAccept (
        std::shared_ptr<TCPConnection> new_connection,
        const boost::system::error_code& error)
{
    if (!error)
    {
        new_connection->start();
    }

    startAccept();
}

/**
 * Parse an XML event into a MouseCatcherEvent
 *
 * @param xmlnode	  Input XML to parse
 * @param outputevent Event to populate with data from XML
 * @return			  False if XML was not valud
 */
bool EventSource_XML_Network::parseEvent (const pugi::xml_node xmlnode,
        MouseCatcherEvent& outputevent)
{
    outputevent.m_action = xmlnode.child("action").text().as_int();
    outputevent.m_channel = xmlnode.child_value("channel");
    outputevent.m_duration = xmlnode.child("duration").text().as_int(1);

    std::string type = xmlnode.child_value("type");
    for (auto et : playlist_event_type_vector)
    {
        if (type == et.second)
        {
            outputevent.m_eventtype = et.first;
            break;
        }
    }

    if (0 == playlist_event_type_vector.count(outputevent.m_eventtype))
    {
    	return false;
    }

    for (pugi::xml_node node : xmlnode.child("actiondata").children())
    {
        outputevent.m_extradata[node.name()] = node.child_value();
    }

    outputevent.m_targetdevice = xmlnode.child_value("targetdevice");

    tm starttime = boost::posix_time::to_tm(
            boost::posix_time::time_from_string(xmlnode.child_value("time")));
    outputevent.m_triggertime = mktime(&starttime);

    for (pugi::xml_node node : xmlnode.child("childevents").children())
    {
        MouseCatcherEvent newchild;
        parseEvent(node, newchild);
        outputevent.m_childevents.push_back(newchild);
    }
    return true;
}

/**
 * Process an incoming request and generate EventActions
 *
 * @param newdata   Incoming request data
 * @param newaction	An action to populate
 * @return			False if request was not valid
 */
bool EventSource_XML_Network::processIncoming (XML_Incoming& newdata,
        EventAction& newaction)
{
    pugi::xml_document document;
    pugi::xml_parse_result result = document.load(newdata.m_xmldata.c_str());

    // Return an error if parsing fails
    if (pugi::status_ok != result.status)
    {
        boost::asio::write(newdata.m_conn->socket(),
                boost::asio::buffer("400 BAD COMMAND\r\n"),
                boost::asio::transfer_all());
        return false;
    }

    pugi::xml_node xml = document.document_element();

    std::string action = xml.child_value("ActionType");

    newaction.isprocessed = false;
    newaction.thisplugin = this;

    newaction.additionaldata.reset();
    newaction.additionaldata = std::make_shared<XML_Incoming>(newdata.m_conn,
            newdata.m_xmldata);

    if (action.empty())
    {
        boost::asio::write(newdata.m_conn->socket(),
                boost::asio::buffer("400 NO ACTION\r\n"));
        return false;
    }
    else if (!action.compare("Add"))
    {
        if (xml.child("MCEvent").empty())
        {
            boost::asio::write(newdata.m_conn->socket(),
                    boost::asio::buffer("400 NO DATA\r\n"));
            return false;
        }

        if (!parseEvent(xml.child("MCEvent"), newaction.event))
        {
        	boost::asio::write(newdata.m_conn->socket(),
        			boost::asio::buffer("400 BAD DATA\r\n"));
        	return false;
        }

        newaction.action = ACTION_ADD;
    }
    else if (!action.compare("Remove"))
    {
        if (-1 == xml.child("eventid").text().as_int(-1))
        {
            boost::asio::write(newdata.m_conn->socket(),
                    boost::asio::buffer("400 NO DATA\r\n"));
            return false;
        }

        newaction.action = ACTION_REMOVE;
        newaction.eventid = xml.child("eventid").text().as_int(-1);
        newaction.event.m_channel = xml.child_value("channel");
    }
    else if (!action.compare("Edit"))
    {
        if ((xml.child("MCEvent").empty())
                || -1 == xml.child("eventid").text().as_int(-1))
        {
            boost::asio::write(newdata.m_conn->socket(),
                    boost::asio::buffer("400 NO DATA\r\n"));
            return false;
        }

        newaction.action = ACTION_EDIT;
        newaction.eventid = xml.child("eventid").text().as_int(-1);

        if (!parseEvent(xml.child("MCEvent"), newaction.event))
		{
			boost::asio::write(newdata.m_conn->socket(),
					boost::asio::buffer("400 BAD DATA\r\n"));
			return false;
		}

    }
    else if (!action.compare("UpdatePlaylist"))
    {
        newaction.action = ACTION_UPDATE_PLAYLIST;

        std::string start = xml.child_value("starttime");

        if (!start.empty())
        {
			tm starttime = boost::posix_time::to_tm(
				boost::posix_time::time_from_string(start));
			newaction.event.m_triggertime = mktime(&starttime);
        }
        else
        {
        	// Set the default time to now if not specified
        	newaction.event.m_triggertime = time(NULL);
        }

        // Set the default length to 1 day if not specified
        newaction.event.m_duration = xml.child("length").text().as_int(
        		time(NULL) + 86400);

        newaction.event.m_channel = xml.child_value("channel");
    }
    else if (!action.compare("UpdateDevices"))
    {
        newaction.action = ACTION_UPDATE_DEVICES;
    }
    else if (!action.compare("UpdateProcessors"))
    {
        newaction.action = ACTION_UPDATE_PROCESSORS;
    }
    else if (!action.compare("UpdateActions"))
    {
        if (xml.child("device").empty())
        {
            boost::asio::write(newdata.m_conn->socket(),
                    boost::asio::buffer("400 NO DATA\r\n"));
            return false;
        }

        newaction.action = ACTION_UPDATE_ACTIONS;
        newaction.event.m_targetdevice = xml.child_value("device");
    }
    else if (!action.compare("UpdateFiles"))
    {
        if (xml.child("device").empty())
        {
            boost::asio::write(newdata.m_conn->socket(),
                    boost::asio::buffer("400 NO DATA\r\n"));
            return false;
        }

        newaction.action = ACTION_UPDATE_FILES;
        newaction.event.m_targetdevice = xml.child_value("device");
    }
    else
    {
        boost::asio::write(newdata.m_conn->socket(),
                boost::asio::buffer("400 BAD ACTION\r\n"));
        return false;
    }

    return true;
}

/**
 * Run the plugin main loop
 *
 * @param ActionQueue Pointer to central queue of MouseCatcher actions
 */
void EventSource_XML_Network::tick (std::vector<EventAction>* ActionQueue)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state");
        return;
    }

    // Process ready asynchronous tasks
    m_io_service->poll();

    // Find processed queue events and report back
    for (EventAction thisaction : (*ActionQueue))
    {
        if (thisaction.isprocessed
                && thisaction.thisplugin
                        == dynamic_cast<MouseCatcherSourcePlugin*>(this))
        {
            std::string responsemessage;
            if (thisaction.returnmessage.empty())
            {
                responsemessage = "200 SUCCESS";
            }
            else
            {
                responsemessage = "500 " + thisaction.returnmessage;
            }
            std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
                    < XML_Incoming > (thisaction.additionaldata);
            boost::asio::write(plugindata->m_conn->socket(),
                    boost::asio::buffer(responsemessage + "\r\n"));
        }
    }

    std::vector<EventAction>::iterator it = std::remove_if(ActionQueue->begin(),
            ActionQueue->end(),
            boost::bind(&MouseCatcherSourcePlugin::actionCompleteCheck, _1,
                    dynamic_cast<MouseCatcherSourcePlugin*>(this)));

    ActionQueue->erase(it, ActionQueue->end());

    // Process everything in m_incoming
    for (XML_Incoming newdata : m_incoming)
    {
        EventAction newaction;

        if (processIncoming(newdata, newaction))
        {
            ActionQueue->push_back(newaction);
        }
    }
    m_incoming.clear();

}

/**
 * Helper function to add a child node and set the value
 *
 * @param parent Node for child to be added to
 * @param name	 Name of node
 * @param value	 Value inside added node
 */
void addchildwithvalue (pugi::xml_node& parent, const std::string name,
        const std::string value)
{
    pugi::xml_node child = parent.append_child(name.c_str());
    child.text().set(value.c_str());
}

/**
 * Helper function to generate XML for an event recursively
 *
 * @param parent Parent node to insert this event into
 * @param event	 Event to convert to XML
 */
void converteventtoxml (pugi::xml_node& parent,
        const MouseCatcherEvent& event)
{
    pugi::xml_node eventdata = parent.append_child("MCEvent");

    addchildwithvalue(eventdata, "channel", event.m_channel);
    addchildwithvalue(eventdata, "type",
            playlist_event_type_vector.at(event.m_eventtype));
    addchildwithvalue(eventdata, "targetdevice", event.m_targetdevice);
    eventdata.append_child("eventid").text().set(event.m_eventid);

    struct tm * timeinfo = localtime(&event.m_triggertime);
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", timeinfo);
    addchildwithvalue(eventdata, "time", buffer);

    addchildwithvalue(eventdata, "action",
            ConvertType::intToString(static_cast<int>(event.m_action)));

    pugi::xml_node duration = eventdata.append_child("duration");
    pugi::xml_attribute durationunits = duration.append_attribute("units");
    durationunits.set_value("seconds");

    duration.text().set(event.m_duration);

    pugi::xml_node actiondata = eventdata.append_child("actiondata");
    for (auto data : event.m_extradata)
    {
        addchildwithvalue(actiondata, data.first, data.second);
    }

    pugi::xml_node childnode = eventdata.append_child("childevents");
    for (MouseCatcherEvent childevent : event.m_childevents)
    {
        converteventtoxml(childnode, childevent);
    }
}

/**
 * Send playlist events to the client
 *
 * @param playlist       List of playlist events requested
 * @param additionaldata Action data, used for source connection handle
 */
void EventSource_XML_Network::updatePlaylist (
        std::vector<MouseCatcherEvent>& playlist,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updatePlaylist");
        return;
    }

    pugi::xml_document document;
    pugi::xml_node rootnode = document.append_child("TarantulaPlaylistData");

    for (MouseCatcherEvent event : playlist)
    {
        converteventtoxml(rootnode, event);
    }

    //Pull out the generated XML as a string
    std::ostringstream ss;
    document.save(ss, "\t", pugi::format_indent);

    std::string xmldata = ss.str();

    //Send back resulting XML
    std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
            < XML_Incoming > (additionaldata);
    boost::asio::write(plugindata->m_conn->socket(),
            boost::asio::buffer(ss.str()));
}

/**
 * Send a list of devices to the client
 *
 * @param devices		 List of devices
 * @param additionaldata Action data, used for source connection handle
 */
void EventSource_XML_Network::updateDevices (
        std::map<std::string, std::string>& devices,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDevices");
        return;
    }

    pugi::xml_document document;
    pugi::xml_node rootnode = document.append_child("TarantulaDeviceData");

    for (auto item : devices)
    {
        pugi::xml_node devicenode = rootnode.append_child("Device");
        devicenode.append_child("Name").text().set(item.first.c_str());
        devicenode.append_child("Type").text().set(item.second.c_str());
    }

    //Pull out the generated XML as a string
    std::ostringstream ss;
    document.save(ss, "\t", pugi::format_indent);

    std::string xmldata = ss.str();

    //Send back resulting XML
    std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
            < XML_Incoming > (additionaldata);
    boost::asio::write(plugindata->m_conn->socket(),
            boost::asio::buffer(ss.str()));

}

/**
 * Send a list of actions on a device to the client
 *
 * @param device         The device this action list matches to
 * @param actions		 List of available actions
 * @param additionaldata Action data, used for source connection handle
 */
void EventSource_XML_Network::updateDeviceActions (std::string device,
        std::vector<ActionInformation>& actions,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDeviceActions");
        return;
    }

    pugi::xml_document document;
    pugi::xml_node rootnode = document.append_child("TarantulaActionData");

    rootnode.append_attribute("device").set_value(device.c_str());

    for (ActionInformation item : actions)
    {
        pugi::xml_node actionnode = rootnode.append_child("Action");
        actionnode.append_child("ID").text().set(item.actionid);
        actionnode.append_child("Name").text().set(item.name.c_str());
        actionnode.append_child("Description").text().set(
                item.description.c_str());

        pugi::xml_node datanode = actionnode.append_child("Data");

        for (auto dataitem : item.extradata)
        {
            pugi::xml_node itemnode = datanode.append_child("DataItem");
            itemnode.append_attribute("type").set_value(
                    dataitem.second.c_str());
            itemnode.text().set(dataitem.first.c_str());

        }
    }

    //Pull out the generated XML as a string
    std::ostringstream ss;
    document.save(ss, "\t", pugi::format_indent);

    std::string xmldata = ss.str();

    //Send back resulting XML
    std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
            < XML_Incoming > (additionaldata);
    boost::asio::write(plugindata->m_conn->socket(),
            boost::asio::buffer(ss.str()));
}

/**
 * Send a list of EventProcessors and their information
 *
 * @param processors     List of loaded eventprocessors
 * @param additionaldata Action data, used for source connection handle
 */
void EventSource_XML_Network::updateEventProcessors (
        std::map<std::string, ProcessorInformation>& processors,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateEventProcessors");
        return;
    }

    pugi::xml_document document;
    pugi::xml_node rootnode = document.append_child(
            "TarantulaEventProcessorData");

    for (auto item : processors)
    {
        pugi::xml_node processornode = rootnode.append_child("EventProcessor");
        processornode.append_child("Name").text().set(item.first.c_str());
        processornode.append_child("Description").text().set(
                item.second.description.c_str());

        pugi::xml_node datanode = processornode.append_child("Data");

        for (auto dataitem : item.second.data)
        {
            pugi::xml_node itemnode = datanode.append_child("DataItem");
            itemnode.append_attribute("type").set_value(
                    dataitem.second.c_str());
            itemnode.text().set(dataitem.first.c_str());

        }
    }

    //Pull out the generated XML as a string
    std::ostringstream ss;
    document.save(ss, "\t", pugi::format_indent);

    std::string xmldata = ss.str();

    //Send back resulting XML
    std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
            < XML_Incoming > (additionaldata);
    boost::asio::write(plugindata->m_conn->socket(),
            boost::asio::buffer(ss.str()));
}

/**
 * Send a list of files and durations on the specified device to the client
 *
 * @param device 		 Device to get files from
 * @param files  		 Vector of files and durations
 * @param additionaldata Action data, used for source connection handle
 */
void EventSource_XML_Network::updateFiles (std::string device,
		std::vector<std::pair<std::string, int>>& files,
		std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateFiless");
        return;
    }

    pugi::xml_document document;
    pugi::xml_node rootnode = document.append_child("TarantulaFileData");

    rootnode.append_attribute("device").set_value(device.c_str());

    for (std::pair<std::string, int> item : files)
    {
    	pugi::xml_node filenode = rootnode.append_child("File");
		filenode.append_child("Name").text().set(item.first.c_str());

    	if (item.second > 0)
    	{
    		filenode.append_child("Duration").text().set(item.second);
    	}
    }

    //Pull out the generated XML as a string
    std::ostringstream ss;
    document.save(ss, "\t", pugi::format_indent);

    std::string xmldata = ss.str();

    //Send back resulting XML
    std::shared_ptr<XML_Incoming> plugindata = std::static_pointer_cast
            < XML_Incoming > (additionaldata);
    boost::asio::write(plugindata->m_conn->socket(),
            boost::asio::buffer(ss.str()));

}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<EventSource_XML_Network> plugtemp = std::make_shared<EventSource_XML_Network>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}

