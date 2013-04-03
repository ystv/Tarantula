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
*   File Name   : FileList.h
*   Version     : 1.0
*   Description : File listing database
*
*****************************************************************************/


#pragma once

#include "MemDB.h" //parent class

/**
 * All the data of a single row of the FileList
 */
class FileDBObject {
public:
    std::string filename;
    bool gone;
    long long filesize;
    long long duration;
    long long lastupdate;
    bool changed;
    FileDBObject();
};

/**
 * An extension of MemDB to generate a list of video files with some more
 * details than Caspar provides natively.
 */
class FileList : public MemDB {
public:
    FileList();
    void newFile(FileDBObject &obj);
    bool FileExists(std::string filename); //returns true if the file is known about
    void setFilePresent(std::string filename); //set a file as not missing
    void setAllGone(); //sets all as missing
    void sortMissing(); //sorts the missing list to the missing table
    void FileChanged(std::string filename,bool newval); //set file changed flag
    FileDBObject getFile(std::string filename);
    std::vector<std::string> getMissingList();
    std::vector<std::string> getChangedList();
    void updateDuration(std::string filename,long long newval);
private:
    DBQuery* newItem;
    DBQuery* removeItem;
    DBQuery* clearMissing;
    DBQuery* setAllMissing;
    DBQuery* setPresent;
    DBQuery* setChanged;
    DBQuery* getChanged;
    DBQuery* setDuration;
    DBQuery* getItem;
    DBQuery* getMissing;
    DBQuery* popMissing;
    DBQuery* startTrans;
    DBQuery* endTrans;
};
