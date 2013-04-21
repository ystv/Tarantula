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
*   File Name   : CasparQueryResponseProcessor.h
*   Version     : 1.0
*   Description : Processes responses to CasparCG queries
*
*****************************************************************************/


#pragma once

#include "CasparCommand.h"
#include "CasparConnection.h"

/**
 * Handles parsing responses to CasparQueryCommands
 *
 */
class CasparQueryResponseProcessor
{
public:
    static void getMediaList (std::vector<std::string>& response,
            std::vector<std::string>& medialist);
    static void getTemplateList (std::vector<std::string>& response,
            std::vector<std::string>& templatelist);
    static int readLayerStatus (std::vector<std::string>& response,
            std::string& filename);
    static int readFileFrames(std::vector<std::string>& response);

};

