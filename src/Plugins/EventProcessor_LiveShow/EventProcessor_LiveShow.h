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
 *   File Name   : EventProcessor_LiveShow.h
 *   Version     : 1.0
 *   Description : Generate events to support a live show
 *
 *****************************************************************************/

#pragma once

#include "MouseCatcherProcessorPlugin.h"
#include "Misc.h"

class EventProcessor_LiveShow : public MouseCatcherProcessorPlugin
{
public:
    EventProcessor_LiveShow (PluginConfig config, Hook h);
    virtual ~EventProcessor_LiveShow ();

    void handleEvent(MouseCatcherEvent originalEvent, MouseCatcherEvent& resultingEvent);

private:
    bool m_enableoverlay; //!< Should periodic Now/Next overlay graphics be shown?

    std::string m_continuitygenerator; //!< Name of EventProcessor_Fill instance
    std::string m_crosspointdevice;    //!< Name of CrosspointDevice to switch for show
    std::string m_liveinput;           //!< Name of input for show
    std::string m_videoinput;          //!< Name of input to go back to
    std::string m_xpoutput;            //!< Name of output

    std::string m_cgdevice;            //!< Name of CGDevice for overlay
    int m_continuitylength;            //!< Length of filled continuity event at start

    std::string m_nownextname;  //!< Name of graphic for overlay events
    int m_nownextperiod;        //!< How often to add a now/next overlay
    int m_nownextminimum;       //!< Minimum video length where an overlay will be included
    int m_nownextduration;      //!< How long to show the now/next event for
    int m_nownextlayer;         //!< Template host layer to render overlays with

    std::string m_vtdevice;     //!< Device to play warning VT clock
    std::string m_vtfile;       //!< Filename of warning VT clock
    int         m_vtduration;   //!< Duration of warning clock
};

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<EventProcessor_LiveShow> plugtemp = std::make_shared<EventProcessor_LiveShow>(config, h);
        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}
