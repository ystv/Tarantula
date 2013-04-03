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
*   File Name   : CasparFlashCommand.h
*   Version     : 1.0
*   Description : Helper class for CG flash template commands
*
*****************************************************************************/


#pragma once

#include "CasparCommand.h"

/**
 * Abstracts away some flash/CG control logic, in particular
 * XML data generation
 *
 * @param layer int The channel to operate on
 */
class CasparFlashCommand: public CasparCommand
{
public:
    CasparFlashCommand (int channel);
    virtual ~CasparFlashCommand ();
    void setLayer (int layer);
    void setHostLayer (int layer);
    void addData (std::string key, std::string value);
    void clearData ();
    void play ();
    void play (std::string templatename);
    void load (std::string templatename);
    void stop ();
    void next ();
    void clear ();
    void update ();
private:
    void addChannelAndLayers ();
    void formatAndAddTemplateData ();
    int m_channelnumber;
    int m_layernumber;
    int m_templatehostlayer;
    std::map<std::string, std::string> m_templatedata;
};

