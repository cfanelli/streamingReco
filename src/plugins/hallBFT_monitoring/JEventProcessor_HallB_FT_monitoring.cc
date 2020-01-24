//
//    File: HallB_FT_monitoring/JEventProcessor_HallB_FT_monitoring.cc
// Created: Thu Jan  9 19:37:42 CET 2020
// Creator: celentan (on Linux apcx4 3.10.0-957.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "JEventProcessor_HallB_FT_monitoring.h"

#include "FT/FTCalHit.h"
#include "FT/FTHodoHit.h"

#include <TDirectory.h>
#include <TH2.h>
#include <TH1.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include "TROOT.h"

#include "DAQ/faWaveboardHit.h"

/*Here goes the histograms*/

static TH1D *hTest = 0;

//---------------------------------
// JEventProcessor_HallB_FT_monitoring    (Constructor)
//---------------------------------
JEventProcessor_HallB_FT_monitoring::JEventProcessor_HallB_FT_monitoring() {

}

//---------------------------------
// ~JEventProcessor_HallB_FT_monitoring    (Destructor)
//---------------------------------
JEventProcessor_HallB_FT_monitoring::~JEventProcessor_HallB_FT_monitoring() {

}

//------------------
// Init
//------------------
void JEventProcessor_HallB_FT_monitoring::Init(void) {
	// This is called once at program startup.

	//japp->RootWriteLock();
	if (hTest != NULL) {
		//japp->RootUnLock();
		return;
	}
	gROOT->cd();
	TDirectory *main = gDirectory;
	gDirectory->mkdir("HallB_FT_monitoring")->cd();

	hTest = new TH1D("hTest", "hTest", 100, 0, 10);
	//japp->RootUnLock();

}

//------------------
// Process
//------------------
void JEventProcessor_HallB_FT_monitoring::Process(const std::shared_ptr<const JEvent>& aEvent) {
	// This is called for every event. Use of common resources like writing
	// to a file or filling a histogram should be mutex protected. Using
	// aEvent->Get<type>() to get reconstructed objects (and thereby activating the
	// reconstruction algorithm) should be done outside of any mutex lock
	// since multiple threads may call this method at the same time.
	// Here's an example:
	//
	// auto myTracks = aEvent->Get*<MyTrack>();
	//
	// lock_guard<mutex> lck( mymutex );
	// for( auto t : myTracks ){
	//  ... fill histograms or trees ...
	// }



	auto calhits=aEvent->Get<FTCalHit>();
	auto hits = aEvent->Get<faWaveboardHit>();
	for (auto hit : hits) {
		std::cout << 1. * hit->m_channel.crate << " " << 1. * hit->m_channel.slot << " " << 1. * hit->m_channel.channel << std::endl;
	}

	//lock
	hTest->Fill(hits.size());
	hTest->Fill(calhits.size());

	//unlock

}

//------------------
// Finish
//------------------
void JEventProcessor_HallB_FT_monitoring::Finish(void) {
	// This is called when at the end of event processing
}