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
*   File Name   : CGDevice_Caspar.h
*   Version     : 1.0
*   Description : Caspar supports both CG and media. This plugin works with
*                 CG.
*
*****************************************************************************/

#pragma once

#include "CGDevice.h"
#include "libCaspar/libCaspar.h"

class CGDevice_Caspar: public CGDevice
{
public:
    CGDevice_Caspar (PluginConfig config, Hook h);
    virtual ~CGDevice_Caspar ();

    // Functions required by all devices
    void updateHardwareStatus();
    virtual void poll ();

    // Functions required by all CGDevices
    void add(std::string graphicname, int layer, std::map<std::string, std::string> *data);
    void remove(int layer);
    void play(int layer);
    void update(int layer, std::map<std::string, std::string> *data);

private:
    std::shared_ptr<CasparConnection> m_pcaspcon;

    //! Configured hostname and port number
    std::string m_hostname;
    std::string m_port;

    // Compositing layer to run CG events on
    int m_layer;

    // Callback functions for CasparCG commands
    void cb_info (std::vector<std::string>& resp);
    void cb_updatetemplates (std::vector<std::string>& resp);
};

