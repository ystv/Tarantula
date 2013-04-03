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
*   File Name   : CasparConnection.h
*   Version     : 1.0
*   Description : Handles connecting to and passing data to and from CasparCG
*
*****************************************************************************/


#pragma once

#include <libCaspar/libCaspar.h>
#include <libCaspar/CasparCommand.h>
#include <string>

#define E_NO_RESPONSE -1
#define E_COMMAND_NOT_UNDERSTOOD 400
#define E_ILLEGAL_CHANNEL 401
#define E_PARAMETER_MISSING 402
#define E_ILLEGAL_PARAMETER 403
#define E_FILE_NOT_FOUND 404
#define E_SERVER_FAILED 500
#define E_COMMAND_FAILED 501
#define E_FILE_UNREADABLE 502

#define S_INFORMATION_RETURNED 100
#define S_DATA_RETURNED 101
#define S_COMMAND_EXECUTED 202
#define S_COMMAND_EXECUTED_WITH_DATA 201
#define S_COMMAND_EXECUTED_WITH_MANY_DATA 200

/**
 * Handles connecting to and passing data to and from CasparCG
 */
class CasparConnection
{
public:
    CasparConnection ();
    CasparConnection (std::string host);
    ~CasparConnection ();
    void sendCommand (CasparCommand cmd, std::vector<std::string> &response);
    bool m_connected;
private:
    int m_skt;
    std::string receiveLine ();
    std::string queryResponse (int responsecode);
};
