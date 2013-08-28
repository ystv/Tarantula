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
*   File Name   : MouseCatcherCommon.h
*   Version     : 1.0
*   Description : Common elements used by MouseCatcher
*
*****************************************************************************/


#pragma once
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "PlaylistDB.h"

class Channel;

typedef std::function<void(PlaylistEntry&, Channel*)> PreProcessorHandler;

class MouseCatcherSourcePlugin;

/**
 * Possible actions for an EventAction to perform
 */
enum EventActionTypes {
    ACTION_ADD,
    ACTION_REMOVE,
    ACTION_EDIT,
    ACTION_UPDATE_PLAYLIST,
    ACTION_UPDATE_DEVICES,
    ACTION_UPDATE_ACTIONS,
    ACTION_UPDATE_PROCESSORS,
    ACTION_UPDATE_FILES
};

/**
 * A standard event structure to insert into the Channel playlist
 */
class MouseCatcherEvent {
public:
    //! Text name for the channel as set in base config
    std::string m_channel;
    //! Can be a device name or an EventProcessor
    std::string m_targetdevice;
    playlist_event_type_t m_eventtype;
    //! Only for fixed and offset events, relative is relative to last event delivered
    long int m_triggertime;
    int m_action = -1;
    std::string m_action_name;
    int m_eventid;

    std::string m_description; //!< Description for benefit of user and use in CG events. Inherits if not set.

    //! If set to zero duration is handled separately (ie video files or crosspoints with no duration). In seconds
    int m_duration;
    std::map<std::string, std::string> m_extradata;
    std::vector<MouseCatcherEvent> m_childevents;
    std::string m_preprocessor;

};

/**
 * Entry in the event queue.
 */
struct EventAction {
    //!  Action to the playlist to be performed by this event
    EventActionTypes action;
    MouseCatcherEvent event;
    //! Playlist event ID to affect or event ID that was generated
    int eventid;
    bool isprocessed;
    std::string returnmessage;
    MouseCatcherSourcePlugin *thisplugin;

    std::shared_ptr<void> additionaldata;
};

class EventAction_check {
public:
    EventAction_check(EventAction& act) : m_action(act){};

    inline bool operator() (MouseCatcherSourcePlugin* plugin) {
        return (m_action.isprocessed) && (m_action.thisplugin == plugin);
    }

private:
    EventAction m_action;
};
