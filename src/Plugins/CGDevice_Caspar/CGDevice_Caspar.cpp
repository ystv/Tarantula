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
*   File Name   : CGDevice_Caspar.h
*   Version     : 1.0
*   Description : Caspar supports both CG and media. This plugin works with
*                 CG.
*
*****************************************************************************/

#include "CGDevice_Caspar.h"
#include "Misc.h"

/**
 * Create a CasparCG Graphics Plugin
 *
 * @param config Configuration data for this plugin
 * @param h      A link back to the GlobalStuff structures
 */
CGDevice_Caspar::CGDevice_Caspar (PluginConfig config, Hook h) :
        CGDevice(config, h)
{
    // Pull server name, port and layer from configuration data
    long int connecttimeout;

    try
    {
        m_hostname = config.m_plugindata_map.at("Host");
        m_port = config.m_plugindata_map.at("Port");
        m_layer = ConvertType::stringToInt(config.m_plugindata_map.at("Layer"));

        connecttimeout = ConvertType::stringToInt(config.m_plugindata_map.at("ConnectTimeout"));
    }
    catch (std::exception& ex)
    {
        m_hook.gs->L->error(m_pluginname, "Configuration data invalid or not supplied");
        m_status = FAILED;
        return;
    }

    h.gs->L->info("Caspar Graphics", "Connecting to " + m_hostname + ":" + m_port);

    try
    {
        m_pcaspcon = std::make_shared<CasparConnection>(m_hostname, m_port, connecttimeout);
    }
    catch (...)
    {
        m_hook.gs->L->error(m_pluginname, "Unable to connect to CasparCG server at " + m_hostname);
        m_status = FAILED;
        return;
    }

    m_status = WAITING;
}

CGDevice_Caspar::~CGDevice_Caspar ()
{
    m_pcaspcon.reset();
}

/**
 * Read and process the responseQueue
 */
void CGDevice_Caspar::poll ()
{
    if (m_status == FAILED || m_status == CRASHED)
    {
        m_hook.gs->L->warn(m_pluginname, "Not in ready state for poll()");
        return;
    }

    if (!(m_pcaspcon->tick()) || (m_pcaspcon->m_errorflag))
    {
        m_status = CRASHED;
        m_hook.gs->L->error(m_pluginname, "CasparCG connection not active");
        m_pcaspcon->m_errorflag = false;
    }

    if (m_pcaspcon->m_badcommandflag)
    {
        m_hook.gs->L->error(m_pluginname, "CasparCG command error");
        m_pcaspcon->m_badcommandflag = false;
    }


    Device::poll();
}

/**
 * Send a command to retrieve CasparCG status
 */
void CGDevice_Caspar::updateHardwareStatus ()
{
    if (WAITING == m_status)
    {
        if (m_pcaspcon->m_connectstate)
        {
            m_status = READY;
        }
    }
    else if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Plugin not in ready state for updateHardwareStatus");
        return;
    }

    CasparCommand statuscom(CASPAR_COMMAND_INFO, boost::bind(&CGDevice_Caspar::cb_info, this, _1));
    statuscom.addParam("1");
    statuscom.addParam("1");
    m_pcaspcon->sendCommand(statuscom);
}

/**
 * Add a new graphics layer to CasparCG
 *
 * @param graphicname Name of the templates
 * @param layer       Internal CG layer to add template to
 * @param pdata       Map of data to send to the template
 */
void CGDevice_Caspar::add (std::string graphicname, int layer,
        std::map<std::string, std::string> *pdata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to add layer while plugin in "
                "crashed state");
        return;
    }

    CGLayer newmap;

    newmap.graphicdata = *pdata;
    newmap.layer = layer;
    newmap.playstep = 0;
    newmap.graphicname = graphicname;

    if (m_plugindata.count(layer))
    {
        g_logger.warn("CGDevice_Demo::Add",
                "Adding item to already existing layer");
    }

    m_plugindata[layer] = newmap;

    CasparFlashCommand com(1);
    com.setLayer(m_layer);
    com.setHostLayer(layer);

    for (auto item : *pdata)
    {
        com.addData(item.first, item.second);
    }

    com.play(graphicname);
    m_pcaspcon->sendCommand(com);

}

void CGDevice_Caspar::remove (int layer)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to remove layer while plugin "
                "in crashed state");
        return;
    }

    // Remove an entry from plugindata corresponding to this layer, if it existed. Else throw
    if (!m_plugindata.count(layer))
    {
        m_hook.gs->L->warn("CGDevice_Demo::Remove",
                "Unable to remove item as the layer doesn't exist");
        return;
    }
    else
    {
        m_plugindata.erase(layer);

        CasparFlashCommand com(1);
        com.setLayer(m_layer);
        com.setHostLayer(layer);

        com.stop();
        m_pcaspcon->sendCommand(com);
    }
}

void CGDevice_Caspar::play (int layer)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to play layer while plugin in"
                " crashed state");
        return;
    }

    // Increment playstep for this layer. If layer didn't exist, throw
    if (m_plugindata.count(layer))
    {
        m_plugindata[layer].playstep++;

        CasparFlashCommand com(1);
        com.setLayer(m_layer);
        com.setHostLayer(layer);

        com.next();
        m_pcaspcon->sendCommand(com);
    }
    else
    {
        m_hook.gs->L->warn("CGDevice_Demo::Play",
                "Unable to advance layer as layer doesn't exist");
    }
}

void CGDevice_Caspar::update (int layer, std::map<std::string, std::string> *pdata)
{
    if (READY != m_status)
    {
        m_hook.gs->L->error(m_pluginname, "Attempt to update layer while plugin "
                "in crashed state");
        return;
    }

    // Replace graphicdata for this layer.
    if (m_plugindata.count(layer))
    {
        m_plugindata[layer].graphicdata = *pdata;
        m_plugindata[layer].layer = layer;

        CasparFlashCommand com(1);
        com.setLayer(m_layer);
        com.setHostLayer(layer);

        for (auto item : *pdata)
        {
            com.addData(item.first, item.second);
        }

        com.update();
        m_pcaspcon->sendCommand(com);
    }
    else
    {
        m_hook.gs->L->warn("CGDevice_Demo::Update",
                "Unable to update item as the layer doesn't exist");
    }
}


/**
 * Callback for info commands
 *
 * @param resp  Lines of data from CasparCG
 */
void CGDevice_Caspar::cb_info (std::vector<std::string>& resp)
{
    // Nothing to do!
}

/**
 * Callback to handle files update
 *
 * @param resp Vector filled with lines from CasparCG
 */
void CGDevice_Caspar::cb_updatetemplates (std::vector<std::string>& resp)
{
    //Call the processor
    CasparQueryResponseProcessor::getTemplateList(resp, m_templatelist);
}

extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //must declare as pointer to avoid object being deleted once function call is complete!
        std::shared_ptr<CGDevice_Caspar> plugtemp = std::make_shared<CGDevice_Caspar>(config, h);

        pluginref = std::dynamic_pointer_cast<Plugin>(plugtemp);
    }
}
