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
*   File Name   : Log_Screen.h
*   Version     : 1.0
*   Description : Defines a basic logger to send all log data to stdout.
*
*****************************************************************************/


#pragma once
#include "Log.h"
#include "TarantulaPlugin.h"
#include <iostream>

/**
 * Simple, statically linked logger to format output and dump to stdout.
 */
class Log_Screen: Log
{
public:
    Log_Screen (Hook h);
    virtual ~Log_Screen ();
    virtual void info (std::string where, std::string message);
    virtual void warn (std::string where, std::string message);
    virtual void error (std::string where, std::string message);
    virtual void OMGWTF (std::string where, std::string message);
};
