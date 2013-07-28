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
*   File Name   : Device.cpp
*   Version     : 1.0
*   Description : Base class for a device
*
*****************************************************************************/

#include <algorithm>

#include "Device.h"
#include "PluginConfig.h"
#include "TarantulaPlugin.h"
#include "pugixml.hpp"
#include "Log.h"

/**
 * Default constructor
 * @param config Configuration data for this plugin
 * @param type   The device type of this particular device
 * @param h      Pointer back to GlobalStuff structure
 */
Device::Device (PluginConfig config, playlist_device_type_t type, Hook h) :
        Plugin(config, h)
{
    this->m_type = type;
    m_event = -1;

    pugi::xml_node pollchild = config.m_plugindata_xml.child("PollPeriod");
    pollperiod = pollchild.text().as_int(0);

    if (0 == pollperiod)
    {
        g_logger.warn(config.m_instance, "Got a bad PollPeriod. Setting to 1 second");
        pollperiod = 1;
    }
    else
    {
        std::string pollunits = pollchild.attribute("units").value();
        if (!pollunits.compare(""))
        {
            g_logger.info(config.m_instance,
                    "PollPeriod had no units. Assuming seconds.");
        }
        else if (!pollunits.compare("frames"))
        {
            ///TODO #16
        }
        else if (!pollunits.compare("minutes"))
        {
            pollperiod *= 60;
        }
    }

    m_polltime = 0;
}

/**
 * Destructor
 */
Device::~Device ()
{

}

/**
 * Add this plugin to the list managing this type
 *
 *  @param thisplugin Pointer to new plugin
 */
void Device::addPluginReference (std::shared_ptr<Plugin> thisplugin)
{
    std::shared_ptr<Device> thisdevice = std::dynamic_pointer_cast<Device>(thisplugin);

    g_devices[thisdevice->getPluginName()] = std::shared_ptr<Device>(thisdevice);
}


/**
 * The type of device we have here
 * @return deviceType This device's type
 */
playlist_device_type_t Device::getType ()
{
    return m_type;
}

/**
 * Check if we are scheduled to pull a new status
 */
void Device::poll ()
{
    if (time(NULL) > m_polltime)
    {
        updateHardwareStatus();
        m_polltime = time(NULL) + pollperiod;
    }
}

/**
 * Call all the tick functions for all registered devices
 */
void deviceTicks ()
{
    for (std::pair<std::string, std::shared_ptr<Device>> thisdevice : g_devices)
    {
        if (thisdevice.second->getStatus() != UNLOAD)
        {
            thisdevice.second->poll();
        }
        else
        {
            g_devices.erase(thisdevice.first);
        }
    }
}
