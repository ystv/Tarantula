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
*   File Name   : Test_Crosspoint.cpp
*   Version     : 1.0
*****************************************************************************/
#include "Channel.h"
#include "CrosspointDevice.h"
#include <sstream>
#include <string>
#include <map>

class TestCP: public CrosspointDevice {
    public:
    
    TestCP(std::string name,Hook h) : CrosspointDevice(name,h) {
        
    }
    
    void switchOP(unsigned int output,unsigned int input) {
        IOMap[output]  = input;
        poll();
    }
    
    void poll() {
        //don't need to do much for this test.
    }
    
    //expose a protected method for the test
    void IOCount(unsigned int i,unsigned int o) {
        setIOCount(i,o);
    }
    
    //expose a protected variable for testing
    void SetIPName(unsigned int i,std::string name) {
        IPnames[i] = name;
    }
    void SetOPName(unsigned int i,std::string name) {
        OPnames[i] = name;
    }
};

int runtest();
extern char testname[];

Log g_logger;
std::vector<cbBegunPlaying> g_begunplayingcallbacks;
std::vector<cbEndPlaying> g_endplayingcallbacks;
std::vector<cbTick> g_tickcallbacks;
std::vector<Channel*> g_channels;
std::map<std::string,Device*> g_devices;

char testname[] = "Crosspoint Base Class Test";

GlobalStuff* NewGS() {
    GlobalStuff* gs = new GlobalStuff();
    gs->L = &g_logger;
    gs->Channels = &g_channels;
    gs->BegunPlayingCallbacks = &g_begunplayingcallbacks;
    gs->EndPlayingCallbacks = &g_endplayingcallbacks;
    gs->TickCallbacks = &g_tickcallbacks;
    gs->Devices = new std::map<std::string,Device*>;
    return gs;
}

int runtest() {
    GlobalStuff *gs = NewGS();
    Hook h;
    h.gs = gs;
    TestCP tcp("Test CP",h);
    //first check we can set the size
    tcp.IOCount(16,32);
    if(tcp.getipCount() != 16) {
        std::cout << "Error: IP count does not match what was set." << std::endl;
        return 1;
    }
    if(tcp.getopCount() != 32) {
        std::cout << "Error: OP count (" << tcp.getipCount() << ") does not match what was set (" << 32 << ")." << std::endl;
        return 1;
    }
    //now we check we can name the inputs:
    for(int i=0;i<16;i++) {
        std::string IPName;
        std::stringstream nss;
        nss << "Input " << i << "456789uihgcftry" << i*3;//random crap on the end to make sure this is what we are setting
        nss >> IPName;
        //now we set the IP name
        tcp.SetIPName(i,IPName);
        //now we check it
        if(tcp.getipName(i) != IPName) {
            std::cout << "Error: Could not set IP name correctly" << std::endl;
            return 1;
        }
    }
    //now we check the output names in the same way
    for(int i=0;i<32;i++) {
        std::string OPName;
        std::stringstream nss;
        nss << "Output " << i << "$%^&bhgyu" << i*3;//random crap on the end to make sure this is what we are setting
        nss >> OPName;
        //now we set the IP name
        tcp.SetOPName(i,OPName);
        //now we check it
        if(tcp.getopName(i) != OPName) {
            std::cout << "Error: Could not set OP name correctly" << std::endl;
            return 1;
        }
    }
    //now we try switching
    tcp.switchOP(12,4);
    if(tcp.getOPSource(12) != 4) {
        std::cout << "Error: Could not switch output 12 to input 4." << std::endl;
        return 1;
    }
    return 0; //it works!
}
