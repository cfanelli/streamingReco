//
//    File: HallB_FT/JEventProcessor_HallB_FT.cc
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

#include "JEventProcessor_HallB_FT.h"

#include "FT/FTCalHit.h"
#include "FT/FTHodoHit.h"
#include "FT/FTCalCluster.h"

#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
//---------------------------------
// JEventProcessor_HallB_FT    (Constructor)
//---------------------------------
JEventProcessor_HallB_FT::JEventProcessor_HallB_FT() {
	f=0;
	hClusE=0;
	hClusXY=0;
}

//---------------------------------
// ~JEventProcessor_HallB_FT    (Destructor)
//---------------------------------
JEventProcessor_HallB_FT::~JEventProcessor_HallB_FT() {

}

//------------------
// Init
//------------------
void JEventProcessor_HallB_FT::Init(void) {
	// This is called once at program startup.
	f=new TFile("out.root","recreate");
	hClusE=new TH1D("hClusE","hClusE",200,0,12);
	hClus2M=new TH1D("hClus2M","hClus2M",300,0,300);
	hClusXY=new TH2D("hClusXY","hClusXY",200,-200,200,200,-200,200);
}

//------------------
// Process
//------------------
void JEventProcessor_HallB_FT::Process(const std::shared_ptr<const JEvent>& aEvent) {
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

	auto ftCalClusters = aEvent->Get<FTCalCluster>();
	for (int ii=0;ii<ftCalClusters.size();ii++) {
		auto cluster = ftCalClusters[ii];
		hClusE->Fill(cluster->getClusterEnergy());
		hClusXY->Fill(cluster->getX(),cluster->getY());
		for (int jj=(ii+1);jj<ftCalClusters.size();jj++){
			auto cluster2 = ftCalClusters[jj];
			auto z = cos(cluster->getCentroid().Angle(cluster2->getCentroid()));
			auto M = sqrt(2*cluster->getClusterEnergy()*cluster2->getClusterEnergy()*(1-z))*1000;
			std::cout<<z<<" "<<M<<std::endl;
			hClus2M->Fill(M);
		}



	}



	/*std::cout<<"event start"<<std::endl;
	std::cout<<"CAL"<<std::endl;
	auto calhits = aEvent->Get<FTCalHit>();
	for (auto calhit : calhits){
		std::cout<<1.*calhit->m_channel.sector<<" "<<1.*calhit->m_channel.iX<<" "<<1.*calhit->m_channel.iY<<std::endl;
	}
	std::cout<<"HODO"<<std::endl;
	auto hodohits = aEvent->Get<FTHodoHit>();
	for (auto hodohit : hodohits){
		std::cout<<1.*hodohit->m_channel.sector<<" "<<1.*hodohit->m_channel.layer<<" "<<1.*hodohit->m_channel.component<<std::endl;
	}
	std::cout<<"OK"<<std::endl;*/

}

//------------------
// Finish
//------------------
void JEventProcessor_HallB_FT::Finish(void) {
	// This is called when at the end of event processing

	hClusE->Write();
	hClus2M->Write();
	hClusXY->Write();
	f->Write();
	f->Close();
}
