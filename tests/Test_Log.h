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
*   File Name   : Test_Log.h
*   Version     : 1.0
*****************************************************************************/
//Test_Log - Dummy log object for unit testing
#pragma once
#include "Log.h"
#include <TarantulaPlugin.h>
#include <iostream>

class Log_Test : Log {
public:
    Log_Test(Hook h);
    ~Log_Test();
    virtual void info(std::string where,std::string message);
    virtual void warn(std::string where,std::string message);
    virtual void error(std::string where,std::string message);
    virtual void OMGWTF(std::string where,std::string message);
    int info,warn,error,omgwtf;
};
