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
*   File Name   : CrosspointDevice_Hedco.h
*   Version     : 1.0
*   Description : CrosspointDevice plugin for the Hedco crosspoint used by YSTV
*                 (known as the NetMux)
*
*****************************************************************************/


#pragma once

#include "boost/asio.hpp"
#include "CrosspointDevice.h"

class CrosspointDevice_Hedco: public CrosspointDevice
{
public:
    CrosspointDevice_Hedco (PluginConfig config, Hook h);
    virtual ~CrosspointDevice_Hedco ();

    void switchOP (std::string output, std::string input);
    void updateHardwareStatus ();

private:
    std::string m_portname;
    int m_baudrate;

    std::fstream m_connection;

    boost::asio::io_service m_io;
    boost::asio::serial_port m_serial;
};

