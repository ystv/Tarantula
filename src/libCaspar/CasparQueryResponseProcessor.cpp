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
*   File Name   : CasparQueryResponseProcessor.cpp
*   Version     : 1.0
*   Description : Processes responses to CasparCG queries
*
*****************************************************************************/


#include <sstream>

#include "pugixml.hpp"
#include "libCaspar/CasparQueryResponseProcessor.h"
#include "Misc.h"

/**
 * Get a list of names and lengths of media files
 *
 * @param medialist Map of filenames and their lengths (in frames)
 * @param response  Lines of data returned from CasparCG
 */
void CasparQueryResponseProcessor::getMediaList (std::vector<std::string>& response,
        std::map<std::string, long>& medialist)
{

    // Parse the response (skipping first two lines: return code and message)
    for (std::vector<std::string>::iterator it = response.begin() + 2;
            it != response.end(); it++)
    {
        // Find out where media name ends
        int nameend = (*it).find("\"", 1);

        // Extract character 1 (0 is ") up to next double quote as media name
        std::string medianame = (*it).substr(1, nameend - 1);

        // Beyond the media name should be "  MOVIE  " followed by frames (sans quotes), 9 chars
        nameend += 9;

        // And beyond that should be a number of frames followed by a space
        long frames = ConvertType::stringToInt(
                (*it).substr(nameend, (*it).find(" ", nameend)));
        medialist[medianame] = frames;
    }
}

/**
 * Get a list of template names
 *
 * @param templatelist List of template names
 * @param response     Lines of data returned from CasparCG
 *
 */
void CasparQueryResponseProcessor::getTemplateList (std::vector<std::string>& response,
        std::vector<std::string>& templatelist)
{
    // Parse the response (skipping first two lines: return code and message)
    for (std::vector<std::string>::iterator it = response.begin() + 1;
            it != response.end(); it++)
    {
        std::string templatename = (*it).substr(1, (*it).find("\"", 1) - 1);

        // Try and trim a leading backslash
        if (templatename[0] == '\\')
        {
            templatename.erase(0, 1);
        }

        templatelist.push_back(templatename);
    }
}

/**
 * Read the state of a channel and layer
 *
 * @param response Data received from CasparCG
 * @param filename A string for the file name
 * @return         Number of frames to play, 0 for stopped, -1 for not playing
 */
int CasparQueryResponseProcessor::readLayerStatus (std::vector<std::string>& response,
        std::string& filename)
{
    std::stringstream ss;
    ss << response[2];

    // Some defaults
    int framesremaining = -1;

    // Load the XML parser
    pugi::xml_document xmldoc;
    pugi::xml_parse_result parseresult = xmldoc.load(ss);

    if (pugi::status_ok == parseresult)
    {
        // Get the producer
        pugi::xml_node producer;
        try
        {
            producer =
                    xmldoc.select_single_node(
                            "//channel/stage/layers/layer/foreground/producer[type=\"ffmpeg-producer\"]").node();
        } catch (...)
        {
            // Not playing, so exit
            return -1;
        }

        framesremaining =
                producer.parent().parent().child("frames-left").text().as_int(-1);
        filename = producer.child_value("filename");

        // Chop off the file path and extension
        filename = filename.substr(0, filename.length() - 3);

    }

    return framesremaining;
}

