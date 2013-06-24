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
*   File Name   : EventSource_Web.cpp
*   Version     : 1.0
*   Description : An EventSource to run an HTTP interface on port 9816.
*
*****************************************************************************/

#include "EventSource_Web.h"
#include "pugixml.hpp"
#include "PlaylistDB.h"
#include "Misc.h"
#include "HTTPConnection.h"

/**
 * Constructor. Reads configuration data into plugin and opens listening sockets
 *
 * @param config Configuration loaded from XML config file
 * @param h      Link back to core structures
 */
EventSource_Web::EventSource_Web (PluginConfig config, Hook h) :
        MouseCatcherSourcePlugin(config, h),
        m_io_service(new boost::asio::io_service),
        m_acceptor(*m_io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9816)),
        m_pevents(std::make_shared<std::set<MouseCatcherEvent, WebSource::MCE_compare>>()),
        m_psnippets(std::make_shared<WebSource::HTMLSnippets>())
{
	// Read configuration
	try
	{
		m_config.m_webpath = config.m_plugindata_map.at("IndexPath");
	}
	catch (std::exception &)
	{
		h.gs->L->error(m_pluginname, "IndexPath not found. Shutting down");
		m_status = FAILED;
		return;
	}

	try
	{
		m_config.m_channel = config.m_plugindata_map.at("Channel");
	}
	catch (std::exception &)
	{
		h.gs->L->error(m_pluginname, "Channel not found. Shutting down");
		m_status = FAILED;
		return;
	}


	try
	{
		m_config.m_pollperiod =
				ConvertType::stringToInt(config.m_plugindata_map.at("PollPeriod"));
	}
	catch (std::exception &)
	{
		h.gs->L->error(m_pluginname, "PollPeriod not found. Shutting down");
		m_status = FAILED;
		return;
	}
	m_last_update_time = 0;
	m_polltime = m_config.m_pollperiod;

	try
	{
		m_config.m_port =
				ConvertType::stringToInt(config.m_plugindata_map.at("Port"));
	}
	catch (std::exception &)
	{
		h.gs->L->error(m_pluginname, "Port not found. Shutting down");
		m_status = FAILED;
		return;
	}

	m_config.m_hook = m_hook;

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

/**
 * Destructor. Shut down the HTTP socket
 */
EventSource_Web::~EventSource_Web ()
{
    // Destroy the IOservice object
    while (m_io_service != NULL)
    {
        m_io_service.reset();
    };
}

/**
 * Make ready to receive a new connection.
 */
void EventSource_Web::startAccept ()
{
    std::shared_ptr<WebSource::HTTPConnection> new_connection =
    		WebSource::HTTPConnection::create(
            m_acceptor.get_io_service(), m_pevents, m_psnippets, m_config);

    m_acceptor.async_accept(new_connection->socket(),
            boost::bind(&EventSource_Web::handleAccept, this,
                    new_connection, boost::asio::placeholders::error));

    m_status = READY;
}

/**
 * Handle a new connection and make ready to receive another.
 *
 * @param new_connection
 * @param error
 */
void EventSource_Web::handleAccept (
        std::shared_ptr<WebSource::HTTPConnection> new_connection,
        const boost::system::error_code& error)
{
    if (!error)
    {
        new_connection->start();
    }

    startAccept();
}

/**
 * Plugin processing routine.
 *
 * @param ActionQueue Pointer to the central queue of event actions.
 */
void EventSource_Web::tick (std::vector<EventAction>* ActionQueue)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state");
        return;
    }

    // Process ready asynchronous tasks
    m_io_service->poll();

    EventAction updateaction;

    m_polltime++;
    if (m_polltime > m_config.m_pollperiod)
    {
    	EventAction updateaction;
    	m_psnippets->m_devices.clear();
    	m_psnippets->m_deviceactions.clear();
    	updateaction.action = ACTION_UPDATE_DEVICES;
    	m_psnippets->m_localqueue.push_back(updateaction);

    	updateaction.action = ACTION_UPDATE_PROCESSORS;
    	m_psnippets->m_localqueue.push_back(updateaction);

    	m_polltime = 0;
    }

    // Process the local action queue
    std::vector<EventAction>::iterator it = std::remove_if(ActionQueue->begin(),
		ActionQueue->end(),
		boost::bind(&MouseCatcherSourcePlugin::actionCompleteCheck, _1,
				dynamic_cast<MouseCatcherSourcePlugin*>(this)));

    ActionQueue->erase(it, ActionQueue->end());

    for (auto thisaction : m_psnippets->m_localqueue)
    {
    	thisaction.thisplugin = this;
    	thisaction.isprocessed = false;
    	ActionQueue->push_back(thisaction);
    }

    m_psnippets->m_localqueue.clear();
}

