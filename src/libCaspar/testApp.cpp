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
*   File Name   : testApp.cpp
*   Description : Tests libCaspar
*   Version     : 1.0
*****************************************************************************/

#define CASPARHOST "127.0.0.1"

#include <libCaspar/libCaspar.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main(int argc,char *argv[]) {
    cout << "libCasparTestApp - Don't expect this to test everything - I'm too lasy for that!" <<endl;
    
    cout << endl << endl << "Now testing CasparCommand." <<endl;
    CasparCommand cc0(CASPAR_COMMAND_LOADBG);
    cc0.addParam("1");
    cc0.addParam("1");
    cc0.addParam("AMB");
    cout << "Got: " << cc0.form();

    cout << "Now testing CasparVideoCommand::Play." <<endl;
    CasparVideoCommand cc1 = CasparVideoCommand(1);
    cc1.setTransition(CASPAR_TRANSITION_WIPE, 20);
    cc1.play("AMB");
    cout << "Got: " << cc1.form();

    cout << "Now testing CasparVideoCommand::Load." <<endl;
    cc1 = CasparVideoCommand(1);
    cc1.setTransition(CASPAR_TRANSITION_MIX, 5);
    cc1.load("TEST",1);
    cout << "Got: " << cc1.form();

    cout << "Now testing CasparVideoCommand::Play with no filename." <<endl;
    cc1 = CasparVideoCommand(1);
    cc1.setTransition(CASPAR_TRANSITION_WIPE, 20);
    cc1.play();
    cout << "Got: " << cc1.form();

    cout << "Now testing CasparVideoCommand::Stop." <<endl;
    cc1 = CasparVideoCommand(1);
    cc1.setLayer(4);
    cc1.stop();
    cout << "Got: " << cc1.form();

    cout << "Now testing CasparVideoCommand::Clear." <<endl;
    cc1 = CasparVideoCommand(3);
    cc1.clear();
    cout << "Got: " << cc1.form();

    cout  << "Now testing CasparVideoCommand::Seek." <<endl;
    cc1 = CasparVideoCommand(1);
    cc1.seek(400);
    cout << "Got: " << cc1.form();

    cout  << "Now testing CasparFlashCommand::Play." <<endl;
    CasparFlashCommand cc2 = CasparFlashCommand(1);
    cc2.addData("f0","test");
    cc2.play("test");
    cout << "Got: " << cc2.form();

    cout  << "Now testing CasparFlashCommand::Load." <<endl;
    cc2 = CasparFlashCommand(1);
    cc2.setLayer(9);
    cc2.load("test");
    cout << "Got: " << cc2.form();

    cout  << "Now testing CasparFlashCommand::Stop." <<endl;
    cc2 = CasparFlashCommand(1);
    cc2.setHostLayer(4);
    cc2.stop();
    cout << "Got: " << cc2.form();

    cout  << "Now testing CasparFlashCommand::Clear." <<endl;
    cc2 = CasparFlashCommand(1);
    cc2.clear();
    cout << "Got: " << cc2.form();

    cout  << "Now testing CasparFlashCommand::Next." <<endl;
    cc2 = CasparFlashCommand(1);
    cc2.next();
    cout << "Got: " << cc2.form();

    cout  << "Now testing CasparFlashCommand::Update." <<endl;
    cc2 = CasparFlashCommand(1);
    cc2.addData("g5","More test text ?!&");
    cc2.update();
    cout << "Got: " << cc2.form();

    cout << endl << "Now testing CasparConnection." <<endl;
    CasparConnection caspCon(CASPARHOST);
    std::vector<std::string> resp;
    caspCon.sendCommand(CasparCommand(CASPAR_COMMAND_CLS), resp);
    cout << "Got " << resp.size() << " responses." << endl;

    CasparCommand ccclr(CASPAR_COMMAND_CLEAR_PRODUCER);
    ccclr.addParam("1");
    caspCon.sendCommand(ccclr, resp);


    srand(static_cast<unsigned>(time(0)));
    int vid = rand() % (resp.size()-2);
    cout << "Chosen vid is no. " << vid <<endl;
    std::string line = resp[vid+2];

    line = line.substr(0,line.find(' ')-1);
    line.erase(0,1);
    CasparVideoCommand ccf1(1);
    ccf1.setLayer(1);
    ccf1.setTransition(CASPAR_TRANSITION_CUT,0);
    ccf1.load(line,false);
    resp.clear();

    caspCon.sendCommand(ccf1,resp);
    CasparVideoCommand ccf2(1);
    ccf2.play();
    resp.clear();

    caspCon.sendCommand(ccf2,resp);
    resp.clear();


    cout << endl << "Now testing CasparQueryResponseProcessor::getMediaList." <<endl;
    CasparCommand query(CASPAR_COMMAND_CLS);
    resp.clear();

    caspCon.sendCommand(query, resp);
    std::vector<std::string> medianames;
    std::map<std::string, int> medialist;

    //Call the processor
    CasparQueryResponseProcessor::getMediaList(resp, medianames);

    //Iterate over media list getting lengths and adding to files list
	for (std::string item : medianames)
	{
		// To grab duration from CasparCG we have to load the file and pull info
		CasparCommand durationquery(CASPAR_COMMAND_LOADBG);
		durationquery.addParam("1");
		durationquery.addParam("5");
		durationquery.addParam(item);
		caspCon.sendCommand(durationquery, resp);

		CasparCommand infoquery(CASPAR_COMMAND_INFO);
		infoquery.addParam("1");
		infoquery.addParam("5");
		resp.clear();
		caspCon.sendCommand(infoquery, resp);

		medialist[item] = CasparQueryResponseProcessor::readFileFrames(resp);
	}

    cout << endl << "Got a list of " << medialist.size() << " items. " << line <<
            " shows as having " << medialist[line] << " frames."<<endl;


    cout << endl << "Now testing CasparQueryResponseProcessor::getTemplateList." <<endl;
    query = CasparCommand(CASPAR_COMMAND_TLS);

    resp.clear();
    caspCon.sendCommand(query,resp);
    std::vector<std::string> templatelist;

    //Call the processor
    CasparQueryResponseProcessor::getTemplateList(resp, templatelist);
    int item = rand() % (templatelist.size());
    cout << endl << "Got " << templatelist.size() << " items. One item is "
            << templatelist[item];
    //Fixes an irritating bug where the transition-producer runs first
    usleep(200);


    cout << endl << "Now testing CasparQueryResponseProcessor::readLayerStatus." <<endl;
    CasparCommand statuscom(CASPAR_COMMAND_INFO_EXPANDED);
    statuscom.addParam("1");

    resp.clear();
    caspCon.sendCommand(statuscom,resp);
    std::string filename;
    int frames = CasparQueryResponseProcessor::readLayerStatus(resp, filename);

    cout << endl << "Got response filename " << filename << " and " <<
            frames << " frames left";

    cout << endl << "Now testing CasparFlashCommand. " << endl;
    CasparFlashCommand flashcom(1);
    flashcom.setLayer(2);
    flashcom.play("Phone");

    resp.clear();
    caspCon.sendCommand(flashcom, resp);

    cout << endl << "Testing complete." << endl;

    return 0;
}
