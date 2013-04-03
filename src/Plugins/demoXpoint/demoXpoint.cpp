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
*   File Name   : demoXpoint.cpp
*   Version     : 1.0
*   Description : Demonstration Crosspoint device
*
*****************************************************************************/


#include <TarantulaPlugin.h>
#include <CrosspointDevice.h>
#include <sstream>

class demoXpoint: public CrosspointDevice
{
public:
    demoXpoint (PluginConfig config, Hook h) :
            CrosspointDevice(config, h)
    {
        // Don't need to do anything specific here - parent callback will handle making use of config and hook.
        m_status = READY;
    }

    /**
     * Switch the given output stream to the specified input stream
     * @param output Output channel name to switch
     * @param input Input channel name to switch to
     */
    void switchOP (std::string output, std::string input)
    {
        if (READY != m_status)
        {
            m_hook.gs->L->error(m_pluginname, "Plugin not ready");
            return;
        }
        m_connectionmap[output] = input;

        std::stringstream logmsg;
        logmsg << "Output " << output << " set to Input " << input;
        m_hook.gs->L->info("Demo Crosspoint", logmsg.str());

    }

    /**
     * Update the status of the Crosspoint (ie port map)
     */
    void updateHardwareStatus ()
    {
        // Since this is a demo there's not a lot to do here either.
    }
};

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, void * pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        pluginref = new demoXpoint(config, h);
    }
}
