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
*   File Name   : MouseCatcherProcessorPlugin.h
*   Version     : 1.0
*   Description : Base class for an EventProcessor
*
*****************************************************************************/


#pragma once

#include <TarantulaPlugin.h>

#include "MouseCatcherCommon.h"

struct ProcessorInformation
{
    std::string name;
    std::string description;
    std::map<std::string, std::string> data;
};

/**
 * Base class for an EventProcessor Plugin
 *
 * @param name ustring Name of this processor
 * @param h Hook Pointer back to the GlobalStuff structures
 */
class MouseCatcherProcessorPlugin: public Plugin
{
public:
    MouseCatcherProcessorPlugin (PluginConfig config, Hook h);
    virtual ~MouseCatcherProcessorPlugin ();
    virtual void handleEvent (MouseCatcherEvent originalEvent,
            MouseCatcherEvent *resultingEvent)=0;

    //! Get information about processor used for EventSources
    ProcessorInformation getProcessorInformation ();
protected:
    ProcessorInformation m_processorinfo;
};
