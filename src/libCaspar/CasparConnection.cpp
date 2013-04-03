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
#include "Misc.h"

/**
 * Default constructor
 */
CasparConnection::CasparConnection ()
{

}

/**
 * Open a connection to a CasparCG server
 *
 * @param The hostname of the server
 */
CasparConnection::CasparConnection (std::string host)
{
    m_skt = socket(PF_INET, SOCK_STREAM, 0);

    // Bind the socket on the local side
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = INADDR_ANY;
    sin.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(m_skt, reinterpret_cast<struct sockaddr *> (&sin), sizeof(sin));

    // Bind the remote side of the socket
    sockaddr_in rem;
    rem.sin_family = AF_INET;
    rem.sin_port = htons(5250); //5250 is the caspar port
    rem.sin_addr.s_addr = inet_addr(host.c_str());
    ret = connect(m_skt, reinterpret_cast<const struct sockaddr*> (&rem), sizeof(rem));
    if (ret > -1)
        m_connected = true;
    else
        m_connected = false;
}

CasparConnection::~CasparConnection ()
{
    close(m_skt);
}

/**
 * Read a line from the open socket to CasparCG
 */
std::string CasparConnection::receiveLine ()
{
    if (!m_connected)
        return ""; // No point trying if it's not connected!
    char buffer;
    std::stringstream strbuf;

    // Check whether data is available
    bool haveR = false;
    bool haveN = false;
    int count = 0;
    while (!haveR || !haveN)
    {
        int ret = recv(m_skt, static_cast<void*> (&buffer), 1, 0);

        if (ret <= 0)
            return "";  // Blank string may mean socket is dead.
        if (buffer == '\r')
            haveR = true;
        if (haveR && buffer == '\n')
            haveN = true; //only have n when we have R
        strbuf << buffer;
        count++;
    }

    return strbuf.str().substr(0, count - 2); // Miss out the last two characters
}

/**
 * Send a CasparCommand to CasparCG
 *
 * @param cmd CasparCommand A command to be executed
 * @param resp CasparResponse A response object to write results into
 */
void CasparConnection::sendCommand (CasparCommand cmd, std::vector<std::string>& response)
{
    const char *buffer = cmd.form().c_str();
    send(m_skt, buffer, strlen(buffer), 0);

    std::string line = receiveLine();
    if (line.empty())
    {
        m_connected = false;
        throw(-1);
    }

    // Extract the response code and identify returned result
    std::string respCode = line.substr(0, 3);
    int responsecode = ConvertType::stringToInt(respCode.c_str());

    response.push_back(queryResponse(responsecode));

    response.push_back(line);

    if (responsecode == E_COMMAND_NOT_UNDERSTOOD ||
            responsecode == S_DATA_RETURNED ||
            responsecode == S_COMMAND_EXECUTED_WITH_MANY_DATA)
    {
        while (!line.empty())
        {
            line = receiveLine();
            response.push_back(line);
        }

        response.pop_back();
    }
    else if (responsecode == S_COMMAND_EXECUTED_WITH_DATA)
    {
        response.push_back(receiveLine());
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
