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
*   File Name   : ConfigLoader.cpp
*   Version     : 1.0
*   Description : Loads base config of ClipSniffer and retrieves input and
*                 output file paths etc
*
*****************************************************************************/


#include <iostream>
#include "../include/ConfigLoader.h"
#include "../../include/TarantulaCore.h"
#include "pugixml.hpp"

ConfigLoader::ConfigLoader() {

}
/**
 * Constructor
 * We can't call the parent constructor with variable because the virtuals
 * won't be resolved properly, so we do it this way
 */

ConfigLoader::ConfigLoader(std::string filename) {
    LoadConfig(filename);
}

/**
 * Load configuration from file
 *
 * @param filename Name of XML config file to open
 */
void ConfigLoader::LoadConfig(std::string filename)
{
    pugi::xml_document configdata;

    // Attempt to load the file
    pugi::xml_parse_result result = configdata.load_file(filename.c_str());

    // Return an error if parsing fails
    if (pugi::status_file_not_found == result.status) {
        std::cout << "Couldn't find the config file." << std::endl;
        exit(1);
    } else if (pugi::status_ok != result.status) {
        std::cout << "Config file parsing failed." << std::endl;
        exit(1);
    }

    // Grab the Config node and get some paths
    pugi::xml_node confignode = configdata.document_element();
    if (confignode.empty()) {
        std::cout << "Couldn't get a root node in the config file. This is bad." << std::endl;
        exit(1);
    }

    MediaPath = confignode.child_value("MediaPath");
    if (!MediaPath.compare(""))
    {
        std::cout << "Couldn't find a MediaPath in config file! Using current directory" << std::endl;
        MediaPath = "./";
    }

    OutputPath = confignode.child_value("DatabasePath");
    if (!OutputPath.compare(""))
    {
        std::cout << "Couldn't find a DatabasePath in config file! Using \"db.sqlite\"" << std::endl;
    }

    delay = confignode.child("PauseBeforeRepeat").text().as_int(30);

    TableName = confignode.child_value("FileTable");
    if (!TableName.compare(""))
    {
        std::cout << "Couldn't find a FileTable in config file! Using \"Files\"" << std::endl;
        TableName = "Files";
    }
}

/**
 * getDelay
 * Returns config delay between file checks
 *
 * @return  int Delay time
 */
int ConfigLoader::getDelay() {
    return delay;
}

/**
 * getMediaPath
 * Returns path to media files
 *
 * @return  ustring Path to media files
 */
std::string ConfigLoader::getMediaPath() {
    return MediaPath;
}

/**
 * getOutputPath
 * Returns path to target database file
 *
 * @return  ustring Path to database file
 */
std::string ConfigLoader::getOutputPath() {
    return OutputPath;
}

/**
 * getTableName
 * Returns the name the table should take
 *
 * @return  ustring Table name
 */
std::string ConfigLoader::getTableName() {
    return TableName;
}
