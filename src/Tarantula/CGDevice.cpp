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
*   File Name   : CGDevice.cpp
*   Version     : 1.0
*   Description : Base class for a graphics device.
*
*****************************************************************************/


#include "CGDevice.h"
#include "Device.h"
#include "TarantulaPlugin.h"
#include "Misc.h"
#include "PlaylistDB.h"

CGDevice::CGDevice (PluginConfig config, Hook h) :
        Device(config, EVENTDEVICE_CGDEVICE, h)
{
    (*h.gs->Devices)[config.m_instance] = this;
    m_actionlist = &CG_device_action_list;

}

/**
 * Free function to convert extradata map in a PlaylistEntry to the formats
 * for a CG device command.
 *
 * Looks for "graphicname" and "layer" keys, all other data is transposed to
 * data map for graphics plugin.
 *
 * @param event
 * @param graphicname
 * @param layernumber
 * @param data        The data to pass to the template.
 */
void parseExtraData (PlaylistEntry *pevent, std::string *pgraphicname,
        int *playernumber, std::map<std::string, std::string> *pdata)
{
    *pgraphicname = "";

    for (std::pair<std::string, std::string> thiselement : pevent->m_extras)
    {
        if (!thiselement.first.compare("graphicname"))
        {
            *pgraphicname = thiselement.second;
        }
        else if (!thiselement.first.compare("layer"))
        {
            try
            {
                *playernumber = ConvertType::stringToInt(thiselement.second);
            } catch (...)
            {
                g_logger.warn("CG Device RunDeviceEvent",
                        "No layer in event " + pevent->m_eventid);
                throw std::exception();
            }
        }
        else
        {
            pdata->operator [](thiselement.first) = thiselement.second;
        }
    }
}

/**
 * Static function to process an incoming event and despatch commands to a
 * CG device.
 *
 * @param device
 * @param event
 */
void CGDevice::runDeviceEvent (Device* device, PlaylistEntry* event)
{
    CGDevice *eventdevice = static_cast<CGDevice*>(device);

    if (CGACTION_ADD == event->m_action)
    {
        int layer;
        std::string graphicname;
        std::map<std::string, std::string> templatedata;

        try
        {
            parseExtraData(event, &graphicname, &layer, &templatedata);
            eventdevice->add(graphicname, layer, &templatedata);
        } catch (...)
        {
            g_logger.error(event->m_device, "Unable to add event " + event->m_eventid);
        }
    }
    else if (CGACTION_PLAY == event->m_action)
    {
        int layer;

        try
        {
            parseExtraData(event, NULL, &layer, NULL);
            eventdevice->play(layer);
        } catch (...)
        {
            g_logger.error(event->m_device, "Unable to play event " + event->m_eventid);
        }
    }
    else if (CGACTION_UPDATE == event->m_action)
    {
        int layer;
        std::map<std::string, std::string> templatedata;

        try
        {
            parseExtraData(event, NULL, &layer, &templatedata);
            eventdevice->update(layer, &templatedata);
        } catch (...)
        {
            g_logger.error(event->m_device,
                    "Unable to update in event " + event->m_eventid);
        }
    }
    else if (CGACTION_REMOVE == event->m_action)
    {
        int layer;

        try
        {
            parseExtraData(event, NULL, &layer, NULL);
            eventdevice->remove(layer);
        } catch (...)
        {
            g_logger.error(event->m_device,
                    "Unable to remove in event " + event->m_eventid);
        }
    }
    else
    {
        g_logger.error("CG Device RunDeviceEvent",
                "Event " + ConvertType::intToString(event->m_eventid)
                        + " has a non-existent action");

    }
}

/**
 * Getter for template list held by a CG plugin.
 *
 * @param templates Vector of strings for template names
 */
void CGDevice::getTemplateList (std::vector<std::string>& templates)
{
    templates = m_templatelist;
}

