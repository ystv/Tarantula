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
*   File Name   : Demo_VideoDevice.h
*   Version     : 1.0
*   Description : Demonstration of a VideoDevice
*
*****************************************************************************/


#pragma once

#include <cstdlib>

#include "VideoDevice.h"
#include "PluginConfig.h"

// Some magic numbers since this is just a demo
#define VDDEMO_CLIPNAME "test1"
#define VDDEMO_CLIPFRAMES 223

/**
 * A demo video device
 */
class VideoDevice_Demo: public VideoDevice
{
public:
    VideoDevice_Demo (PluginConfig config, Hook h);
    ~VideoDevice_Demo ();
    void updateHardwareStatus ();
    void getFiles ();
    void cue (std::string clip);
    void play ();
    void stop ();
protected:
    bool m_clipready;

};

