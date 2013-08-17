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
*   File Name   : Log_Screen.cpp
*   Version     : 1.0
*   Description : Implements a basic logger to send all log data to stdout.
*                 Statically linked and loaded as part of the core for useful
*                 output if all else fails.
*
*****************************************************************************/


#include "Log_Screen.h"

Log_Screen::Log_Screen (Hook h)
{
    h.gs->L->registerLogger(this);
}

Log_Screen::~Log_Screen ()
{
    // Nothing to do...
}

std::string Log_Screen::gettime ()
{
	long timenow = time(NULL);
	struct tm * timeinfo = localtime(&timenow);
	char buffer[25];
	strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", timeinfo);

	return std::string(buffer);
}

void Log_Screen::info (std::string where, std::string message)
{
    std::cout << gettime() << " INFO: " << where << ":: " << message << std::endl;
}

void Log_Screen::warn (std::string where, std::string message)
{
    std::cout << gettime() << " WARN: " << where << ":: " << message << std::endl;
}

void Log_Screen::error (std::string where, std::string message)
{
    std::cerr << gettime() << " ERROR: " << where << ":: " << message << std::endl;
}

void Log_Screen::OMGWTF (std::string where, std::string message)
{
    std::cerr << gettime() << " OMGWTF: " << where << ":: " << message << std::endl;
}

