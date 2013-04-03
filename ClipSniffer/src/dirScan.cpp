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
*   File Name   : dirScan.cpp
*   Version     : 1.0
*****************************************************************************/
//dirScan.cpp - defines a class to scan a DIR for files.

#include "dirScan.h"

#ifdef WIN32
#include <windows.h>
#include <glib.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <iostream>

dirScan::dirScan(dirScan_StrType p) {
    path = p;
}

dirScan::~dirScan() {

}

void dirScan::procFileDBObject(FileDBObject *fdbo) {
    if(files.FileExists(fdbo->filename)) {
        //file exists. Mark as not missing
        files.setFilePresent(fdbo->filename);
        //now check if it's changed
        FileDBObject fdbo2 = files.getFile(fdbo->filename);
        if(fdbo2.filesize != fdbo->filesize) {
            files.FileChanged(fdbo->filename,true);
        }
        if(fdbo2.lastupdate != fdbo->lastupdate) {
            files.FileChanged(fdbo->filename,true);
        }
    }
    else {
        //file is new - process accordingly
        files.newFile(*fdbo);
    }
}

std::vector<dirScan_StrType> dirScan::scan() {
    //common code
    files.setAllGone(); //set all files missing so we can tick them off as still there
#ifdef WIN32
    wchar_t *pathchr = reinterpret_cast<wchar_t*>(g_utf8_to_utf16(path.c_str(),-1,NULL,NULL,NULL));
    std::wstring path2(pathchr);
    if(path2[path.length()-1] != '\\')
        path2 = path2 + L"\\";
    path2 = path2 + L"*";
    WIN32_FIND_DATA FileData;
    HANDLE hFile;
    hFile = FindFirstFile(path2.c_str(), &FileData);
    do {
        if(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue; //we aren't interested in directories here.
        FileDBObject fdbo;
        fdbo.filename = g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(FileData.cFileName),-1,NULL,NULL,NULL);
        LARGE_INTEGER fs; //to hold filesize
        fs.LowPart = FileData.nFileSizeLow;
        fs.HighPart = FileData.nFileSizeHigh;
        fdbo.filesize = fs.QuadPart;
        LARGE_INTEGER modtme;
        modtme.HighPart = FileData.ftLastWriteTime.dwHighDateTime;
        modtme.LowPart = FileData.ftLastWriteTime.dwLowDateTime;
        fdbo.lastupdate = modtme.QuadPart;
        procFileDBObject(&fdbo);
    } while (FindNextFile(hFile,&FileData) != 0);
    FindClose(hFile);
#else //linux from here on
    DIR *d;
    struct dirent *dir;
    d  = opendir(path.c_str());
    while ((dir = readdir(d)) != NULL) {
        FileDBObject fdbo;
        fdbo.filename = dir->d_name;
        struct stat st;
        if(path[path.length()-1] != '/')
            path += TXT("/"); //TXT macro gets compiled out here but for completeness it's included.
        stat((path + "/" + fdbo.filename).c_str(),&st);
        if(S_ISREG (st.st_mode)) {
            fdbo.filesize = st.st_size;
            fdbo.lastupdate = st.st_mtime;
            procFileDBObject(&fdbo);
        }
    }
#endif
    //build missing file list
    files.sortMissing();
    std::vector<dirScan_StrType> tmp;
    return tmp;
}