/**
 * Callback to update the internal playlist
 *
 * @param playlist
 * @param additionaldata Unused
 */
void EventSource_Web::updatePlaylist (
        std::vector<MouseCatcherEvent>& playlist,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updatePlaylist");
        return;
    }

    // Extract the additionaldata structure into a real form
    std::shared_ptr<WebSource::EventActionData> ead = std::static_pointer_cast <WebSource::EventActionData> (additionaldata);

    // Generate and format XHTML for the playlist
    pugi::xml_node schedulenode = ead->data.document_element();
    for (MouseCatcherEvent currentevent : playlist)
    {
        generateScheduleSegment(currentevent, schedulenode);
    }

    // Only send back the HTML if this was a standalone request
    if (!ead->attachedrequest)
    {
        std::stringstream html;
        schedulenode.print(html);
        ead->connection->m_reply.content = html.str();
        ead->connection->m_reply.status = http::server3::reply::ok;
        ead->connection->commitResponse();
    }

    ead->complete = true;
}

/**
 * Callback to update the internal device list
 *
 * @param devices
 * @param additionaldata Unused
 */
void EventSource_Web::updateDevices (
        std::map<std::string, std::string>& devices,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDevices");
        return;
    }

    EventAction devicedetails;
    devicedetails.action = ACTION_UPDATE_ACTIONS;
    devicedetails.isprocessed = false;
    devicedetails.thisplugin = this;

    for (auto thisdevice : devices)
    {

    	if (0 == m_psnippets->m_deviceactions.count(thisdevice.second))
    	{
    		// This is the first device of its type, get a list of actions
			devicedetails.event.m_targetdevice = thisdevice.first;
			m_psnippets->m_localqueue.push_back(devicedetails);

			// Create a new entry in the actions list
			m_psnippets->m_deviceactions[thisdevice.second].m_pdevices =
					std::make_shared<pugi::xml_document>();
			m_psnippets->m_deviceactions[thisdevice.second].m_isprocessor = false;
			m_psnippets->m_deviceactions[thisdevice.second].m_pdevices->
				append_child("select").append_attribute("name").set_value("device");

			pugi::xml_node device =
					m_psnippets->m_deviceactions[thisdevice.second].m_pdevices->
					document_element().append_child("option");
			device.append_attribute("value").set_value(thisdevice.first.c_str());
			device.text().set(thisdevice.first.c_str());
    	}
    	else
    	{
    		// Just add the device to the list
    		pugi::xml_node device = m_psnippets->m_deviceactions[thisdevice.second].
    				m_pdevices->document_element().append_child("option");
			device.append_attribute("value").set_value(thisdevice.first.c_str());
			device.text().set(thisdevice.first.c_str());
    	}

    }

    m_psnippets->m_devices = devices;

    // Get files for this device
    EventAction files;
    files.action = ACTION_UPDATE_FILES;
    for (auto device : devices)
    {
    	if (device.second == "Video")
    	{
			files.event.m_targetdevice = device.first;
			m_psnippets->m_localqueue.push_back(files);
    	}
    }

}

/**
 * Update internal list of actions for each device
 *
 * @param device
 * @param actions
 * @param additionaldata Unused
 */
void EventSource_Web::updateDeviceActions (std::string device,
        std::vector<ActionInformation>& actions,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateDeviceActions");
        return;
    }

    // Insert new actions into relevant list
    std::string devicetype = m_psnippets->m_devices[device];
    m_psnippets->m_deviceactions[devicetype].m_actions = actions;

    // Generate the action selection box and subforms
    m_psnippets->m_deviceactions[devicetype].m_pactionsnippet =
			std::make_shared<pugi::xml_document>();

    pugi::xml_node actionsnode = m_psnippets->m_deviceactions[devicetype].
    		m_pactionsnippet->append_child("p");

    pugi::xml_node label = actionsnode.append_child("label");
    label.text().set("Action");
    label.append_attribute("for").set_value("action");
    pugi::xml_node input = actionsnode.append_child("select");
    input.append_attribute("name").set_value(
    		std::string("action-" + devicetype).c_str());
    input.append_attribute("class").set_value("action-select");

    for (auto thisaction : actions)
    {
    	// Add item to the dropdown
    	pugi::xml_node item = input.append_child("option");
    	item.append_attribute("value").set_value(thisaction.actionid);
    	item.text().set(thisaction.name.c_str());

    	pugi::xml_node subform = actionsnode.append_child("div");
    	subform.append_attribute("id").set_value(
    			std::string("action-form-" + devicetype + "-" +
    			ConvertType::intToString(thisaction.actionid)).c_str());
    	subform.append_attribute("class").set_value("action-form");

    	// Add the action options
    	for (auto thisoption : thisaction.extradata)
    	{
    		if (!thisoption.second.substr(thisoption.second.length()-3, 3).compare("..."))
    		{
    			//!TODO Write the handler for key-value maps
    		}
    		else if (!thisoption.first.compare("filename"))
    		{
    			pugi::xml_node para = subform.append_child("p");
    			para.append_attribute("class").set_value("form-line");

    			label = para.append_child("label");
    			label.text().set(thisoption.first.c_str());
				label.append_attribute("for").set_value(
						std::string("action-" +
								ConvertType::intToString(thisaction.actionid) +
								"-" + thisoption.first).c_str());

				item = para.append_child("select");
				item.append_attribute("name").set_value(
						std::string("action-" +
								ConvertType::intToString(thisaction.actionid) +
								"-" + thisoption.first).c_str());
				item.append_attribute("class").set_value("action-filename action-data-input");
    		}
    		else
    		{
    			pugi::xml_node para = subform.append_child("p");
    			para.append_attribute("class").set_value("form-line");

    			label = para.append_child("label");
    			label.text().set(thisoption.first.c_str());
    			label.append_attribute("for").set_value(
    					std::string("action-" +
    							ConvertType::intToString(thisaction.actionid) +
    							"-" + thisoption.first).c_str());

    			item = para.append_child("input");
    			item.append_attribute("name").set_value(
    					std::string("action-" +
    							ConvertType::intToString(thisaction.actionid) +
    							"-" + thisoption.first).c_str());
    			item.append_attribute("class").set_value(
    					std::string("actioninput-" + thisoption.second +
    							"action-data-input").c_str());

    		}
    	}
    }


}

