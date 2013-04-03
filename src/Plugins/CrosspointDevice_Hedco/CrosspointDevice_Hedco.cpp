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
*   File Name   : CrosspointDevice_Hedco.cpp
*   Version     : 1.0
*   Description : CrosspointDevice plugin for the Hedco crosspoint used by YSTV
*                 (known as the NetMux)
*
*****************************************************************************/


#include <fstream>
#include <iostream>
#include "boost/asio.hpp"
#include "CrosspointDevice_Hedco.h"
#include "CrosspointDevice.h"
#include "Device.h"
#include "Misc.h"
#include "pugixml.hpp"

/**
 * Constructor. Attempts to open a connection to the device over RS-232
 *
 * @param config
 * @param h
 */
CrosspointDevice_Hedco::CrosspointDevice_Hedco (PluginConfig config, Hook h) :
        CrosspointDevice(config, h), m_io(), m_serial(m_io)
{
    // Parse config
    m_portname = config.m_plugindata_xml.child_value("Port");
    if (m_portname.empty())
    {
        m_hook.gs->L->error("Hedco Crosspoint",
                "No serial port given in config!");

        // This is a config glitch, so it will never recover.
        m_status = UNLOAD;
        return;
    }
    m_baudrate = config.m_plugindata_xml.child("Baud").text().as_int(19200);

    // Open serial port
    try
    {
        m_serial.open(m_portname);
    } catch (...)
    {
        m_hook.gs->L->error("Hedco Crosspoint",
                "Unable to open serial port " + m_portname);
        m_status = FAILED;
        return;
    }

    // Set up serial port (8N1 no parity, no flow control)
    m_serial.set_option(boost::asio::serial_port_base::baud_rate(m_baudrate));
    m_serial.set_option(
            boost::asio::serial_port_base::stop_bits(
                    boost::asio::serial_port_base::stop_bits::one));
    m_serial.set_option(boost::asio::serial_port_base::character_size(8));
    m_serial.set_option(
            boost::asio::serial_port_base::parity(
                    boost::asio::serial_port_base::parity::none));
    m_serial.set_option(
            boost::asio::serial_port_base::flow_control(
                    boost::asio::serial_port_base::flow_control::none));

}

CrosspointDevice_Hedco::~CrosspointDevice_Hedco ()
{
    m_connection.close();
}

void CrosspointDevice_Hedco::switchOP (std::string output, std::string input)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state");
        return;
    }

    // Assemble serial command
    std::stringstream commandstring;
    commandstring << "BUFFER 01\r\nBUFFER CLEAR\r\n";
    commandstring << "LEVEL " << m_channeldata[output].videoport << "\r\n";
    commandstring << "XPT " << m_channeldata[input].videoport << ",01\r\n";
    commandstring << "LEVEL " << m_channeldata[output].audioport << "\r\n";
    commandstring << "XPT " << m_channeldata[input].audioport << ",01\r\n";
    commandstring << "EXECUTE 01";

    // Send command
    try
    {
        boost::asio::write(m_serial, boost::asio::buffer(commandstring.str(),
                        commandstring.str().size()));
    }
    catch (std::exception& e)
    {
        m_hook.gs->L->error(m_pluginname, std::string("Error writing data: ") +
                e.what());
        m_status = FAILED;
        return;
    }

    // Update the IO mapping
    m_connectionmap[output] = input;
}

void CrosspointDevice_Hedco::updateHardwareStatus ()
{
    boost::asio::streambuf buffer;

    try
    {
        boost::asio::write(m_serial, boost::asio::buffer("READ\r\n", 6));

        boost::asio::read_until(m_serial, buffer, '>');
        boost::asio::read_until(m_serial, buffer, '>');
    }
    catch (std::exception& e)
    {
        m_hook.gs->L->error(m_pluginname, std::string("Error getting state: ") +
                e.what());
        m_status = FAILED;
        return;
    }

    std::istream inputdata(&buffer);

    while (!inputdata.eof())
    {
        std::string response;
        std::getline(inputdata, response);

        if ((response.length() < 2) || (!response.compare("READ\r")))
        {
            continue;
        }

        if ("Level" != response.substr(4, 5))
        {
            m_hook.gs->L->warn("Hedco Crosspoint",
                    "Bad data received for updateHardwareStatus");
            return;
        }

        int videooutput = ConvertType::stringToInt(response.substr(10, 2));
        int videoinput = ConvertType::stringToInt(response.substr(17, 2));

        std::getline(inputdata, response);

        while (response.length() < 2)
        {
            if (!inputdata.eof())
            {
                std::getline(inputdata, response);
            }
            else
            {
                m_hook.gs->L->error("Hedco Crosspoint", "Bad response data format");
                return;
            }
        }

        int audioinput = ConvertType::stringToInt(response.substr(17, 2));

        if (m_channeldata[m_inputvideotoname[videoinput]].audioport == audioinput)
        {
            m_connectionmap[m_outputvideotoname[videooutput]] =
                    m_inputvideotoname[videoinput];
        }
        else
        {
            m_hook.gs->L->error("Hedco Crosspoint", "Input audio and video does"
                    " not match a channel, has the crosspoint been fiddled manually?");
        }
    }
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new CrosspointDevice_Hedco(config, h);
    }
}

