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
*   File Name   : ConfigLoader.h
*   Version     : 1.0
*   Description : Loads base config of ClipSniffer and retrieves input and
*                 output file paths etc
*
*****************************************************************************/


#pragma once
#include <string>

class ConfigLoader {
public:
    ConfigLoader();
    ConfigLoader(std::string filename);
    int getDelay();
    void LoadConfig(std::string filename);
    std::string getMediaPath();
    std::string getOutputPath();
    std::string getTableName();
private:
    int delay;
    std::string MediaPath;
    std::string OutputPath;
    std::string TableName;

};
