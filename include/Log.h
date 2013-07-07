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
*   File Name   : Log.h
*   Version     : 1.0
*   Description : Log.h Defines the base class for all logging plugins.
*
*****************************************************************************/


#pragma once
#include <string>
#include <vector>


/**
 * Base class for logging plugins.
 */
class Log
{
public:
    Log ();
    virtual ~Log ();
    virtual void info (std::string where, std::string message);
    virtual void warn (std::string where, std::string message);
    virtual void error (std::string where, std::string message);
    virtual void OMGWTF (std::string where, std::string message);
    void registerLogger (Log* instance);
    void unregisterLogger (Log* instance);
private:
    std::vector<Log*> m_ploggers;
};
