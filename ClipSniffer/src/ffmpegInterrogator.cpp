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
*   File Name   : ffmpegInterrogator.cpp
*   Version     : 1.0
*****************************************************************************/
//ffmpegInterrogator.cpp - interrogates a file using ffmpeg

#include "ffmpegInterrogator.h"

//begin hack due to broken ffmpeg
#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 
//end hack

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

//begin hack to make AVDictionary usable
struct AVDictionary {
    int count;
    AVDictionaryEntry *elems;
};
//end hack

#include <iostream>

MediaFileInfo interrogate(std::string filename) {
    MediaFileInfo mfi;
    mfi.length = 0;
    mfi.title = filename;
    AVFormatContext *pFormatCtx;
    // Open video file
    const char *fn = filename.c_str();
    if(avformat_open_input(&pFormatCtx,fn, NULL, NULL)!=0) {
        std::cout << "Error opening file." << fn << std::endl;
        return mfi;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        std::cout << "Error reading streams from file." << std::endl;
        return mfi;
    }
    mfi.filename = filename;
    mfi.length = (pFormatCtx->duration*25)/AV_TIME_BASE;
    for(int i=0;i< (pFormatCtx->metadata->count);++i) {
        if((std::string)pFormatCtx->metadata->elems[i].key == "title") {
    mfi.title = pFormatCtx->metadata->elems[i].value;
        }
    }
    avformat_close_input(&pFormatCtx);
    return mfi;
}
