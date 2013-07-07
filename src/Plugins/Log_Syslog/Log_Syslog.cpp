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
*   File Name   : Log_Syslog.cpp
*   Version     : 1.0
*   Description : Logger plugin to send log data to the system log (on Linux)
*
*****************************************************************************/


#include <Log.h>
#include <TarantulaPlugin.h>
#include <string>
#include <cstring>

#include <syslog.h>

//now for the actual log class:
class Log_Syslog : Log
{
public:
    Log_Syslog (Hook h, PluginConfig config);
    virtual ~Log_Syslog ();
    virtual void info (std::string where, std::string message);
    virtual void warn (std::string where, std::string message);
    virtual void error (std::string where, std::string message);
    virtual void OMGWTF (std::string where, std::string message);
private:
    void LogMsg (std::string where, std::string message, int severity);
};

Log_Syslog::Log_Syslog (Hook h, PluginConfig config)
{
    h.gs->L->registerLogger(this);

    openlog("Tarantula", LOG_NDELAY, LOG_USER);

    if (config.m_plugindata_map.find("LogLevel") != config.m_plugindata_map.end())
    {
        std::string level = config.m_plugindata_map["LogLevel"];

        if (!level.compare("Critical"))
        {
            setlogmask(LOG_UPTO(LOG_CRIT));
        }
        else if (!level.compare("Error"))
        {
            setlogmask(LOG_UPTO(LOG_ERR));
        }
        else if (!level.compare("Warning"))
        {
            setlogmask(LOG_UPTO(LOG_WARNING));
        }
        else if (!level.compare("Info"))
        {
            setlogmask(LOG_UPTO(LOG_INFO));
        }
        else
        {
            h.gs->L->warn("Syslog Plugin",
                    "Bad LogLevel in configuration file, assuming Info.");
        }
    }
    else
    {
        h.gs->L->warn("Syslog Plugin",
                "No LogLevel in configuration file, assuming Info.");
    }

}

Log_Syslog::~Log_Syslog (void)
{
    closelog();
}

void Log_Syslog::info (std::string where, std::string message)
{
    LogMsg(where, message, LOG_INFO);
}

void Log_Syslog::warn (std::string where, std::string message)
{
    LogMsg(where, message, LOG_WARNING);
}

void Log_Syslog::error (std::string where, std::string message)
{
    LogMsg(where, message, LOG_ERR);
}

void Log_Syslog::OMGWTF (std::string where, std::string message)
{
    LogMsg(where, message, LOG_CRIT);
}

void Log_Syslog::LogMsg (std::string where, std::string message, int severity)
{
    std::string level;

    switch (severity)
    {
        case LOG_CRIT:
            level = "CRITICAL";
            break;
        case LOG_ERR:
            level = "ERROR";
            break;
        case LOG_WARNING:
            level = "WARNING";
            break;
        default:
            level = "INFO";
            break;
    }

    syslog(LOG_MAKEPRI(LOG_USER, severity), "%s:: %s:: %s", level.c_str(),
            where.c_str(), message.c_str());
}

//now for the bit to bind it as a plugin
extern "C"
{
    void LoadPlugin (Hook h, PluginConfig config, std::shared_ptr<Plugin>& pluginref)
    {
        //Log plugins are handled seperately and have non-standard plugin load functions
        Log_Syslog *log_syslog = new Log_Syslog(h, config);
    }
}
