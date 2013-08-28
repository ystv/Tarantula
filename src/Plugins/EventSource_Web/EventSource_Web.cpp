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
#include "DateConversions.h"

/**
 * Constructor. Reads configuration data into plugin and opens listening sockets
 *
 * @param config Configuration loaded from XML config file
 * @param h      Link back to core structures
 */
EventSource_Web::EventSource_Web (PluginConfig config, Hook h) :
        MouseCatcherSourcePlugin(config, h),
        m_io_service(new boost::asio::io_service),
        m_pevents(std::make_shared<std::set<MouseCatcherEvent, WebSource::MCE_compare>>()),
        m_sharedata(std::make_shared<WebSource::ShareData>())
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
		m_config.m_pollperiod = ConvertType::stringToInt(config.m_plugindata_map.at("PollPeriod"));
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
		m_config.m_port = ConvertType::stringToInt(config.m_plugindata_map.at("Port"));
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
        m_acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(
                *m_io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), m_config.m_port));
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
            m_acceptor->get_io_service(), m_pevents, m_sharedata, m_config);

    m_acceptor->async_accept(new_connection->socket(),
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

    // Process WaitingRequests
    for (std::shared_ptr<WebSource::WaitingRequest> req : m_sharedata->m_requests)
    {
        if (std::find_if(req->actions.begin(), req->actions.end(),
                [](const std::shared_ptr<WebSource::EventActionData> &ead)
                { return ead->complete == false; }) == req->actions.end())
        {
            generateSchedulePage(req);
            req->connection->commitResponse("application/xhtml+xml");

            req->complete = true;
        }
    }

    m_sharedata->m_requests.erase(std::remove_if(m_sharedata->m_requests.begin(), m_sharedata->m_requests.end(),
            [](const std::shared_ptr<WebSource::WaitingRequest> &req)
            { return req->complete == true; }), m_sharedata->m_requests.end());

    // Process the local action queue
    std::vector<EventAction>::iterator it = std::remove_if(ActionQueue->begin(),
		ActionQueue->end(),
		boost::bind(&MouseCatcherSourcePlugin::actionCompleteCheck, _1,
				dynamic_cast<MouseCatcherSourcePlugin*>(this)));

    ActionQueue->erase(it, ActionQueue->end());

    for (auto thisaction : m_sharedata->m_localqueue)
    {
    	thisaction.thisplugin = this;
    	thisaction.isprocessed = false;
    	ActionQueue->push_back(thisaction);
    }

    m_sharedata->m_localqueue.clear();
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
    ead->complete = true;

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

    // Extract the additionaldata structure into a real form
    std::shared_ptr<WebSource::EventActionData> ead = std::static_pointer_cast <WebSource::EventActionData> (additionaldata);
    ead->complete = true;

    // Prepare an XML node
    pugi::xml_node devicenode = ead->data.append_child("devices");


    EventAction devicedetails;
    devicedetails.action = ACTION_UPDATE_ACTIONS;
    devicedetails.isprocessed = false;
    devicedetails.thisplugin = this;

    for (auto thisdevice : devices)
    {

    	if (NULL == devicenode.find_child_by_attribute("type", thisdevice.second.c_str()))
    	{
    		// This is the first device of its type, get a list of actions
			devicedetails.event.m_targetdevice = thisdevice.first;

            std::shared_ptr<WebSource::EventActionData> uead = std::make_shared<WebSource::EventActionData>();
            uead->complete = false;
            uead->type = WebSource::WEBACTION_ALL;
            uead->connection = std::shared_ptr<WebSource::HTTPConnection>(ead->connection);
            uead->attachedrequest.reset();
            uead->attachedrequest = std::shared_ptr<WebSource::WaitingRequest>(ead->attachedrequest);

            // Assemble a header for the action data
            pugi::xml_node actiondatanode = uead->data.append_child("actiondata");
            actiondatanode.append_attribute("type").set_value(thisdevice.second.c_str());

            // Add additional data to the request
            devicedetails.additionaldata.reset();
            devicedetails.additionaldata = std::shared_ptr<WebSource::EventActionData>(uead);

            // Add request to the queue
            m_sharedata->m_localqueue.push_back(devicedetails);
            ead->attachedrequest->actions.push_back(uead);

			// Create a new entry in the actions list
            pugi::xml_node typenode = devicenode.append_child("devicetype");
            typenode.append_attribute("type").set_value(thisdevice.second.c_str());

            pugi::xml_node selnode = typenode.append_child("select");
            selnode.append_attribute("name").set_value("device");

			pugi::xml_node device = selnode.append_child("option");
			device.append_attribute("value").set_value(thisdevice.first.c_str());
			device.text().set(thisdevice.first.c_str());
    	}
    	else
    	{
    		// Just add the device to the list
    		pugi::xml_node device = devicenode.find_child_by_attribute("type", thisdevice.second.c_str()).
    		        first_child().append_child("option");
			device.append_attribute("value").set_value(thisdevice.first.c_str());
			device.text().set(thisdevice.first.c_str());
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

    // Extract the additionaldata structure into a real form
    std::shared_ptr<WebSource::EventActionData> ead = std::static_pointer_cast <WebSource::EventActionData> (additionaldata);
    ead->complete = true;

    // Prepare an XML node
    pugi::xml_node rootnode = ead->data.document_element();

    std::string devicetype = rootnode.attribute("type").as_string();

    pugi::xml_node actionsnode = rootnode.append_child("p");

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

    // Extract the additionaldata structure into a real form
    std::shared_ptr<WebSource::EventActionData> ead = std::static_pointer_cast <WebSource::EventActionData> (additionaldata);
    ead->complete = true;

    pugi::xml_node rootnode = ead->data.append_child("processordata");

    for (auto thisprocessor: processors)
    {
        // Replace spaces in the type name with underscores
        std::string cleanname = thisprocessor.first;
        std::transform(cleanname.begin(), cleanname.end(), cleanname.begin(),
                [](char ch)
                {
                    return ch == ' ' ? '_' : ch;
                });

    	// Add new entry
		pugi::xml_node procnode = rootnode.append_child("processor");
		procnode.append_attribute("name").set_value(thisprocessor.first.c_str());

		pugi::xml_node actionsnode = procnode.append_child("p");
		actionsnode.append_attribute("class").set_value("procs-form");

		// Add the action options
		for (auto thisoption : thisprocessor.second.data)
		{
			if (!thisoption.second.substr(thisoption.second.length()-3, 3).compare("..."))
			{
				//!TODO Write the handler for key-value maps
			}
			else
			{
				pugi::xml_node para = actionsnode.append_child("p");
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

    // Extract the additionaldata structure into a real form
    std::shared_ptr<WebSource::EventActionData> ead = std::static_pointer_cast <WebSource::EventActionData> (additionaldata);

    pugi::xml_node rootnode = ead->data.document_element();

    for (auto fileitem : files)
    {
        pugi::xml_node thisfile = rootnode.append_child("option");
        thisfile.append_attribute("value").set_value(fileitem.first.c_str());
        thisfile.append_attribute("tar-length").set_value(fileitem.second);
        thisfile.text().set(std::string(fileitem.first + " - " +
                ConvertType::intToString(fileitem.second/25) +
                " seconds").c_str());
    }

    // Only send back the HTML if this was a standalone request
    if (!ead->attachedrequest)
    {
        std::stringstream html;
        rootnode.print(html);
        ead->connection->m_reply.content = html.str();
        ead->connection->m_reply.status = http::server3::reply::ok;
        ead->connection->commitResponse();
    }

    ead->complete = true;

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

    // Headings and setup
    pugi::xml_node headingnode = parent.append_child("h3");
    headingnode.append_attribute("id").set_value(std::string("eventhead-" +
            ConvertType::intToString(targetevent.m_eventid)).c_str());

    headingnode.text().set(std::string(std::string(eventstart_buffer) + " - " +
            std::string(eventend_buffer) + "  " + targetevent.m_targetdevice).c_str());

    pugi::xml_node datadiv = parent.append_child("div");
    pugi::xml_node datatable = datadiv.append_child("table");

    // Fill table contents for parent event
    rowgenerate(datatable, "Channel", targetevent.m_channel);
    rowgenerate(datatable, "Description", targetevent.m_description);
    rowgenerate(datatable, "Device", targetevent.m_targetdevice);

    // Get the name of the action
    if (-1 == targetevent.m_action)
    {
        rowgenerate(datatable, "Action", "Event Processor");
    }
    else
    {
        rowgenerate(datatable, "Action", targetevent.m_action_name);
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

    rowgenerate(datatable, "PreProcessor", targetevent.m_preprocessor);

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
    //add_after_data.text().set(devicetype.c_str());

    pugi::xml_node add_after_link = add_after.append_child("a");
    add_after_link.append_attribute("href").set_value("");
    add_after_link.append_attribute("class").set_value("add-after-link");
    add_after_link.text().set("Add event after this");

}

/**
 * Generate the interface page based on a WaitingRequest
 *
 * @param req WaitingRequest that triggered all the lookups
 */
void EventSource_Web::generateSchedulePage (std::shared_ptr<WebSource::WaitingRequest> req)
{
    pugi::xml_document pagedocument;

    pugi::xml_parse_result result = pagedocument.load_file(
            std::string(m_config.m_webpath + "/index.html").c_str());

    // Return an error if parsing fails
    if (pugi::status_ok != result.status)
    {
        req->connection->m_reply =
                http::server3::reply::stock_reply(http::server3::reply::internal_server_error);
        return;
    }

    // Assemble some data
    std::map<std::string, WebSource::typedata> actionlist;

    // Get the list of devices
    std::vector<std::shared_ptr<WebSource::EventActionData>>::iterator it =
            std::find_if(req->actions.begin(), req->actions.end(),
                    [](const std::shared_ptr<WebSource::EventActionData> &dat)
                    { return !std::string(dat->data.document_element().name()).compare("devices"); });
    pugi::xml_node devnode = (*it)->data.document_element();

    // Extract a set of device types and devices
    for (pugi::xml_node devtype : devnode.children())
    {
        WebSource::typedata thisdev;
        thisdev.m_pdevices = std::make_shared<pugi::xml_document>();
        thisdev.m_pactionsnippet = std::make_shared<pugi::xml_document>();
        thisdev.m_pdevices->append_copy(devtype.first_child());
        thisdev.m_isprocessor = false;

        actionlist[devtype.attribute("type").as_string()] = thisdev;
    }

    // Add action data to set of devices
    for (std::shared_ptr<WebSource::EventActionData> actiondata : req->actions)
    {
        pugi::xml_node rootnode = actiondata->data.document_element();

        if (!std::string(rootnode.name()).compare("actiondata"))
        {
            std::string actiontype = rootnode.attribute("type").as_string();

            actionlist[actiontype].m_pactionsnippet->append_copy(rootnode.first_child());
        }
        else if (!std::string(rootnode.name()).compare("processordata"))
        {
            for (pugi::xml_node processornode : rootnode.children())
            {
                WebSource::typedata thisproc;

                thisproc.m_pdevices = std::make_shared<pugi::xml_document>();
                thisproc.m_pactionsnippet = std::make_shared<pugi::xml_document>();
                //thisdev.m_pdevices->append_copy(rootnode.first_child());
                thisproc.m_isprocessor = true;
                thisproc.m_pactionsnippet->append_copy(processornode.first_child());

                actionlist[processornode.attribute("name").as_string()] = thisproc;
            }
        }
    }

    // Generate the event type related items
    for (auto devicetype : actionlist)
    {
        // Replace spaces in the type name with underscores
        std::string cleanname = devicetype.first;
        std::transform(cleanname.begin(), cleanname.end(), cleanname.begin(),
                [](char ch)
                {
                    return ch == ' ' ? '_' : ch;
                });

        // Add the Add button
        pugi::xml_node addbutton = pagedocument.select_single_node(
                "//div[@id='add-event-bounding']").node().append_child("button");

        addbutton.append_attribute("class").set_value("button-big margin-bottom addbutton");
        addbutton.append_attribute("id").set_value(cleanname.c_str());
        addbutton.text().set(devicetype.first.c_str());

        // Add the event type drop-down option
        pugi::xml_node dropdownoption = pagedocument.select_single_node(
                "//select[@name='type']").node().append_child("option");
        dropdownoption.append_attribute("value").set_value(cleanname.c_str());
        dropdownoption.text().set(devicetype.first.c_str());

        // Generate a sub-form for this type
        pugi::xml_node subform = pagedocument.select_single_node(
                "//div[@id='add-form']/form").node().append_child("div");
        subform.append_attribute("id").set_value(std::string("add-" +
                cleanname).c_str());
        subform.append_attribute("class").set_value("add-sub formitem");

        // Add the Device drop-down
        pugi::xml_node para = subform.append_child("p");
        para.append_attribute("class").set_value("form-line");
        pugi::xml_node label = para.append_child("label");

        if (!devicetype.second.m_isprocessor)
        {
            label.append_attribute("for").set_value("device");
            label.text().set("Device");
            para.append_copy(devicetype.second.m_pdevices->document_element());
        }

        // Add the description field
        para = subform.append_child("p");
        para.append_attribute("class").set_value("form-line");

        label = para.append_child("label");
        label.append_attribute("for").set_value("time");
        label.text().set("Description");
        pugi::xml_node descinput = para.append_child("input");
        descinput.append_attribute("class").set_value("form-input");
        descinput.append_attribute("name").set_value("description");

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
        subform.append_copy(devicetype.second.m_pactionsnippet->document_element());

    }

    // Set the date in the header and datepicker
    std::string headerdate = DateConversions::gregorianDateToString(req->requesteddate, "%A %d %B %Y");
    pagedocument.select_single_node("//h2[@id='dateheader']").node().text().set(headerdate.c_str());

    headerdate = DateConversions::gregorianDateToString(req->requesteddate, "%Y, %M, %D");
    pagedocument.select_single_node("//div[@id='currentdate']").node().text().set(headerdate.c_str());


    // Find the schedule data
    std::vector<std::shared_ptr<WebSource::EventActionData>>::iterator it2 =
            std::find_if(req->actions.begin(), req->actions.end(),
                [](const std::shared_ptr<WebSource::EventActionData> &dat)
                { return !std::string(dat->data.document_element().name()).compare("div"); });
    pugi::xml_node scheduledata = (*it2)->data.document_element();

    // Insert the playlist data
    for (pugi::xml_node scheditem : scheduledata.children())
    {
        pagedocument.select_single_node("//div[@id='scheduledata']").node().append_copy(scheditem);
    }

    // Dump the HTML to the reply
    std::stringstream html;
    pagedocument.save(html, "", pugi::format_no_declaration);
    req->connection->m_reply.content = html.str();
    req->connection->m_reply.status = http::server3::reply::ok;
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<EventSource_Web> plugtemp = std::make_shared<EventSource_Web>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}

