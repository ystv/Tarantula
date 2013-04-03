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
*   File Name   : ClipSniffer.cpp
*   Version     : 1.0
*   Description : Pulls metadata from video clips using libavformat from ffmpeg
*
*****************************************************************************/


#include "ClipSniffer.h"
#include <iostream>
using namespace std;

#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 
//end hack

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#ifdef WIN32
#include <windows.h>
#define sleep(s) Sleep(s*1000)
#else
#include <unistd.h>
#endif

#include "Log.h"

Log g_logger;

std::string PathCombine(std::string a,std::string b) {
    std::string combined;
    combined = a;
    #ifdef WIN32
    char sep = '\\';
    #else
    char sep = '/';
    #endif
    if(a[a.length()-1] != sep)
        combined += sep;
    combined += b;
    return combined;
}


//a huge hack in place because MemDB doesn't seem to be easy to remove references to wstring from for some reason
#ifdef WIN32
std::wstring u2w(std::string ustr) {
    wchar_t *wchr = reinterpret_cast<wchar_t*>(g_utf8_to_utf16(ustr.c_str(),-1,NULL,NULL,NULL));
    return std::wstring(wchr);
}

std::string w2u(std::wstring wstr) {
    gchar* uchr = g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(wstr.c_str()),-1,NULL,NULL,NULL);
    return std::string(uchr);
}
#else 
std::string u2w(std::string ustr) {
    return ustr;
}

std::string w2u(std::string wstr) {
    return wstr;
}
#endif


int main(int argc,char** argv) {
    av_register_all();
    // This is believed to be the correct version
#if LIBAVFORMAT_VERSION_MAJOR > 53
#if LIBAVFORMAT_VERSION_MINOR > 6
    avformat_network_init();
#endif
#endif
    //Load the config file and get some details
    ConfigLoader config("config_base/ClipSniffer.xml");
    const char *outputpath = config.getOutputPath().c_str();
    std::string path = config.getMediaPath();

    dirScan ds(path);
    while(1) {
        ds.scan();
        std::vector<std::string> changed = ds.files.getChangedList();
        for(std::vector<std::string>::iterator it = changed.begin();it<changed.end();++it) {
            MediaFileInfo mi = interrogate(PathCombine(path,*it));
            ds.files.updateDuration(*it,mi.length);
            ds.files.FileChanged(*it,0);
            std::cout << "Got changes in " << (*it).c_str() << std::endl;
        }
        ds.files.dump(outputpath);
        sleep(config.getDelay());
    }
    return 0;
}
