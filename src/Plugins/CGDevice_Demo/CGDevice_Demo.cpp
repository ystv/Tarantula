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
*   File Name   : CGDevice_Demo.cpp
*   Version     : 1.0
*   Description : Simple demonstration graphics device plugin
*
*****************************************************************************/


#include "CGDevice_Demo.h"
#include "CGDevice.h"

CGDevice_Demo::CGDevice_Demo (PluginConfig config, Hook h) :
        CGDevice(config, h)
{
    // Handled in parent class
    m_status = READY;


}

CGDevice_Demo::~CGDevice_Demo ()
{
    // No pointers
}

void CGDevice_Demo::updateHardwareStatus ()
{
    // Not really relevant.
}

void CGDevice_Demo::add (std::string graphicname, int layer,
        std::map<std::string, std::string> *pdata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to add layer while plugin in "
                "crashed state");
        return;
    }

    CGLayer newmap;

    newmap.graphicdata = *pdata;
    newmap.layer = layer;
    newmap.playstep = 0;
    newmap.graphicname = graphicname;

    if (m_plugindata.count(layer))
    {
        g_logger.warn("CGDevice_Demo::Add",
                "unable to add new item as the layer already exists");
        return;
    }
    else
    {
        m_plugindata[layer] = newmap;
    }

}

void CGDevice_Demo::remove (int layer)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to remove layer while plugin "
                "in crashed state");
        return;
    }

    // Remove an entry from plugindata corresponding to this layer, if it existed. Else throw
    if (m_plugindata.count(layer))
    {
        g_logger.warn("CGDevice_Demo::Remove",
                "unable to remove item as the layer doesn't exists");
        return;
    }
    else
    {
        m_plugindata.erase(layer);
    }
}

void CGDevice_Demo::play (int layer)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to play layer while plugin in"
                " crashed state");
        return;
    }

    // Increment playstep for this layer. If layer didn't exist, throw
    if (m_plugindata.count(layer))
    {
        m_plugindata[layer].playstep++;
    }
    else
    {
        g_logger.warn("CGDevice_Demo::Play",
                "Unable to advance layer as layer doesn't exist");
    }
}

void CGDevice_Demo::update (int layer, std::map<std::string, std::string> *pdata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to update layer while plugin "
                "in crashed state");
        return;
    }

    // Replace graphicdata for this layer.
    if (m_plugindata.count(layer))
    {
        m_plugindata[layer].graphicdata = *pdata;
        m_plugindata[layer].layer = layer;
    }
    else
    {
        g_logger.warn("CGDevice_Demo::Update",
                "Unable to update item as the layer doesn't exist");
    }
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<CGDevice_Demo> plugtemp = std::make_shared<CGDevice_Demo>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}

