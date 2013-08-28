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
*   File Name   : EventProcessor_Demo.h
*   Version     : 1.0
*   Description : Demonstration EventProcessor
*
*****************************************************************************/


#pragma once

#include "MouseCatcherProcessorPlugin.h"
#include "PluginConfig.h"

/**
 * A sample EventProcessor, should spit back the original event on the first device,
 * plus a child on the same device.
 *
 * @param config Configuration data for the plugin
 * @param h      Link back to GlobalStuff structures
 */
class EventProcessor_Demo: public MouseCatcherProcessorPlugin
{
public:
    EventProcessor_Demo (PluginConfig config, Hook h);
    virtual ~EventProcessor_Demo ();
    virtual void handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent);

    static void demoPreProcessor (PlaylistEntry& event, Channel *pchannel);
};

