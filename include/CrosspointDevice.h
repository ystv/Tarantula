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
*   File Name   : CrosspointDevice.h
*   Version     : 1.0
*   Description : Base device for crosspoints
*
*****************************************************************************/


#pragma once

#include <vector>
#include <map>

#include "Device.h"
#include "PluginConfig.h"
#include "TarantulaPlugin.h"

enum crosspoint_direction_t
{
    CROSSPOINT_INPUT, CROSSPOINT_OUTPUT
};

/**
 * Struct containing data relating to the state of one input/output channel
 */
struct CrosspointChannel
{
    std::string name;
    int audioport;
    int videoport;
    crosspoint_direction_t direction;
};

/**
 *  Definitions of available Crosspoint Actions
 */

const ActionInformation CROSSPOINTACTION_SWITCH = { 0, "Switch",
        "Switch crosspoint output to connect to a different input", { {
                "output", "string" }, { "input", "string" } } };

const std::vector<const ActionInformation*> crosspoint_device_action_list = {
        &CROSSPOINTACTION_SWITCH };

/**
 * Base device for crosspoints
 */
class CrosspointDevice: public Device
{
public:
    CrosspointDevice (PluginConfig config, Hook h);
    virtual ~CrosspointDevice ();

    std::string getNameFromAudioPort (int audioport,
            crosspoint_direction_t direction);
    std::string getNameFromVideoPort (int videoport,
            crosspoint_direction_t direction);
    int getAudioPortFromName (std::string name);
    int getVideoPortfromName (std::string name);
    crosspoint_direction_t getDirectionFromName (std::string name);

    std::vector<CrosspointChannel> getAvailableInputs ();
    std::vector<CrosspointChannel> getAvailableOutputs ();

    std::map<std::string, std::string> getStatus ();

    static void runDeviceEvent (std::shared_ptr<Device> pdevice, PlaylistEntry *pevent);

    virtual void switchOP (std::string output, std::string input)=0;

protected:
    std::map<int, std::string> m_inputaudiotoname;
    std::map<int, std::string> m_inputvideotoname;
    std::map<int, std::string> m_outputaudiotoname;
    std::map<int, std::string> m_outputvideotoname;
    std::map<std::string, CrosspointChannel> m_channeldata;
    std::map<std::string, std::string> m_connectionmap;
    void m_readConfig (PluginConfig config);
};

