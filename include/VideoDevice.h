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
*   File Name   : VideoDevice.h
*   Version     : 1.0
*   Description : Base device for playing a video
*
*****************************************************************************/


#pragma once

#include <list>

#include "Device.h"
#include "TarantulaPlugin.h"
#include "PluginConfig.h"

/**
 * Class to hold info about a video file
 */
class VideoFile {
public:
    std::string m_filename;

    //! Path as used on video server
    std::string m_path;

    //! Frames
    unsigned long m_duration;

    //! Bytes
    unsigned long m_filesize;
    std::string m_title;

    //! Start Of Media (frames)
    unsigned long m_som;

private:

};

//! Named this way to avoid conflict with VideoDeviceStatus class
enum VideoDeviceActivity {
    //!Not playing anything
    VDS_STOPPED,
    //!Playing something
    VDS_PLAYING,
    //!Playing a file not in the list!
    VDS_MISSING,
    //!Gone AWoL. Something is wrong!
    VDS_FAIL
};

/**
 * Details about what a Video Device is up to
 */
class VideoDeviceStatus {
public:
    VideoDeviceStatus ();
    VideoDeviceActivity state;
    std::string filename;
    unsigned long remainingframes;
};

/**
 * Definitions of available Video Actions
 */
const ActionInformation VIDEOACTION_PLAY = {0, "Play", "Load and play a video file immediately",
        {
                {"filename", "string"}
        }
};
const ActionInformation VIDEOACTION_LOAD = {1, "Load", "Load a video file to be played",
        {
                {"filename", "string"},
        }
};
const ActionInformation VIDEOACTION_PLAYLOADED = {2, "Play_Loaded", "Play a video file previously loaded with Load",
        {
        }
};
const ActionInformation VIDEOACTION_STOP = {3, "Stop", "Stop playing",
        {
        }
};

const std::vector<const ActionInformation*> video_device_action_list =
    {&VIDEOACTION_PLAY, &VIDEOACTION_LOAD, &VIDEOACTION_PLAYLOADED, &VIDEOACTION_STOP};

/**
 * Video Device control base class
 */
class VideoDevice: public Device {
public:
    VideoDevice (PluginConfig config, Hook h);

    virtual void getFiles ()=0; //Read list of video files
    virtual void cue (std::string clip)=0;
    virtual void play ()=0;
    virtual void stop ()=0;
    virtual void immediatePlay (std::string filename); // Can be overridden, defaults to a cue then a play.

    static void runDeviceEvent (Device *pdevice, PlaylistEntry *pevent);

    //! Override so EndPlaying is called correctly
    void poll ();
    VideoDeviceStatus getStatus ();
    std::map<std::string, std::string> m_extradata;

    void getFileList(std::vector<std::pair<std::string, int>>& filelist);
protected:
    std::map<std::string, VideoFile> m_files;
    unsigned int m_channel; // Channel number. Value dependent on exact server.
    std::string m_xpname;
    unsigned int m_xpport;
    VideoDeviceStatus m_videostatus;
};
