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
*   File Name   : Channel.h
*   Version     : 1.0
*   Description : Class for running channels and all event handling
*
*****************************************************************************/


#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <mutex>

#include "TarantulaCore.h"
#include "PlaylistDB.h"

//define callbacks
typedef std::function<void(std::string, int)> cbBegunPlaying;
typedef std::function<void(std::string, int)> cbEndPlaying;
typedef std::function<void(void)> cbTick;
//end callbacks

/**
 * Channel class.
 * All the magic of channels to broadcast
 */
class Channel
{
public:
    Channel ();
    Channel (std::string name, std::string xpname, std::string xport);
    void init ();
    ~Channel ();
    void tick ();
    void begunPlaying (std::string name, int id);
    void endPlaying (std::string name, int id);
    int createEvent (PlaylistEntry *ev);

    void manualTrigger (int id);

    PlaylistDB m_pl;
    std::string m_channame;
    //! Crosspoint name for this channel
    std::string m_xpdevicename;
    //! Crosspoint port name for this channel
    std::string m_xpport;

    static int getChannelByName (std::string channelname);

    static void manualHoldRelease (PlaylistEntry &event, Channel *pchannel);

private:
    void runEvent (PlaylistEntry& pevent);

    void periodicDatabaseSync (std::shared_ptr<void> data, std::timed_mutex &core_lock);

    int m_sync_counter;

    int m_hold_event;
};

/*
 *  Because you cannot add member function pointers to the callbacks, here is a workaround:
 *  We go through and call the callback functions of each Channel class in the host's stack.
 */
void channelTick ();
void channelBegunPlaying (std::string name, int id);
void channelEndPlaying (std::string name, int id);

extern std::vector<std::shared_ptr<Channel>> g_channels; //declared here because adding it to TarantulaCore creates reference loops.

