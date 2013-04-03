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
*   File Name   : TarantulaPluginLoader.cpp
*   Version     : 1.0
*   Description : Functions for loading plugins
*
*****************************************************************************/


#define PLUGINLOADER // This is a bit of a special case so needs to announce itself as such.
#include "TarantulaCore.h"
#include "TarantulaPlugin.h"
#include <sstream>
#include <dlfcn.h>
#include "TarantulaPluginLoader.h"

/**
 * Initialise a new GlobalStuff struct with the relevant pointers
 *
 * @return New GlobalStuff struct
 */
GlobalStuff* NewGS ()
{
    GlobalStuff* pgs = new GlobalStuff();
    pgs->L = &g_logger;
    pgs->Channels = &g_channels;
    pgs->BegunPlayingCallbacks = &g_begunplayingcallbacks;
    pgs->EndPlayingCallbacks = &g_endplayingcallbacks;
    pgs->TickCallbacks = &g_tickcallbacks;
    pgs->Devices = &g_devices;
    pgs->dbg = &g_dbg;
    return pgs;
}

/**
 * Load a plugin from a specified config file
 *
 * @param cfg  Configuration data for the plugin to load
 * @param pref Pointer be used as the plugin reference, filled by the plugin
 * itself
 */
void LoadPlugin (PluginConfig cfg, void* pref)
{
    std::stringstream logmsg; // For assembling log messages

    // Try to open the plugin file
    void* ppluginf = dlopen(("bin/" + cfg.m_library).c_str(), RTLD_LAZY);
    if (!ppluginf)
    {
        std::stringstream logmsg;
        logmsg << "Could not open plugin file " << cfg.m_library << " "
                << dlerror();
        g_logger.error("Plugin Loader", logmsg.str());
        return;
    }

    void *ppluginhdl = dlsym(ppluginf, "LoadPlugin");
    if (!ppluginhdl)
    {
        logmsg
                << "Could not get Loadplugin function from SO file. Is it a valid plugin?";
        g_logger.error("Plugin Loader", logmsg.str());
        dlclose(ppluginf);
        return;
    }
    // So by this point we have a handle to the plugin function. Now let's call it.
    LoadPluginFunc plugfunc = (LoadPluginFunc) ppluginhdl;
    Hook h;
    h.gs = NewGS();
    return plugfunc(h, cfg, pref);
}

/**
 * Unload the specified plugin
 * @param filename
 */
void unLoad (const char* filename)
{
    //TODO: Write me.
}
