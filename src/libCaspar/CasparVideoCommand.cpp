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
*   File Name   : CasparVideoCommand.cpp
*   Version     : 1.0
*   Description : Handles sending a video command to Caspar allowing
*                 specification of channels etc
*
*****************************************************************************/


#include "libCaspar/CasparCommand.h"
#include "Misc.h"
#include "libCaspar/CasparVideoCommand.h"

/**
 * Constructor
 *
 * @param channel int The channel number to execute on
 */
CasparVideoCommand::CasparVideoCommand (int channel) :
        CasparCommand()
{
    m_channelnumber = channel;
    // Default if not set
    m_layernumber = 1;
    m_transitionframes = 0;
    m_transitiontype = CASPAR_TRANSITION_MIX;

}

CasparVideoCommand::~CasparVideoCommand ()
{

}

/**
 * Sets the layer inside the channel to run this command on,
 * otherwise defaults to 1
 *
 * @param layer int The layer to use
 */
void CasparVideoCommand::setLayer (int layer)
{
    m_layernumber = layer;
}

/**
 * Adds the channel and layer to the command as a parameter
 */
void CasparVideoCommand::addChannelAndLayer ()
{
    addParam(ConvertType::intToString(m_channelnumber));
    addParam(ConvertType::intToString(m_layernumber));
}

/**
 * Set the transition to use for this command
 *
 * @param transition CasparTransitionType The type of transition, defaults to CUT
 * @param frames int Number of frames for transition to last
 */
void CasparVideoCommand::setTransition (CasparTransitionType transition,
        int frames)
{
    m_transitiontype = transition;
    m_transitionframes = frames;
}

/**
 * Calls Play on this layer
 */
void CasparVideoCommand::play ()
{
    clearParams();
    setType(CASPAR_COMMAND_PLAY);

    // Add the channel and layer
    addChannelAndLayer();
}

/**
 * Plays the specified video file immediately
 *
 * @param filename string The name of the file to play (As CasparCG knows it).
 */
void CasparVideoCommand::play (std::string filename)
{
    clearParams();
    setType(CASPAR_COMMAND_PLAY);

    // Add the channel and layer
    addChannelAndLayer();

    // Add the filename
    if (filename != "")
        addParam(filename);

    // Add transition details
    addParam(getTransitionType(m_transitiontype));
    addParam(ConvertType::intToString(m_transitionframes));
}

/**
 * Loads the specified video file in the background, with autoplay capability
 *
 * @param filename string The name of the file to play (As CasparCG knows it)
 * @param autoplay bool Whether to set up for automatic playback as soon as last file ends
 */
void CasparVideoCommand::load (std::string filename, bool autoplay)
{
    clearParams();

    // Not really much point in non-background loads
    setType(CASPAR_COMMAND_LOADBG);

    // Channel and layer
    addChannelAndLayer();

    // Add the filename
    addParam(filename);

    // Add transition details
    addParam(getTransitionType(m_transitiontype));
    addParam(ConvertType::intToString(m_transitionframes));

    // Add autoplay if needed
    if (autoplay)
        addParam("AUTO");
}

/**
 * Stops the specified layer playing
 */
void CasparVideoCommand::stop ()
{
    clearParams();
    setType(CASPAR_COMMAND_STOP);
    addChannelAndLayer();
}

/**
 * Clears the specified layer
 */
void CasparVideoCommand::clear ()
{
    clearParams();
    setType(CASPAR_COMMAND_CLEAR);
    addChannelAndLayer();
}

/**
 * Seek to the specified frame number in this layer
 *
 * @param frame int Frame number to seek to
 */
void CasparVideoCommand::seek (int frame)
{
    clearParams();
    setType(CASPAR_COMMAND_SEEK);
    addChannelAndLayer();
    addParam(ConvertType::intToString(frame));
}

/**
 * Get the string equivalent of a transition type
 *
 * @todo Add the rest of the transition types
 */
std::string CasparVideoCommand::getTransitionType (CasparTransitionType type)
{
    std::string ret = "";
    switch (type)
    {
        case CASPAR_TRANSITION_MIX:
            ret = "MIX";
            break;
        case CASPAR_TRANSITION_WIPE:
            ret = "WIPE";
            break;
        case CASPAR_TRANSITION_PUSH:
            ret = "PUSH";
            break;
        case CASPAR_TRANSITION_SLIDE:
            ret = "SLIDE";
            break;
        default:
            ret = "CUT";
            break;
    }
    return ret;
}
