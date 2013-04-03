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
*   File Name   : CGDevice_Demo.h
*   Version     : 1.0
*   Description : Simple demonstration graphics device plugin
*
*****************************************************************************/


#include "CGDevice.h"
#include <string>
#include <map>

class PluginConfig;
class Hook;


class CGDevice_Demo : CGDevice {
public:
    CGDevice_Demo (PluginConfig config, Hook h);
    virtual ~CGDevice_Demo ();

    // Functions required by all devices
    void updateHardwareStatus();

    // Functions required by all CGDevices
    void add(std::string graphicname, int layer, std::map<std::string, std::string> *data);
    void remove(int layer);
    void play(int layer);
    void update(int layer, std::map<std::string, std::string> *data);

private:
    std::map<int, CGLayer> m_plugindata;
};
