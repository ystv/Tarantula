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


#include <string>
#include <sstream>
#include <istream>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <ctime>

#include "DateConversions.h"

/**
 * Convert a date/time as string to local Unix timestamp. Optional input format
 * specification, default is YYYY-MM-DD HH:MM:SS.FFFF
 *
 * @param datetime  String containing date/time to convert
 * @param formatter Input format. If time is not set, do not include. Default: %Y-%m-%d %H:%M:%S%F
 * @return time_t   Resulting timestamp
 */
time_t DateConversions::datetimeToTimeT (std::string datetime, std::string formatter /*= "%Y-%m-%d %H:%M:%S%F"*/)
{
    // Configure custom formatter
    boost::posix_time::time_facet *facet = new  boost::posix_time::time_facet(formatter.c_str());
    std::istringstream is(datetime);
    is.imbue(std::locale(is.getloc(), facet));

    boost::posix_time::ptime newtime;

    is >> newtime;
    tm timestruct = boost::posix_time::to_tm(newtime);
    time_t outputtime = mktime(&timestruct);
    return outputtime;
}

/**
 * Convert a timestamp to a date/time string.
 *
 * @param timestamp Input timestamp to convert
 * @param formatter Output format. Default: YYYY-mmm-DD HH:MM:SS.fffffffff
 * @return          String of date/time
 */
std::string DateConversions::timeTToString (time_t timestamp, std::string formatter /*= "%Y-%m-%d %H:%M:%S%F"*/)
{
    // Configure custom formatter
    boost::posix_time::time_facet *facet = new  boost::posix_time::time_facet(formatter.c_str());
    std::stringstream ss;
    ss.imbue(std::locale(ss.getloc(), facet));

    // Generate and output the time
    boost::posix_time::ptime timedata = boost::posix_time::from_time_t(timestamp);
    ss << timedata;

    return ss.str();
}

/**
 * Convert Boost gregorian date to string.
 *
 * @param date      Date to be converted
 * @param formatter Format string. Defaults to YYYY-MM-DD
 * @return          Resulting string
 */
std::string DateConversions::gregorianDateToString (boost::gregorian::date date, std::string formatter /*= "%Y-%m-%d"*/)
{
    // Configure custom formatter
    boost::gregorian::date_facet *facet = new boost::gregorian::date_facet(formatter.c_str());
    std::stringstream ss;
    ss.imbue(std::locale(ss.getloc(), facet));

    // Output the date
    ss << date;

    return ss.str();

}
