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
*   File Name   : CasparFlashCommand.cpp
*   Version     : 1.0
*   Description : Helper class for CG flash template commands
*
*****************************************************************************/


#include <sstream>

#include "pugixml.hpp"
#include "libCaspar/CasparFlashCommand.h"
#include "Misc.h"

CasparFlashCommand::CasparFlashCommand (int channel) :
        CasparCommand()
{
    m_channelnumber = channel;

    // Default if not set
    m_layernumber = 1;
    m_templatehostlayer = 1;

}

CasparFlashCommand::~CasparFlashCommand ()
{

}

/**
 * Sets the layer inside the channel to run this command on,
 * otherwise defaults to 1
 *
 * @param layer int The layer to use
 */
void CasparFlashCommand::setLayer (int layer)
{
    m_layernumber = layer;
}

/**
 * Sets the template host layer inside the layer to run this command on,
 * otherwise defaults to 1
 *
 * @param layer int The layer to use
 */
void CasparFlashCommand::setHostLayer (int layer)
{
    m_templatehostlayer = layer;
}

/**
 * Adds the channel and layer to the command as a paremeter
 */
void CasparFlashCommand::addChannelAndLayers ()
{
    addParam(ConvertType::intToString(m_channelnumber));
    addParam(ConvertType::intToString(m_layernumber));
    addParam(ConvertType::intToString(m_templatehostlayer));
}

/**
 * Add a data item to be sent to the template
 *
 * @param key string The key being sent to the template
 * @param value string The value being sent to the template
 */
void CasparFlashCommand::addData (std::string key, std::string value)
{
    m_templatedata[key] = value;
}

/**
 * Clear the data list to send to the template
 */
void CasparFlashCommand::clearData ()
{
    m_templatedata.clear();
}

/**
 * Formats the template data for use by CasparCG and adds it as a parameter
 */
void CasparFlashCommand::formatAndAddTemplateData ()
{
    pugi::xml_document datadocument;
    pugi::xml_node rootnode = datadocument.append_child("document");

    for (std::pair<std::string, std::string> currentdata : m_templatedata)
    {
        pugi::xml_node componentdata = rootnode.append_child("componentData");

        pugi::xml_attribute componentid = componentdata.append_attribute("id");
        componentid.set_value(currentdata.first.c_str());

        pugi::xml_node datanode = componentdata.append_child("data");

        pugi::xml_attribute dataid = datanode.append_attribute("id");
        dataid.set_value(currentdata.first.c_str());

        pugi::xml_attribute datavalue = datanode.append_attribute("value");
        datavalue.set_value(currentdata.second.c_str());
    }

    // Pull out the generated XML as a string
    std::stringstream ss;
    rootnode.print(ss, "", pugi::format_raw);

    std::string xmldata = ss.str();

    // Replace all the quotes with \"
    int found = -2;
    found = xmldata.find("\"", found + 2);
    while (found != static_cast<int>(std::string::npos))
    {
        xmldata.insert(found, "\\");
        found = xmldata.find("\"", found + 2);
    }

    // Add as a parameter
    addParam("\\\"" + xmldata + "\\\"");
}

/**
 * Play the loaded template on this layer
 */
void CasparFlashCommand::play ()
{
    clearParams();
    setType(CASPAR_COMMAND_CG_PLAY);
    addChannelAndLayers();
}

/**
 * Load and play the specified template
 *
 * @param templatename string Name of the template to play
 */
void CasparFlashCommand::play (std::string templatename)
{
    clearParams();
    setType(CASPAR_COMMAND_CG_ADD);
    addChannelAndLayers();
    addParam(templatename);
    addParam("1");
    formatAndAddTemplateData();
}

/**
 * Load a template into memory with some data
 *
 * @param templatename string Name of the template to load
 */
void CasparFlashCommand::load (std::string templatename)
{
    clearParams();
    setType(CASPAR_COMMAND_CG_ADD);
    addChannelAndLayers();
    addParam(templatename);
    addParam("0");
    formatAndAddTemplateData();
}

/**
 * Stop the current template on this layer
 */
void CasparFlashCommand::stop ()
{
    clearParams();
    setType(CASPAR_COMMAND_CG_STOP);
    addChannelAndLayers();
}

/**
 * Advance the current template on this layer
 */
void CasparFlashCommand::next ()
{
    clearParams();
    setType(CASPAR_COMMAND_CG_NEXT);
    addChannelAndLayers();
}

/**
 * Clear this layer (actually restarts Flash ActiveX control)
 */
void CasparFlashCommand::clear ()
{
    clearParams();
    setType(CASPAR_COMMAND_CG_CLEAR);

    //Clears all template host layers, so we don't need that
    addParam(ConvertType::intToString(m_channelnumber));
    addParam(ConvertType::intToString(m_layernumber));

}

/**
 * Update the currently running template with fresh data
 */
void CasparFlashCommand::update ()
{
    clearParams();
    setType(CASPAR_COMMAND_CG_UPDATE);
    addChannelAndLayers();
    formatAndAddTemplateData();
}
