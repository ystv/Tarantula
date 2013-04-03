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
#include "Misc.h"

/**
 * Convert a float to a string
 *
 * @param fl Float to convert
 * @return   Converted string
 */
std::string ConvertType::floatToString (float fl)
{
    std::stringstream ss;
    ss << fl;
    return ss.str();
}

/**
 * Convert int to a string
 *
 * @param val Integer to convert
 * @return    Converted string
 */
std::string ConvertType::intToString (int val)
{
    std::stringstream ss;
    ss << val;
    return ss.str();
}

/**
 * ConvertType a string to a long integer
 *
 * @param val String to convert
 * @return    Converted integer
 */
int ConvertType::stringToInt (std::string val)
{
    std::stringstream ss;
    ss << val;
    int out;
    ss >> out;
    return out;
}

