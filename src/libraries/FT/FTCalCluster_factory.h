/*
 * FTCalCluster_factory
 *
 *  Created on: Dec 23, 2019
 *      Author: celentan
 */

#ifndef SRC_LIBRARIES_FT_FTCALCluster_FACTORY_H_
#define SRC_LIBRARIES_FT_FTCALCluster_FACTORY_H_

#include "JANA/JFactoryT.h"
#include "FTCalCluster.h"

class FTCalCluster_factory: public JFactoryT<FTCalCluster> {
public:
	FTCalCluster_factory();
	virtual ~FTCalCluster_factory();

	// Overloaded by user Factories
	virtual void Init();
	virtual void ChangeRun(const std::shared_ptr<const JEvent> &aEvent);
	virtual void Process(const std::shared_ptr<const JEvent> &aEvent);
};

#endif /* SRC_LIBRARIES_FT_FTCALCluster_FACTORY_H_ */
