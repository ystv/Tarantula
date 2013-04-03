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
*   File Name   : Test_LogTest.cpp
*   Version     : 1.0
*****************************************************************************/
//Test_LogTest - tests the logging module using the Test_Log dummy

#include "Test_Log.h"

int runtest();
extern char testname[];

char testname[] = "LogTest";

//copied from main Tarrantula:
Log g_logger;
std::vector<cbBegunPlaying> g_begunplayingcallbacks;
std::vector<cbEndPlaying> g_endplayingcallbacks;
std::vector<cbTick> g_tickcallbacks;
std::vector<Channel*> g_channels;

GlobalStuff* NewGS() {
    GlobalStuff* gs = new GlobalStuff();
    gs->L = &g_logger;
    gs->Channels = &g_channels;
    gs->BegunPlayingCallbacks = &g_begunplayingcallbacks;
    gs->EndPlayingCallbacks = &g_endplayingcallbacks;
    gs->TickCallbacks = &g_tickcallbacks;
    return gs;
}
//end copying.

int runtest() {
    GlobalStuff *gs = NewGS();
    Hook h;
    h.gs = gs;
    Log_Test tl(h);
    #ifdef Test_Info
    h.gs->L->Info("Test","Test");
    return !(tl.info) && tl.warn && tl.error && tl.omgwtf;
    #endif
    #ifdef Test_Warn
    h.gs->L->Warn("Test","Test");
    return !(tl.warn) && tl.error && tl.omgwtf && tl.info;
    #endif
    #ifdef Test_Error
    h.gs->L->Error("Test","Test");
    return !(tl.error) && tl.omgwtf && tl.warn && tl.info;
    #endif
    #ifdef Test_OMGWTF
    h.gs->L->OMGWTF("Test","Test");
    return !(tl.omgwtf) && tl.error && tl.warn && tl.info;
    #endif
}
