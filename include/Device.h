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
*   File Name   : Device.h
*   Version     : 1.0
*   Description : Base class for a device
*
*****************************************************************************/

#pragma once
#include <string>
#include "TarantulaPlugin.h"
#include "PluginConfig.h"
#include "PlaylistDB.h"

class PlaylistEntry;

/**
 * Base class for a device
 */
class Device: public Plugin
{
public:
    Device (PluginConfig config, playlist_device_type_t, Hook h);
    virtual ~Device ();
    void addPluginReference (std::shared_ptr<Plugin> thisplugin);

    playlist_device_type_t getType ();

    //! Update status from device. Called once a tick.
    virtual void poll ();
    //! Actually read status from hardware, called less often.
    virtual void updateHardwareStatus ()=0;
    //! How often to run updateHardwareStatus()
    int pollperiod;
    //! Pointer to vector of available actions
    const std::vector<const ActionInformation*> *m_actionlist;

    static void runDeviceEvent (std::shared_ptr<Device> pdevice, PlaylistEntry *pevent);
protected:
    playlist_device_type_t m_type;
    //! Last event played on the device
    int m_event;
    //! Timestamp of next updateHardwareStatus()
    long int m_polltime;

private:

};

void deviceTicks (); //calls all the device poll functions