/**
 * Update internal list of EventProcessors
 *
 * @param processors
 * @param additionaldata
 */
void EventSource_Web::updateEventProcessors (
        std::map<std::string, ProcessorInformation>& processors,
        std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateEventProcessors");
        return;
    }

    for (auto thisprocessor: processors)
    {
    	// Add new entry
		m_psnippets->m_deviceactions[thisprocessor.first].m_isprocessor = true;
		m_psnippets->m_devices[thisprocessor.first] = thisprocessor.first;

		// Generate the subforms
		m_psnippets->m_deviceactions[thisprocessor.first].m_pactionsnippet =
				std::make_shared<pugi::xml_document>();

		pugi::xml_node actionsnode = m_psnippets->m_deviceactions[thisprocessor.first].
				m_pactionsnippet->append_child("p");


		pugi::xml_node subform = actionsnode.append_child("div");
		subform.append_attribute("id").set_value(
				std::string("action-form-" + thisprocessor.first +
						"-0").c_str());
		subform.append_attribute("class").set_value("action-form");

		// Add the action options
		for (auto thisoption : thisprocessor.second.data)
		{
			if (!thisoption.second.substr(thisoption.second.length()-3, 3).compare("..."))
			{
				//!TODO Write the handler for key-value maps
			}
			else if (!thisoption.first.compare("filename"))
			{
				pugi::xml_node para = subform.append_child("p");
				para.append_attribute("class").set_value("form-line");

				pugi::xml_node label = para.append_child("label");
				label.text().set(thisoption.first.c_str());
				label.append_attribute("for").set_value(
						std::string("action-0-" + thisoption.first).c_str());

				pugi::xml_node item = para.append_child("select");
				item.append_attribute("name").set_value(
						std::string("actionfile-0").c_str());
				item.append_attribute("class").set_value("action-filename");
			}
			else
			{
				pugi::xml_node para = subform.append_child("p");
				para.append_attribute("class").set_value("form-line");

				pugi::xml_node label = para.append_child("label");
				label.text().set(thisoption.first.c_str());
				label.append_attribute("for").set_value(
						std::string("action-0-" + thisoption.first).c_str());

				pugi::xml_node item = para.append_child("input");
				item.append_attribute("name").set_value(
						std::string("action-0-" + thisoption.first).c_str());
				item.append_attribute("class").set_value(
						std::string("actioninput-" + thisoption.second).c_str());

			}
		}

    }

}

/**
 * Send a list of files and durations on the specified device to the client
 *
 * @param device 		 Device to get files from
 * @param files  		 Vector of files and durations
 * @param additionaldata Action data, used for source plugin handle
 */
void EventSource_Web::updateFiles (std::string device,
		std::vector<std::pair<std::string, int>>& files,
		std::shared_ptr<void> additionaldata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateFiless");
        return;
    }

    m_psnippets->m_files[device].clear();
    m_psnippets->m_files[device] = files;

}

/**
 * Free function generates a table row from a key-value pair
 *
 * @param parent Parent node to insert table
 * @param key
 * @param value
 */
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
void EventSource_Web::generateScheduleSegment(MouseCatcherEvent& targetevent,
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

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new EventSource_Web(config, h);
    }

}

