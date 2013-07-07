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
*   File Name   : CGDevice.h
*   Version     : 1.0
*   Description : Base class definition for graphics devices.
*
*****************************************************************************/


#pragma once
#include "Device.h"
#include "TarantulaPlugin.h"

/**
 * Stores data about a layer within the graphics device
 */
struct CGLayer
{
    int layer;
    std::string graphicname;
    int playstep;
    std::map<std::string, std::string> graphicdata;
};

/**
 * Definitions of available CG Actions
 */
const ActionInformation CGACTION_ADD = { 0, "Add", "Adds a new CG event", { {
        "graphicname", "string" }, { "hostlayer", "int" }, { "templatedata...",
        "string" } } };
const ActionInformation CGACTION_PLAY = { 1, "Play",
        "Plays template on by one step", { { "hostlayer", "int" } } };
const ActionInformation CGACTION_UPDATE = { 2, "Update",
        "Replace existing template data with new data", {
                { "hostlayer", "int" }, { "templatedata...", "string" } } };
const ActionInformation CGACTION_REMOVE = { 3, "Remove",
        "Stop template and clear layer", { { "hostlayer", "int" } } };

const std::vector<const ActionInformation*> CG_device_action_list = {
        &CGACTION_ADD, &CGACTION_PLAY, &CGACTION_UPDATE, &CGACTION_REMOVE };

/**
 * Base class for a CG device
 */
class CGDevice: public Device
{
public:
    CGDevice (PluginConfig config, Hook h);
    virtual ~CGDevice () = 0;

    virtual void add (std::string graphicname, int layer,
            std::map<std::string, std::string> *pdata)=0;
    virtual void remove (int layer)=0;
    virtual void play (int layer)=0;
    virtual void update (int layer, std::map<std::string, std::string> *pdata)=0;

    void getHardwareStatus ();
    static void runDeviceEvent (std::shared_ptr<Device> pdevice, PlaylistEntry *pevent);
    void getTemplateList (std::vector<std::pair<std::string, int>>& templates);

protected:
    std::map<int, CGLayer> m_plugindata;

    std::vector<std::string> m_templatelist;

};

// Convienience free function for parsing data
void parseExtraData (PlaylistEntry *pevent, std::string *pgraphicname,
        int *playernumber, std::map<std::string, std::string> *pdata);
