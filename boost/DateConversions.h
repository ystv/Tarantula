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
*   File Name   : Misc.cpp
*   Version     : 1.0
*   Description : Miscellaneous conversion functions etc
*
*****************************************************************************/

#pragma once

#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <ctime>

class DateConversions
{
public:
    static time_t datetimeToTimeT (std::string datetime, std::string formatter = "%Y-%m-%d %H:%M:%S%F");
    static std::string timeTToString (time_t timestamp, std::string formatter = "%Y-%m-%d %H:%M:%S%F");
    static std::string gregorianDateToString (boost::gregorian::date date, std::string formatter = "%Y-%m-%d");
};
