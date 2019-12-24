/*
 * faWaveboardHitfactory.cc
 *
 *  Created on: Dec 23, 2019
 *      Author: celentan
 */

#include "faWaveboardHit_factory.h"
#include "TridasEvent.h"

#include "JANA/JEvent.h"

faWaveboardHit_factory::faWaveboardHit_factory() {
	// TODO Auto-generated constructor stub

}

faWaveboardHit_factory::~faWaveboardHit_factory() {
	// TODO Auto-generated destructor stub
}

void faWaveboardHit_factory::Init() {

}
void faWaveboardHit_factory::ChangeRun(const std::shared_ptr<const JEvent> &aEvent) {

}
void faWaveboardHit_factory::Process(const std::shared_ptr<const JEvent> &aEvent) {
	auto tridas_event = aEvent->GetSingle<TridasEvent>();

	//loop over the hits in the tridas event
	//For now this is done in a not-so-elegant, but I'd prefer to fasten this.
	//The reason is that, using an intermediate structure to pass data from TriDAS to JANA,
	//I need to copy two times

	for (auto hit : tridas_event->hits){
		// Create a faWaveboardHit object;
		auto faHit = new faWaveboardHit;

		faHit->charge = hit.charge;
		faHit->m_time = hit.time;

		faHit->m_channel.crate = hit.crate;
		faHit->m_channel.slot = hit.slot;
		faHit->m_channel.channel = hit.channel;

		//Assign the samples
		faHit->samples = hit.data;

		//Assign the dT
		faHit->m_dT = 4; //Todo: read from dB

		//Add to the framework
		mData.push_back(faHit);
	}
}

