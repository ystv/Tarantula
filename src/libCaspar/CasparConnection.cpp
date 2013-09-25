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
*   File Name   : CasparConnection.cpp
*   Version     : 1.0
*   Description : Handles connecting to and passing data to and from CasparCG
*
*****************************************************************************/


#include <libCaspar/libCaspar.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Misc.h"

/**
 * Open a connection to a CasparCG server
 *
 * @param host           The hostname/IP of the server
 * @param port           Port number to connect to
 * @param connecttimeout Number of ticks before connection times out
 */
CasparConnection::CasparConnection (std::string host, std::string port, long int connecttimeout) :
    m_io_service(),
    m_socket(m_io_service)
{
    /* Resolve the host */
    boost::asio::ip::tcp::resolver res(m_io_service);
    boost::asio::ip::tcp::resolver::query resq(host, port);
    boost::asio::ip::tcp::resolver::iterator resiter = res.resolve(resq);

    m_connecttimeout = connecttimeout;

    /* Spawn off an async connect operation */
    boost::asio::async_connect(m_socket, resiter,
            boost::bind(&CasparConnection::cb_connect, this, boost::asio::placeholders::error));

    m_connectstate = false;
    m_errorflag = false;
    m_badcommandflag = false;
}

CasparConnection::~CasparConnection ()
{
}


/**
 * Read a line from the open socket to CasparCG
 *
 * @return String of data, or blank string for no data
 */
std::string CasparConnection::receiveLine ()
{
    if (m_datalines.size() > 0)
    {
        std::string ret = m_datalines.back();
        m_datalines.pop_back();
        return ret;
    }
    else
    {
        return "";
    }
}

/**
 * Check socket is still alive and progress waiting async operations
 *
 * @return True if socket is still alive
 */
