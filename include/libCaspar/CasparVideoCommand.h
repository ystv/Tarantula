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
*   File Name   : CasparVideoCommand.h
*   Version     : 1.0
*   Description : Handles sending a video command to Caspar allowing
*                 specification of channels etc
*
*****************************************************************************/


#pragma once

#include "CasparCommand.h"

enum CasparTransitionType
{
    CASPAR_TRANSITION_CUT,
    CASPAR_TRANSITION_MIX,
    CASPAR_TRANSITION_WIPE,
    CASPAR_TRANSITION_PUSH,
    CASPAR_TRANSITION_SLIDE
};

/**
 * Abstracts away some of CasparCommand to provide an interface
 * to generate video commands
 * Note that play(), load() etc just set up the command, you have to
 * send it to a CasparConnection to make it run
 */
class CasparVideoCommand: public CasparCommand
{
public:
    CasparVideoCommand (int channel);
    virtual ~CasparVideoCommand ();
    void setLayer (int layer);
    void setTransition (CasparTransitionType transition, int frames);
    void play ();
    void play (std::string filename);
    void load (std::string filename, bool autoplay);
    void stop ();
    void clear ();
    void seek (int frames);
private:
    void addChannelAndLayer ();
    int m_channelnumber;
    int m_layernumber;
    int m_transitionframes;
    CasparTransitionType m_transitiontype;
    std::string getTransitionType (CasparTransitionType type);
};

