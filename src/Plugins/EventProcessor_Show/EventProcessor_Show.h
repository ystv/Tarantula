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
 *   File Name   : EventProcessorShow.h
 *   Version     : 1.0
 *   Description : 
 *
 *****************************************************************************/

#pragma once

#include "MouseCatcherProcessorPlugin.h"

/**
 * Class to generate a video event, a continuity fill and optional Now/Next overlays
 */
class EventProcessor_Show: public MouseCatcherProcessorPlugin
{
public:
    EventProcessor_Show (PluginConfig config, Hook h);
    virtual ~EventProcessor_Show ();

    void handleEvent (MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent);

private:
    bool m_enableoverlay; //!< Should periodic Now/Next overlay graphics be shown?

    std::string m_continuitygenerator; //!< Name of EventProcessor_Fill instance
    std::string m_videodevice;         //!< Name of VideoDevice for show
    std::string m_cgdevice;            //!< Name of CGDevice for overlay
    int m_continuitylength;            //!< Length of filled continuity event at start

    std::string m_nownextname;  //!< Name of graphic for overlay events
    int m_nownextperiod;        //!< How often to add a now/next overlay
    int m_nownextminimum;       //!< Minimum video length where an overlay will be included
    int m_nownextduration;      //!< How long to show the now/next event for
    int m_nownextlayer;         //!< Template host layer to render overlays with
};

