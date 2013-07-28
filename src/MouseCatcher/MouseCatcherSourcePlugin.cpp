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
*   File Name   : MouseCatcherSourcePlugin.cpp
*   Version     : 1.0
*   Description : Base class for an EventSource
*
*****************************************************************************/


#include <queue>

#include "MouseCatcherCommon.h"
#include "MouseCatcherSourcePlugin.h"
#include "MouseCatcherCore.h"
#include "PluginConfig.h"
#include "Misc.h"
#include "TarantulaPlugin.h"

/**
 * Setup required by all source plugins, setting name and hook
 *
 * @param config The configuration data for this plugin.
 * @param h      Link back to GlobalStuff structures
 */
MouseCatcherSourcePlugin::MouseCatcherSourcePlugin (PluginConfig config, Hook h) :
        Plugin(config, h)
{
}

/**
 * Destructor
 */
MouseCatcherSourcePlugin::~MouseCatcherSourcePlugin ()
{

}

/**
 * Add this plugin to the list managing this type
 *
 *  @param thisplugin Pointer to new plugin
 */
void MouseCatcherSourcePlugin::addPluginReference (std::shared_ptr<Plugin> thisplugin)
{
    std::shared_ptr<MouseCatcherSourcePlugin> thissource =
            std::dynamic_pointer_cast<MouseCatcherSourcePlugin>(thisplugin);
    g_mcsources.push_back(std::shared_ptr<MouseCatcherSourcePlugin>(thissource));
}

/**
 * Static predicate used to erase completed items from the ActionQueue
 * @param a		 The EventAction checked
 * @param plugin The plugin running the queue checks
 * @return		 True if the EventAction is processed and owned by the plugin
 */
bool MouseCatcherSourcePlugin::actionCompleteCheck (const EventAction &a,
		const MouseCatcherSourcePlugin *plugin)
{
	return (a.isprocessed) && (a.thisplugin == plugin);
}

