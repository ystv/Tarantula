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
*   File Name   : CallBackTools.cpp
*   Version     : 1.0
*   Description : CallBackTools are the functions to fire various callbacks
*                 when they happen
*
*****************************************************************************/


#include "TarantulaCore.h"

/**
 * Fairly self-explanatory this one, it calls everything registered for a tick
 */
void tick() {
    //loop over the tick callback items, running each one.
    for(std::vector<cbTick>::iterator it = g_tickcallbacks.begin();it!=g_tickcallbacks.end();it++) {
        (*it)();
    }
}

/**
 * Runs all the callbacks when a device has started playing. Individual callback functions
 * are expected to test for the name and id they're interested in
 *
 * @param name The name of the device firing the callback
 * @param int  The event id the callback marks the start of
 */
void begunPlaying(std::string name, int id) {
    // Loop over the callback items, running each one.
    for(std::vector<cbBegunPlaying>::iterator it = g_begunplayingcallbacks.begin();
            it!=g_begunplayingcallbacks.end(); it++) {
        (*it)(name,id);
    }
}

/**
 * Runs all the callbacks when a device finishes playing. Individual callback functions
 * are expected to test for the name and id they're interested in
 *
 * @param name The name of the device firing the callback
 * @param int  The event id the callback marks the end of
 */
void endPlaying(std::string name, int id) {
    // Loop over the callback items, running each one.
    for(std::vector<cbEndPlaying>::iterator it = g_endplayingcallbacks.begin();
            it!=g_endplayingcallbacks.end(); it++) {
        (*it)(name,id);
    }
}

