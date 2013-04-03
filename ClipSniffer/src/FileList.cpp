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
*   File Name   : FileList.cpp
*   Version     : 1.0
*   Description : File listing database
*
*****************************************************************************/


#include "FileList.h"
/**
 * FileDBObject
 * All the data of a single row of the FileList
 */
FileDBObject::FileDBObject() {
    filename.clear();
    gone = false;
    filesize = -1;
    duration=0;
    lastupdate = 0;
    changed = true;
}

/**
 * FileList
 * An extension of MemDB to generate a list of video files with some more
 * details than Caspar provides natively.
 */
FileList::FileList() : MemDB() {
    //setup DB here.
    oneTimeExec("CREATE TABLE files (filename TEXT PRIMARY KEY UNIQUE ON CONFLICT REPLACE,filesize INT64,duration INT64,missing INT,changed INT,lastupdate INT64)");
    oneTimeExec("CREATE TABLE MissingFiles (filename TEXT PRIMARY KEY UNIQUE ON CONFLICT REPLACE)");
    //prepare queries
    newItem = prepare("INSERT INTO files (filename,filesize,duration,missing,changed,lastupdate) VALUES(?,?,?,?,?,?)");
    removeItem = prepare("DELETE FROM files WHERE filename = ?");
    clearMissing = prepare("DELETE FROM files WHERE missing = 1");
    setAllMissing = prepare("UPDATE files SET missing = 1");
    setPresent = prepare("UPDATE files SET missing = 0 WHERE filename = ?");
    setChanged = prepare("UPDATE files SET changed = ? WHERE filename = ?");
    getItem = prepare("SELECT filename,filesize,duration,missing,changed,lastupdate FROM files WHERE filename = ?");
    getChanged = prepare("SELECT filename FROM files WHERE changed = 1");
    getMissing = prepare("SELECT * FROM MissingFiles");
    popMissing = prepare("INSERT INTO MissingFiles (filename) SELECT filename FROM files WHERE missing = 1");
    startTrans = prepare("BEGIN TRANSACTION");
    endTrans = prepare("COMMIT TRANSACTION");
    setDuration = prepare("UPDATE files SET duration = ? WHERE filename = ?");
}

void FileList::newFile(FileDBObject &obj) {
    newItem->rmParams();
    newItem->addParam(1,DBParam(obj.filename));
    newItem->addParam(2,DBParam(obj.filesize));
    newItem->addParam(3,DBParam(obj.duration));
    newItem->addParam(4,DBParam(obj.gone));
    newItem->addParam(5,DBParam(obj.changed));
    newItem->addParam(6,DBParam(obj.lastupdate));
    newItem->bindParams();
    sqlite3_step(newItem->getStmt());
    dump("MidLoad.sqlite");
}
bool FileList::FileExists(std::string filename) {
    getItem->rmParams();
    getItem->addParam(1,DBParam(filename));
    getItem->bindParams();
    int rc = sqlite3_step(getItem->getStmt());
    return rc == SQLITE_ROW;
}

void FileList::setFilePresent(std::string filename) {
    setPresent->rmParams();
    setPresent->addParam(1,DBParam(filename));
    setPresent->bindParams();
    sqlite3_step(setPresent->getStmt());
}

void FileList::FileChanged(std::string filename,bool newval) {
    setChanged->rmParams();
    setChanged->addParam(1,DBParam(newval));
    setChanged->addParam(2,DBParam(filename));
    setChanged->bindParams();
    sqlite3_step(setChanged->getStmt());
}

void FileList::setAllGone() {
    sqlite3_step(setAllMissing->getStmt());
}

void FileList::sortMissing() {
    sqlite3_step(startTrans->getStmt());
    sqlite3_step(popMissing->getStmt());
    sqlite3_step(clearMissing->getStmt());
    sqlite3_step(endTrans->getStmt());
}

FileDBObject FileList::getFile(std::string filename) {
    FileDBObject fdbo;
    getItem->rmParams();
    getItem->addParam(1,DBParam(filename));
    getItem->bindParams();
    sqlite3_stmt *stmt = getItem->getStmt();
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        fdbo.filename =  std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));
        fdbo.filesize = sqlite3_column_int64(stmt,1);
        fdbo.duration = sqlite3_column_int64(stmt,2);
        fdbo.gone = sqlite3_column_int(stmt,3);
        fdbo.changed = sqlite3_column_int(stmt,4);
        fdbo.lastupdate = sqlite3_column_int64(stmt,5);
    }
    return fdbo;
}

std::vector<std::string> FileList::getChangedList() {
    std::vector<std::string> changed;
    sqlite3_stmt *stmt = getChanged->getStmt();
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        std::string str(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));
        changed.push_back(str);
    }
    return changed;
}

std::vector<std::string> FileList::getMissingList() {
    std::vector<std::string> missing;
    sqlite3_stmt *stmt = getMissing->getStmt();
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        std::string str(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));
        missing.push_back(str);
    }
    return missing;
}

void FileList::updateDuration(std::string filename,long long newval) {
    setDuration->rmParams();
    setDuration->addParam(1,DBParam(newval));
    setDuration->addParam(2,DBParam(filename));
    setDuration->bindParams();
    sqlite3_step(setDuration->getStmt());
}
