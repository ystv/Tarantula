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
*   File Name   : Log.cpp
*   Version     : 1.0
*   Description : Logging plugin base class. Makes calls on all registered
*                 loggers when new log entries are received.
*
*****************************************************************************/


#include "Log.h"

Log::Log ()
{

}

Log::~Log ()
{

}

void Log::info (std::string where, std::string message)
{
    for (Log* it : m_ploggers)
    {
        it->info(where, message);
    }
}

void Log::warn (std::string where, std::string message)
{
    for (Log* it : m_ploggers)
    {
        it->warn(where, message);
    }
}

void Log::error (std::string where, std::string message)
{
    for (Log* it : m_ploggers)
    {
        it->error(where, message);
    }
}

void Log::OMGWTF (std::string where, std::string message)
{
    for (Log* it : m_ploggers)
    {
        it->OMGWTF(where, message);
    }
}

void Log::registerLogger (Log* instance)
{
    m_ploggers.push_back(instance);
}

void Log::unregisterLogger (Log* instance)
{
    for (std::vector<Log*>::iterator it = m_ploggers.begin();
            it != m_ploggers.end(); ++it)
    {
        if (*it == instance)
        {
            m_ploggers.erase(it);
            return;
        }
    }
}
