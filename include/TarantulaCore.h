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

#include "BaseConfigLoader.h"

// Forward declarations to save on #includes
class Log;
class Device;
struct PluginStateData;


// Define callbacks
typedef void (*cbBegunPlaying) (std::string, int);
typedef void (*cbEndPlaying) (std::string, int);
typedef void (*cbTick) ();

struct DebugData
{
    double lastTickTimeUsed;

};

extern Log g_logger;
extern std::vector<cbBegunPlaying> g_begunplayingcallbacks;
extern std::vector<cbEndPlaying> g_endplayingcallbacks;
extern std::vector<cbTick> g_tickcallbacks;
extern BaseConfigLoader g_baseconfig;

extern std::map<std::string, Device*> g_devices;
extern std::vector<PluginStateData> g_plugins;

extern DebugData g_dbg;
