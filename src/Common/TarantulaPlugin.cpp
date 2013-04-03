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
*   File Name   : TarantulaPlugin.cpp
*   Version     : 1.0
*   Description : Implements the base class for all plugins.
*
*****************************************************************************/


#include "TarantulaPlugin.h"

Plugin::Plugin (PluginConfig config, Hook h)
{
    m_status = STARTING;
    m_pluginname = config.m_instance;
    m_hook = h;
    m_configfile = config.m_filename;
}

std::string Plugin::getPluginName ()
{
    return m_pluginname;
}

plugin_status_t Plugin::getStatus ()
{
    return m_status;
}

std::string Plugin::getConfigFilename ()
{
    return m_configfile;
}
