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
*   File Name   : CasparCommand.h
*   Version     : 1.0
*   Description : Handles forming and sending commands to CasparCG
*
*****************************************************************************/


#pragma once

#include <string>
#include <map>
#include <vector>

enum CasparCommandType
{
    // Media commands
    CASPAR_COMMAND_LOAD,
    CASPAR_COMMAND_LOADBG,
    CASPAR_COMMAND_PLAY,
    CASPAR_COMMAND_STOP,
    CASPAR_COMMAND_CLEAR,
    CASPAR_COMMAND_CLEAR_PRODUCER,
    CASPAR_COMMAND_SEEK,

    // Data commands
    CASPAR_COMMAND_DATA_STORE,
    CASPAR_COMMAND_DATA_RETRIEVE,
    CASPAR_COMMAND_DATA_LIST,

    // CG Commands
    CASPAR_COMMAND_CG_ADD,
    CASPAR_COMMAND_CG_REMOVE,
    CASPAR_COMMAND_CG_CLEAR,
    CASPAR_COMMAND_CG_PLAY,
    CASPAR_COMMAND_CG_STOP,
    CASPAR_COMMAND_CG_NEXT,
    CASPAR_COMMAND_CG_GOTO,
    CASPAR_COMMAND_CG_UPDATE,
    CASPAR_COMMAND_CG_INVOKE,

    // Statistics and Status
    CASPAR_COMMAND_CINF,
    CASPAR_COMMAND_CLS,
    CASPAR_COMMAND_TLS,
    CASPAR_COMMAND_VERSION,
    CASPAR_COMMAND_INFO,
    CASPAR_COMMAND_INFO_EXPANDED,

    // Misc Commands
    CASPAR_COMMAND_BYE
};

/**
 * This class is pretty dumb - it just strings commands together.
 * it would be possible (and recommended really) to extend this class to make it
 * a bit more specific to each command type.
 */
class CasparCommand
{
public:
    CasparCommand (CasparCommandType cct);
    void addParam (std::string param);
    void clearParams ();
    std::string form (); // Form the command to a string
protected:
    //!Can only do this from a subclass
    CasparCommand ();
    void setType (CasparCommandType cct);
private:
    std::vector<std::string> m_params; // Components of the command
    std::map<CasparCommandType, std::string> CasparCommandTypeText;
    std::string getCommandQuery (CasparCommandType command);
    std::string m_commandquery;
};
