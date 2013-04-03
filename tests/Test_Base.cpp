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
*   File Name   : Test_Base.cpp
*   Version     : 1.0
*****************************************************************************/
//Test_Base.cpp - base unit test main function

#include <iostream>

//prototypes
int runtest();
extern char testname[];


int main(int argc,char *argv[]) {
    std::cout << "Running test: " << testname << " ...";
    int result = runtest();
    switch(result) {
        case 0:
            std::cout << "\t\t\t\E[32m[PASSED]\E[00m" << std::endl;
            break;
        default:
            std::cout << "\t\t\t\E[31m[FAILED]\E[00m" << std::endl;
            break;
    }
    return result;
}
