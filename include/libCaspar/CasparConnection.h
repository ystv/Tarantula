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
#include <boost/asio.hpp>
#include <string>
#include <queue>

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
    CasparConnection (std::string host, std::string port, long int connecttimeout);
    ~CasparConnection ();

    std::string receiveLine ();
    bool tick ();
    void run ();
    void sendCommand (CasparCommand cmd);

    bool m_errorflag;      //! Connection error
    bool m_badcommandflag; //! Connection still good but CasparCG returned an error
    bool m_connectstate;   //! Connection completion flag

private:
    std::string queryResponse (int responsecode);
    std::vector <std::string> m_datalines;
    std::queue <CasparCommand> m_commandqueue;

    long int m_connecttimeout;

    boost::asio::io_service m_io_service;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::streambuf m_recvdata;

    void cb_connect (const boost::system::error_code& err);
    void cb_write (const boost::system::error_code& err);
    void cb_firstread (const boost::system::error_code& err);
    void cb_doneread (const boost::system::error_code& err);
    void processData (std::istream& is);
    void sendQueuedCommand ();

};
