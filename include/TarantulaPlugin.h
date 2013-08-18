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
*   File Name   : TarantulaPlugin.h
*   Version     : 1.0
*   Description : Holds a couple of structs for plugins to latch onto.
*
*****************************************************************************/


#pragma once

#include <map>
#include <vector>
#include <string>
#include <exception>
#include <functional>

#include "PluginConfig.h"
#include "Channel.h"
#include "Log.h"
#include "AsyncJobSystem.h"

class Log;
class Device;
class Plugin;

// Callbacks
typedef std::function<void(std::string, int)> cbBegunPlaying;
typedef std::function<void(std::string, int)> cbEndPlaying;
typedef std::function<void(void)> cbTick;

#define TPlugin

// Struct passed to all plugins to enable access to parts of core.
struct GlobalStuff
{
    std::map<std::string, std::shared_ptr<Device>> *Devices;
    std::vector<std::shared_ptr<Channel>> *Channels;
    DebugData *dbg;

    Log *L; //Global Instance of a logging class.

    AsyncJobSystem *Async;

    // Callbacks
    std::vector<cbBegunPlaying> *BegunPlayingCallbacks;
    std::vector<cbEndPlaying> *EndPlayingCallbacks;
    std::vector<cbTick> *TickCallbacks;
};

struct Hook
{
    GlobalStuff *gs;
    PluginConfig cfg;
};

// Function prototypes defined in CallBackTools.cpp
void tick ();
void begunPlaying (std::string name, int id);
void EndPlaying (std::string name, int id);

//Define plugin functions
typedef void (*LoadPluginFunc) (Hook, PluginConfig, std::shared_ptr<Plugin>&);
//End Define

/**
 * Internal plugin status
 */
enum plugin_status_t
{
    READY,   //!< Plugin is up and running healthily
    STARTING,   //!< Plugin constructor is running
    WAITING, //!< Plugin is waiting for an event to complete or timeout
    FAILED,  //!< Plugin setup failed or timed out.
    CRASHED, //!< Plugin worked once, but has now failed.
    UNLOAD,  //!< Plugin wishes to be unloaded
};

/**
 * Base class for all plugins, provides some plugin admin stuff
 */
class Plugin
{
public:
    Plugin (PluginConfig config, Hook h);
    virtual ~Plugin();

    virtual void addPluginReference (std::shared_ptr<Plugin> thisplugin);
    std::string getPluginName ();
    plugin_status_t getStatus ();
    // Get the name of the config file the plugin was loaded from (for reloading)
    std::string getConfigFilename ();
    // Disable the plugin if permanently crashed
    void disablePlugin();
protected:
    std::string m_pluginname;
    Hook m_hook;
    plugin_status_t m_status;
private:
    std::string m_configfile;
};

/**
 * Plugin state reference used by plugin state manager
 */
struct PluginStateData
{
    std::shared_ptr<Plugin> ppluginreference;
    int reloadsremaining;
    int reloadtimer;
    std::string type;
};

