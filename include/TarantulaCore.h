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
*   File Name   : TarantulaCore.h
*   Version     : 1.0
*   Description : Global core components
*
*****************************************************************************/


#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "BaseConfigLoader.h"
#include "AsyncJobSystem.h"

// Forward declarations to save on #includes
class Log;
class Device;
class Channel;
struct PluginStateData;
class PlaylistEntry;

// Define callbacks
typedef std::function<void(std::string, int)> cbBegunPlaying;
typedef std::function<void(std::string, int)> cbEndPlaying;
typedef std::function<void(void)> cbTick;
typedef std::function<void(PlaylistEntry&, Channel*)> PreProcessorHandler;

struct DebugData
{
    double lastTickTimeUsed;

};

extern Log g_logger;
extern std::vector<cbBegunPlaying> g_begunplayingcallbacks;
extern std::vector<cbEndPlaying> g_endplayingcallbacks;
extern std::vector<cbTick> g_tickcallbacks;
extern std::shared_ptr<BaseConfigLoader> g_pbaseconfig;

extern std::map<std::string, std::shared_ptr<Device>> g_devices;
extern std::vector<PluginStateData> g_plugins;
extern std::unordered_map<std::string, PreProcessorHandler> g_preprocessorlist;

extern AsyncJobSystem g_async;
extern std::timed_mutex g_core_lock;

extern DebugData g_dbg;
