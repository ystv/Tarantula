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
*   File Name   : CasparCommand.cpp
*   Version     : 1.0
*   Description : Handles forming and sending commands to CasparCG
*
*****************************************************************************/


#include "libCaspar/libCaspar.h"
#include <sstream>

/**
 * Form a stub command with no details
 */
CasparCommand::CasparCommand ()
{

}

/**
 * Form a command with a known command type
 *
 * @param cct CasparCommandType The type of command
 */
CasparCommand::CasparCommand (CasparCommandType cct)
{
    setType(cct);
}

/**
 * Add a parameter to the command
 *
 * @param param string The string to add to the end of the command
 */
void CasparCommand::addParam (std::string param)
{
    m_params.push_back(param);
}

/**
 * Remove all parameters from this command
 */
void CasparCommand::clearParams ()
{
    m_params.clear();
}

/**
 * Set the type of this command
 *
 * @param cct CasparCommandType The type of command
 */
void CasparCommand::setType (CasparCommandType cct)
{
    m_commandquery = getCommandQuery(cct);
}

/**
 * Form a complete command ready to send to CasparCG
 *
 * @return string The complete command string
 */
std::string CasparCommand::form ()
{
    int found = -1;
    // Replace all ?s with entries in m_params
    for (std::vector<std::string>::iterator it = m_params.begin();
            it != m_params.end(); it++)
    {
        found = m_commandquery.find("?", found + 1);
        if (found == static_cast<int>(std::string::npos))
        {
            break;
        }

        m_commandquery.erase(found, 1);
        m_commandquery.insert(found, (*it));
        found += (*it).length();
    }

    //Remove any remaining ?s
    found = m_commandquery.find("?", found + 1);
    while (found != static_cast<int>(std::string::npos))
    {
        m_commandquery.erase(found, 1);
        found = m_commandquery.find("?", found + 1);
    }

    return m_commandquery;
}

/**
 * Returns the command stub string equivalent to that command
 *
 * @param command CasparCommandType The command type to look up a string for
 * @return string The command stub for the input command type
 */
std::string CasparCommand::getCommandQuery (CasparCommandType command)
{
    std::string ret = "FAIL\r\n";
    switch (command)
    {
        //media commands
        //Usually something like LOAD chan-layer clip transition duration auto?
        case CASPAR_COMMAND_LOAD:
            ret = "LOAD ?-? ? ? ?\r\n";
            break;
        case CASPAR_COMMAND_LOADBG:
            ret = "LOADBG ?-? ? ? ? ?\r\n";
            break;
        case CASPAR_COMMAND_PLAY:
            ret = "PLAY ?-? ? ? ?\r\n";
            break;
        case CASPAR_COMMAND_STOP:
            ret = "STOP ?-?\r\n";
            break;
        case CASPAR_COMMAND_CLEAR:
            ret = "CLEAR ?-?\r\n";
            break;
        case CASPAR_COMMAND_CLEAR_PRODUCER:
            ret = "CLEAR ?\r\n";
            break;
        case CASPAR_COMMAND_SEEK:
            ret = "CALL ?-? SEEK ?\r\n";
            break;
            //data commands
        case CASPAR_COMMAND_DATA_STORE:
            ret = "DATA STORE\r\n";
            break;
        case CASPAR_COMMAND_DATA_RETRIEVE:
            ret = "DATA RETRIEVE\r\n";
            break;
        case CASPAR_COMMAND_DATA_LIST:
            ret = "DATA LIST\r\n";
            break;
            //CG Commands
            //Usually CG chan-layer COMMAND hostlayer template autoplay data
        case CASPAR_COMMAND_CG_ADD:
            ret = "CG ?-? ADD ? ? ? ?\r\n";
            break;
        case CASPAR_COMMAND_CG_REMOVE:
            ret = "CG ?-? REMOVE ?\r\n";
            break;
        case CASPAR_COMMAND_CG_CLEAR:
            ret = "CG ?-? CLEAR\r\n";
            break;
        case CASPAR_COMMAND_CG_PLAY:
            ret = "CG ?-? PLAY ?\r\n";
            break;
        case CASPAR_COMMAND_CG_STOP:
            ret = "CG ?-? STOP ?\r\n";
            break;
        case CASPAR_COMMAND_CG_NEXT:
            ret = "CG ?-? NEXT ?\r\n";
            break;
        case CASPAR_COMMAND_CG_GOTO:
            ret = "CG GOTO\r\n";
            break;
        case CASPAR_COMMAND_CG_UPDATE:
            ret = "CG ?-? UPDATE ? ?\r\n";
            break;
        case CASPAR_COMMAND_CG_INVOKE:
            ret = "CG ?-? INVOKE ? ?\r\n";
            break;
            //Statistics and Status
        case CASPAR_COMMAND_CINF:
            ret = "CINF\r\n";
            break;
        case CASPAR_COMMAND_CLS:
            ret = "CLS\r\n";
            break;
        case CASPAR_COMMAND_TLS:
            ret = "TLS\r\n";
            break;
        case CASPAR_COMMAND_VERSION:
            ret = "VERSION\r\n";
            break;
        case CASPAR_COMMAND_INFO:
            ret = "INFO ?-?\r\n";
            break;
        case CASPAR_COMMAND_INFO_EXPANDED:
            ret = "INFO ?\r\n";
            break;
            //Misc Commands
        case CASPAR_COMMAND_BYE:
            ret = "BYE\r\n";
            break;
        default:
            break;
    }
    return ret;
}
