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
*   File Name   : Test_Log.cpp
*   Version     : 1.0
*****************************************************************************/
//Test_Log - Dummy log object for unit testing

#include "Test_Log.h"

using std::cout;
using std::cerr;
using std::endl;



Log_Test::Log_Test(Hook h) {
    info = warn=error=omgwtf=0;
    h.gs->L->registerLogger(this);
}

Log_Test::~Log_Test() {

}

void Log_Test::info(std::string where,std::string message) {
    info = 1;
}

void Log_Test::warn(std::string where,std::string message) {
    warn = 1;
}

void Log_Test::error(std::string where,std::string message) {
    error = 1;
}

void Log_Test::OMGWTF(std::string where,std::string message) {
    omgwtf = 1;
}