bool CasparConnection::tick ()
{
    if (m_socket.is_open())
    {
        m_io_service.poll();
        m_io_service.reset();

        if (!m_connectstate)
        {
            m_connecttimeout--;
        }

        if (0 == m_connecttimeout)
        {
            m_errorflag = true;
            return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Timeout handler for run()
 */
void CasparConnection::runTimeout ()
{
    m_io_service.stop();
}

/**
 * Keep running the io_service until we run out of work or timeout
 * @param timeout Time in milliseconds to run for. Defaults to 1000. -1 will run until work runs out
 */
void CasparConnection::run (int timeout /* = 1000 */)
{
    if (timeout > -1)
    {
        boost::asio::deadline_timer tmr(m_io_service);
        tmr.expires_from_now(boost::posix_time::milliseconds(timeout));

        tmr.async_wait(boost::bind(&CasparConnection::runTimeout, this));
    }

    m_io_service.run();

    m_io_service.reset();
}

/**
 * Send a CasparCommand to CasparCG
 *
 * @param cmd     A command to be executed
 */
void CasparConnection::sendCommand (CasparCommand cmd)
{
    // Queue prevents multiple commands running in parallel and confusion of results
    m_commandqueue.push(cmd);

    // If this is the only command, send it
    if (1 == m_commandqueue.size())
    {
        const char *buffer = cmd.form().c_str();

        boost::asio::async_write(m_socket, boost::asio::buffer(buffer, strlen(buffer)),
                boost::bind(&CasparConnection::cb_write, this, boost::asio::placeholders::error));
    }
}

/**
 * Returns a string representing a response code
 *
 * @param responsecode Response code to analyse
 * @return             Human-readable version of response code.
 */
std::string CasparConnection::queryResponse (int responsecode)
{

    switch (responsecode)
    {
        case E_NO_RESPONSE:
            return "No response fed to the casparResponse object yet!";

        case E_COMMAND_NOT_UNDERSTOOD:
            return "Command not understood";

        case E_ILLEGAL_CHANNEL:
            return "Illegal channel";

        case E_PARAMETER_MISSING:
            return "Parameter missing";

        case E_ILLEGAL_PARAMETER:
            return "Illegal parameter";

        case E_FILE_NOT_FOUND:
            return "Media file not found";

        case E_SERVER_FAILED:
            return "Internal server error";

        case E_COMMAND_FAILED:
            return "Internal server error";

        case E_FILE_UNREADABLE:
            return "Media file unreadable";

        case S_INFORMATION_RETURNED:
            return "Information about an event.";

        case S_DATA_RETURNED:
            return "Information about an event. A line of data is being returned.";

        case S_COMMAND_EXECUTED:
            return "The command has been executed";

        case S_COMMAND_EXECUTED_WITH_DATA:
            return "The command has been executed and a line of data is being returned";

        case S_COMMAND_EXECUTED_WITH_MANY_DATA:
            return "The command has been executed and several lines of data are being returned (terminated by an empty line.)";

        default:
            return "Caspar response code not found!";
    }
}

/**
 * Handle completion of connection attempt
 *
 * @param err System error code on failure
 */
void CasparConnection::cb_connect (const boost::system::error_code& err)
{
    if (!err)
    {
        m_connectstate = true;
    }
    else
    {
        m_errorflag = true;
    }
}

/**
 * Callback after a write has completed, sets up a read
 *
 * @param err System error code if one ocurred
 */
void CasparConnection::cb_write (const boost::system::error_code& err)
{
    if (!err)
    {
        boost::asio::async_read_until(m_socket, m_recvdata, "\n",
                boost::bind(&CasparConnection::cb_firstread, this, boost::asio::placeholders::error));
    }
    else
    {
        m_errorflag = true;
    }
}

/**
 * Handle reading the initial line of a response, work out if an error ocurred or further reads are needed
 *
 * @param err System error code if one ocurred
 */
void CasparConnection::cb_firstread (const boost::system::error_code& err)
{
    if (!err)
    {
        std::istream is(&m_recvdata);
        std::string line;
        std::getline(is, line);

        std::string respCode = line.substr(0, 3);
        int responsecode = ConvertType::stringToInt(respCode.c_str());

        // Check whether further data is expected
        if (responsecode == E_COMMAND_NOT_UNDERSTOOD ||
                responsecode == S_DATA_RETURNED ||
                responsecode == S_COMMAND_EXECUTED_WITH_MANY_DATA ||
                responsecode == S_COMMAND_EXECUTED_WITH_DATA)
        {
            // Store the first line
            m_datalines.push_back(line);


            // Process additional data and kick off another read if needed
            processData(is);
        }
        else if (responsecode != S_INFORMATION_RETURNED &&
                    responsecode != S_COMMAND_EXECUTED)
        {
            // An error occurred, set the error flag
            m_badcommandflag = true;

            sendQueuedCommand();
        }
        else
        {
            sendQueuedCommand();
        }
    }
    else
    {
        m_errorflag = true;
    }
}

/**
 * Read the remaining lines of data and call a handler function
 *
 * @param err System error code if one ocurred
 */
void CasparConnection::cb_doneread (const boost::system::error_code& err)
{
    if (!err)
    {
        // Grab all the data from the read buffer
        std::istream is(&m_recvdata);

        // Process the data
        processData(is);
    }
    else
    {
        m_errorflag = true;
    }
}

/**
 * Process lines of incoming data and start another read if needed
 *
 * @param is Input stream of data for processing
 */
void CasparConnection::processData (std::istream& is)
{
    std::string line;

    // Append the first line to the preceeding
    if (std::getline(is, line))
    {
        // Append if line was incomplete
        if (m_datalines.back().find("\r") == std::string::npos)
        {
            m_datalines.back() += line;
        }
        else
        {
            m_datalines.push_back(line);
        }
    }

    while (std::getline(is, line))
    {
        m_datalines.push_back(line);
    }

    // If the last line was not a blank marking the end of data, kick off another read
    if (m_datalines.back() != "\r")
    {
        boost::asio::async_read_until(m_socket, m_recvdata, "\n",
                boost::bind(&CasparConnection::cb_doneread, this, boost::asio::placeholders::error));
    }
    // Otherwise call the handler
    else
    {
        ResponseHandler handler = m_commandqueue.front().getHandler();

        handler(m_datalines);

        sendQueuedCommand();
    }
}

/**
 * Clear out the buffers and send another command if one is ready
 */
void CasparConnection::sendQueuedCommand()
{
    m_recvdata.consume(m_recvdata.size());

    m_datalines.clear();

    // Remove the command from the queue
    m_commandqueue.pop();

    // Send a new command
    if (m_commandqueue.size() > 0)
    {
        const char *buffer = m_commandqueue.front().form().c_str();

        boost::asio::async_write(m_socket, boost::asio::buffer(buffer, strlen(buffer)),
                boost::bind(&CasparConnection::cb_write, this, boost::asio::placeholders::error));
    }
}
