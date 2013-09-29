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
 *   File Name   : EventProcessor_GFXHelper.h
 *   Version     : 1.0
 *   Description : Generate events to add a graphics item and remove it later.
 *
 *****************************************************************************/

#pragma once

#include "MouseCatcherProcessorPlugin.h"
#include "PluginConfig.h"

/**
 * Class to generate a CG add and CG remove event.
 */
class EventProcessor_GFXHelper : public MouseCatcherProcessorPlugin
{
public:
    EventProcessor_GFXHelper (PluginConfig config, Hook h);
    virtual ~EventProcessor_GFXHelper ();

    void handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent);

private:
    std::string m_gfxdevice;
};

