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
*   File Name   : Tarantula.cpp
*   Version     : 1.0
*   Description : Tarantula main file.
*
*****************************************************************************/


#include <iostream>
#include <queue>
#include <ctime>
#include <dirent.h> //for reading the directories
#include <unistd.h>
#include <algorithm>

#include "TarantulaPlugin.h" //this is the one place in the core where this is included.
#include "Log_Screen.h"
#include "Channel.h"
#include "Device.h"
#include "TarantulaPluginLoader.h"
#include "BaseConfigLoader.h"
#include "PluginConfigLoader.h"
#include "MouseCatcherCommon.h"
#include "MouseCatcherCore.h"
#include "Misc.h"

using std::cout;
using std::cerr;
using std::endl;

// Globals
Log g_logger;
std::vector<cbBegunPlaying> g_begunplayingcallbacks;
std::vector<cbEndPlaying> g_endplayingcallbacks;
std::vector<cbTick> g_tickcallbacks;
std::vector<Channel*> g_channels;
std::map<std::string, Device*> g_devices;
std::vector<PluginStateData> g_plugins;
//It doesn't work if I put these elsewhere, plugins unload each other ~SN
std::vector<MouseCatcherSourcePlugin*> g_mcsources;
std::map<std::string, MouseCatcherProcessorPlugin*> g_mcprocessors;
std::shared_ptr<BaseConfigLoader> g_pbaseconfig;
DebugData g_dbg;

// Functions used only in this file
static void processPluginStates ();
static void reloadPlugin (PluginStateData& state);

int main (int argc, char *argv[])
{
    GlobalStuff *gs = NewGS();
    gs->Channels = &g_channels; //copy from the global instance in Channel.cpp

    //Add channel tick to callback
    g_tickcallbacks.push_back(channelTick);
    g_begunplayingcallbacks.push_back(channelBegunPlaying);
    g_endplayingcallbacks.push_back(channelEndPlaying);

    //Add Device ticks to callback
    g_tickcallbacks.push_back(deviceTicks);

    //Add plugin tick handler
    g_tickcallbacks.push_back(processPluginStates);

    //Static register the screen log handler if modules fail
    Hook h;
    h.gs = gs;
    Log_Screen ls(h);

    // Load all non-Mousecatcher plugins
    g_pbaseconfig = std::make_shared<BaseConfigLoader>("config_base/Base.xml");
    loadAllPlugins("config_base/" + g_pbaseconfig->getDevicesPath(), "Device");
    loadAllPlugins("config_base/" + g_pbaseconfig->getInterfacesPath(),
            "Interface");
    loadAllPlugins("config_base/" + g_pbaseconfig->getLogsPath(), "Logger");
    g_logger.info("Tarantula Core",
            "Config loaded. System name is: " + g_pbaseconfig->getSystemName());

    // Run all the channel constructors from the config file's details
    std::vector<ChannelDetails> loadedchannels = g_pbaseconfig->getLoadedChannels();

    for (ChannelDetails thischannel : loadedchannels)
    {
        Channel *pcl;

        try
        {
            pcl = new Channel(thischannel.m_channame, thischannel.m_xpname,
                    thischannel.m_xpport);
            g_channels.push_back(pcl);
        }
        catch (std::exception&)
        {
            g_logger.info("Initialisation", "Exception caught when creating channel");
        }
    }

    // Initialise MouseCatcher
    MouseCatcherCore::init("config_base/" + g_pbaseconfig->getEventSourcesPath(),
            "config_base/" + g_pbaseconfig->getEventProcessorsPath());

    // Tick loop with length set by framerate
    while (1)
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        timespec begin = ts;

        // Call all registered tick callbacks
        tick();

        // Check plugin health
        processPluginStates();

        clock_gettime(CLOCK_MONOTONIC, &ts);
        timespec end = ts;
        clock_t diff = end.tv_nsec - begin.tv_nsec;
        if (diff < 0)
        {
            // We're across seconds
            clock_t sdiff = end.tv_sec - begin.tv_sec;
            diff = sdiff * 1000000000 + end.tv_nsec - begin.tv_nsec;
        }
        clock_t remaining = (1000000000 / g_pbaseconfig->getFramerate()) - diff;
        remaining /= 1000;
        g_dbg.lastTickTimeUsed = (double) diff / 1000000;

        if (remaining < 0)
        {
            g_logger.warn("Tarantula Main",
                    "That tick took "+ ConvertType::floatToString((double) diff / 1000000)
                            + "ms - Too Long!");
            continue;
        }
        usleep(remaining);
    }
    return 0;
}

/**
 * Unload and reload a crashed plugin
 */
void reloadPlugin (PluginStateData& state)
{
    std::string file = state.ppluginreference->getConfigFilename();

    // Unload the plugin
    delete state.ppluginreference;
    state.ppluginreference = NULL;

    state.reloadsremaining--;

    // Reload the plugin
    PluginConfigLoader plugin_config;
    plugin_config.loadConfig(file, state.type);
    LoadPlugin(plugin_config.getConfig(), &state.ppluginreference);

}

/**
 * Iterate over all loaded plugins and process the status state machine
 */
void processPluginStates ()
{
    // Process the state machine on all plugins
    for (PluginStateData& pluginstate : g_plugins)
    {
        switch (pluginstate.ppluginreference->getStatus())
        {
            case STARTING:
            {
                g_logger.warn(pluginstate.ppluginreference->getPluginName(),
                        "Plugin still marked as starting.");
                break;
            }
            case FAILED:
            {
                if (pluginstate.reloadsremaining > 0)
                {
                    g_logger.info(pluginstate.ppluginreference->getPluginName(),
                            "Reloading plugin after startup failure");
                    reloadPlugin(pluginstate);
                }
                else
                {
                    g_logger.OMGWTF(pluginstate.ppluginreference->getPluginName(),
                            "Plugin startup failure is permanent. Unloading plugin");
                    pluginstate.ppluginreference->disablePlugin();
                    pluginstate.ppluginreference = NULL;
                }
                break;
            }
            case CRASHED:
            {
                if (pluginstate.reloadsremaining > 0)
                {
                    g_logger.info(pluginstate.ppluginreference->getPluginName(),
                            "Reloading plugin after earlier failure");
                    reloadPlugin(pluginstate);
                }
                else
                {
                    g_logger.OMGWTF(pluginstate.ppluginreference->getPluginName(),
                            "Plugin failure is permanent. Disabling plugin");
                    pluginstate.ppluginreference->disablePlugin();
                    pluginstate.ppluginreference = NULL;
                }
                break;
            }
            case UNLOAD:
            {
                g_logger.info(pluginstate.ppluginreference->getPluginName(),
                        "Unload requested. Unloading plugin");
                pluginstate.ppluginreference->disablePlugin();
                pluginstate.ppluginreference = NULL;
                break;
            }
            case READY:
            case WAITING:
            default:
            {
                // Nothing to do here
                break;
            }

        }
    }

    // Delete all plugins in state CLEANUP
    g_plugins.erase(std::remove_if(g_plugins.begin(), g_plugins.end(),
                    [](PluginStateData p){return NULL == p.ppluginreference;}), g_plugins.end());

}

