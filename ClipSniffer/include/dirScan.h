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
*   File Name   : dirScan.h
*   Version     : 1.0
*****************************************************************************/
//dirScan.h - defines a class to scan a DIR for files.

//a bit of windows debug code to track memory
#ifdef WIN32
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#pragma once
#include <string>
#include <vector>
#include <map>
#include "FileList.h"

#define dirScan_StrType std::string

class dirScan {
public:
    dirScan(dirScan_StrType p);
    ~dirScan();
    std::vector<dirScan_StrType> scan();
    FileList files;
private:
    dirScan_StrType path;
    void procFileDBObject(FileDBObject *fdbo);
};
