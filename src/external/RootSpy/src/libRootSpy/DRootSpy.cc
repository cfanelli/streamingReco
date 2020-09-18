// $Id$
//
//    File: DRootSpy.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <thread>
#include <memory>
#include <atomic>
using namespace std;

#include <TROOT.h>
#include <TDirectoryFile.h>
#include <TClass.h>
#include <TH1.h>
#include <TMessage.h>
#include <pthread.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TFile.h>
#include <TMemFile.h>

#include "DRootSpy.h"
#include "rs_udpmessage.h"

double DRootSpy::start_time = 0.0; // overwritten on fist call to rs_cmsg::GetTime()

#ifndef _DBG_
#define _DBG_ cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl;
#endif
void *WatchConnectionC(void *ptr);
void* DebugSamplerC(void *ptr);
void *ReturnFinalsC(void * ptr);

//...................................................
// This is used to temporarily store simple macro
// definitions until a DRootSpy object is created
typedef struct {
	string name;
	string path;
	string macro_data;
} RS_QUEUED_MACRO_t;
//...................................................

// --------- GLOBALS ----------------
string gROOTSPY_PROGRAM_NAME = "rootspy-server";
DRootSpy *gROOTSPY = NULL;
//sem_t RootSpy_final_sem;
atomic<bool> process_finals(false);
pthread_rwlock_t *gROOTSPY_RW_LOCK = NULL;
vector<RS_QUEUED_MACRO_t> QUEUED_MACROS;

//...................................................
// REGISTER_ROOTSPY_MACRO
//
// The following allows us to optionally register macros iff the symbol
// REGISTER_ROOTSPY_MACRO can be found by dlsym. This way, plugins with
// macros can be used with or without the rootspy plugin attached. The
// user's build system (SBMS for the online packages in GlueX) can be
// made to automatically copy scripts into strings that are compiled
// along with a call to REGISTER_ROOTSPY_MACRO so that the whole system
// is automatic.
extern "C" {
void REGISTER_ROOTSPY_MACRO(string name, string path, string macro_data) {
	// If a DRootSpy object already exists, register the macro right away
	// otherwise, just copy it to a global container to be regsitered later
	if (gROOTSPY) {
		gROOTSPY->RegisterMacro(name, path, macro_data);
	} else {
		RS_QUEUED_MACRO_t rsqm = { name, path, macro_data };
		QUEUED_MACROS.push_back(rsqm);
	}
}
}
;
//...................................................

//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
//TODO: documentation comment.
DRootSpy::DRootSpy(string udl) {
	Initialize(NULL, udl);
}

//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
//TODO: documentation comment.
DRootSpy::DRootSpy(pthread_rwlock_t *rw_lock, string udl) {
	cout << "DRootSpy::DRootSpy creator2 called" << endl;
	Initialize(rw_lock, udl);
}

//---------------------------------
// Initialize
//
/// This is called from the constructors and should not be
/// called from anywhere else.
//---------------------------------
void DRootSpy::Initialize(pthread_rwlock_t *rw_lock, string myUDL) {

	// These are used help profile the program
	// when the ROOTSPY_DEBUG environment variable
	// is set.
	in_callback = false;
	in_list_hists = false;
	in_list_macros = false;
	in_get_hist = false;
	in_get_hists = false;
	in_get_tree = false;
	in_get_tree_info = false;
	in_get_macro = false;

	// Initialize the gROOTSPY_RW_LOCK global either with the user
	// supplied rw_lock or by allocating our own.
	if (rw_lock) {
		// User provided rw_lock for locking ROOT global
		gROOTSPY_RW_LOCK = rw_lock;
		own_gROOTSPY_RW_LOCK = false;
	} else {
		// Create our own rw_lock. Make it shared.
		pthread_rwlockattr_t attr;
		pthread_rwlockattr_init(&attr);
		pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		gROOTSPY_RW_LOCK = new pthread_rwlock_t;
		pthread_rwlock_init(gROOTSPY_RW_LOCK, &attr);
		own_gROOTSPY_RW_LOCK = true;
	}

	// Initialize member data
	pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
	hist_dir = gDirectory;
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "rs_%s_%d", hostname, getpid());
	myname = string(str);

	// Determine UDL for connecting to cMsg server
	// The preference is given is:
	//  1. udl passed to us (if not set to "<default">)
	//  2. environment variable ROOTSPY_UDL if set
	//  3. hardwired UDL (cMsg://localhost/cMsg/rootspy)
	//
	// If "<default>" is specified, then check for 2 then 3
	if (myUDL == "<default>") {
		const char *ROOTSPY_UDL = getenv("ROOTSPY_UDL");
		if (ROOTSPY_UDL) {
			myUDL = ROOTSPY_UDL;
		} else {
			myUDL = "cMsg://127.0.0.1/cMsg/rootspy";
		}
	}

	// Copy to member
	myudl = myUDL;

	// Make connection to cMsg system and subscribe to messages
	cMsgSys = NULL;
	done = false;
	ConnectToCMSG();

	// Launch thread to watch connection and re-establish if it
	// goes away.
	pthread_create(&mywatcherthread, NULL, WatchConnectionC, this);

	// semaphore for handling sending final histograms
	//int err = sem_init(&RootSpy_final_sem, 0, 1);

	finalhists = NULL;

	//create a thread to simulate what the environment of using the
	//finalHist msg and vector would be like. Note that if an error
	// occurs in the sem_init call above, then we should NOT launch
	// the thread here because the semaphore will be unavailable.
	// This is always the case for Mac OS X which does not support
	// unnamed semaphores, but does define the sem_init function
	// such that it always returns -1.
//	if(err == 0){
	pthread_create(&mythread, NULL, ReturnFinalsC, this);
//	}else{
//		cout << "Unable to create semephore. Final Histograms feature disabled." << endl;
//	}

	// Register any macros that were queued up before the DRootSpy object was created
	for (unsigned int i = 0; i < QUEUED_MACROS.size(); i++) {
		RS_QUEUED_MACRO_t &rsqm = QUEUED_MACROS[i];
		RegisterMacro(rsqm.name, rsqm.path, rsqm.macro_data);
	}
	QUEUED_MACROS.clear();

	VERBOSE = 1;
	const char *ROOTSPY_VERBOSE = getenv("ROOTSPY_VERBOSE");
	if (ROOTSPY_VERBOSE) VERBOSE = atoi(ROOTSPY_VERBOSE);
	const char *ROOTSPY_DEBUG = getenv("ROOTSPY_DEBUG");
	DEBUG = ROOTSPY_DEBUG != NULL;

	debug_file = NULL;
	hcounts = NULL;
	hfractions = NULL;

	if (DEBUG) {
		string fname = strlen(ROOTSPY_DEBUG) > 0 ? ROOTSPY_DEBUG : "rs_stats.root";
		cout << "ROOTSPY_DEBUG is set. Will write stats to \"" << fname << "\"" << endl;

		pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
		debug_file = new TFile(fname.c_str(), "RECREATE");

		hcounts = new TH1I("counts", "Sampling counts", kNCOUNTERS, 0, kNCOUNTERS);
		TAxis *xaxis = hcounts->GetXaxis();
		xaxis->SetBinLabel(1 + kNSAMPLES, "Num. Samples");
		xaxis->SetBinLabel(1 + kREADLOCKED, "read locked");
		xaxis->SetBinLabel(1 + kWRITELOCKED, "write locked");
		xaxis->SetBinLabel(1 + kINCALLBACK, "in callback");
		xaxis->SetBinLabel(1 + kIN_LIST_HISTS, "in list hists");
		xaxis->SetBinLabel(1 + kIN_LIST_MACROS, "in list macros");
		xaxis->SetBinLabel(1 + kIN_GET_HIST, "in get hist");
		xaxis->SetBinLabel(1 + kIN_GET_HISTS, "in get hists");
		xaxis->SetBinLabel(1 + kIN_GET_TREE, "in get tree");
		xaxis->SetBinLabel(1 + kIN_GET_TREE_INFO, "in get tree info");
		xaxis->SetBinLabel(1 + kIN_GET_MACRO, "in get macro");

		hfractions = new TH1D("fractions", "Sampling fractions", kNCOUNTERS, 0, kNCOUNTERS);
		for (int ibin = 1; ibin <= kNCOUNTERS; ibin++) {
			hfractions->GetXaxis()->SetBinLabel(ibin, hcounts->GetXaxis()->GetBinLabel(ibin));
		}

		hist_dir->cd();

		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

		// Launch thread to record samples for debugging
		pthread_create(&mydebugthread, NULL, DebugSamplerC, this);
	}

	gROOTSPY = this;
}

//---------------------------------
// ~DRootSpy    (Destructor)
//---------------------------------
DRootSpy::~DRootSpy() {
	// Tell watcher thread to stop
	void *retval;
	done = true;
	pthread_join(mywatcherthread, &retval);
	if (DEBUG) pthread_join(mydebugthread, &retval);

	// Unsubscribe
	for (unsigned int i = 0; i < subscription_handles.size(); i++) {
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();
	cMsgSys->disconnect();

//	sem_destroy(&RootSpy_final_sem);

	// If writing DEBUG stats, then finish that off
	if (debug_file) {

		cout << "--- Closing ROOTSPY_DEBUG root file ----" << endl;

		pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);
		TDirectory *savedir = gDirectory;

		double nsamples = (double) hcounts->GetBinContent(1);
		if (nsamples > 0) {
			for (int ibin = 1; ibin < kNCOUNTERS; ibin++) {
				double val = (double) hcounts->GetBinContent(ibin);
				hfractions->SetBinContent(ibin, val / nsamples);
			}
		}

		debug_file->Write();
		debug_file->Close();
		delete debug_file;
		debug_file = NULL;

		savedir->cd();
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
	}

	if (own_gROOTSPY_RW_LOCK && gROOTSPY_RW_LOCK != NULL) {
		delete gROOTSPY_RW_LOCK;
		gROOTSPY_RW_LOCK = NULL;
	}
}

//---------------------------------
// ConnectToCMSG
//---------------------------------
void DRootSpy::ConnectToCMSG(void) {
	// Connect to cMsg system
	string myName = myname;
	string myDescr = "Access ROOT objects in JANA program";
	cMsgSys = new cMsg(myudl, myname, myDescr);      // the cMsg system object, where
	try {                                    //  all args are of type string
		cMsgSys->connect();
	} catch (cMsgException e) {
		cout << endl << endl << endl << endl << "_______________  ROOTSPY unable to connect to cMsg system! __________________" << endl;
		cout << e.toString() << endl;
		cout << endl << endl << endl << endl;
		return;
	}

	cout << "---------------------------------------------------" << endl;
	cout << "rootspy name: \"" << myname << "\"" << endl;
	cout << "---------------------------------------------------" << endl;

	// Subscribe to generic rootspy info requests
	subscription_handles.push_back(cMsgSys->subscribe("rootspy", "*", this, NULL));

	// Subscribe to rootspy requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));

	// Start cMsg system
	cMsgSys->start();

	// Broadcast that we're here to anyone already listening
	cMsgMessage ImAlive;
	ImAlive.setSubject("rootspy");
	ImAlive.setType(myname);
	cMsgSys->send(&ImAlive);
}

//---------------------------------
// WatchConnectionC
//---------------------------------
void* WatchConnectionC(void *ptr) {
	// C-style wrapper for WatchConnection
	// method so it can be run in a separate
	// thread.
	DRootSpy *rootspy = (DRootSpy*) ptr;
	return rootspy->WatchConnection();
}

//---------------------------------
// WatchConnection
//---------------------------------
void* DRootSpy::WatchConnection(void) {
	/// This is called in its own thread. It mostly sleeps
	/// but wakes up once in a while to check that we are
	/// still connected. If not, it will try and delete the 
	/// existing cMsg object and then call ConnectToCMSG()
	/// to create a new one, reconnecting to the server.

	while (!done) {
		usleep(1000);
		if (cMsgSys) {
			if (!cMsgSys->isConnected()) {
				delete cMsgSys;
				cMsgSys = NULL;
			}
		} else {
			ConnectToCMSG();
		}
	}

	return NULL;
}

//---------------------------------
// DebugSamplerC
//---------------------------------
void* DebugSamplerC(void *ptr) {
	// C-style wrapper for DebugSampler
	// method so it can be run in a separate
	// thread.
	DRootSpy *rootspy = (DRootSpy*) ptr;
	return rootspy->DebugSampler();
}

//---------------------------------
// DebugSampler
//---------------------------------
void* DRootSpy::DebugSampler(void) {
	/// This method runs in a dedicated thread, but
	/// is only launched if the ROOTSPY_DEBUG envar
	/// is set. (If this is set to something other
	/// than an empty string then that is used as
	/// the name of the output ROOT file.) This thread
	/// will mostly sleep, but occasionally wake
	/// up to record the state of the system in 
	/// output histogram.

	// Note that this is the only thread that should
	// fill these histograms. Thus, we shouldn't need
	// to get the ROOT lock.

	while (!done) {

		hcounts->Fill(kNSAMPLES);

		if (!pthread_rwlock_tryrdlock(gROOTSPY_RW_LOCK)) {
			pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		} else {
			hcounts->Fill(kREADLOCKED);
		}
		if (!pthread_rwlock_trywrlock(gROOTSPY_RW_LOCK)) {
			pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		} else {
			hcounts->Fill(kWRITELOCKED);
		}

		if (in_callback) hcounts->Fill(kINCALLBACK);
		if (in_list_hists) hcounts->Fill(kIN_LIST_HISTS);
		if (in_list_macros) hcounts->Fill(kIN_LIST_MACROS);
		if (in_get_hist) hcounts->Fill(kIN_GET_HIST);
		if (in_get_hists) hcounts->Fill(kIN_GET_HISTS);
		if (in_get_tree) hcounts->Fill(kIN_GET_TREE);
		if (in_get_tree_info) hcounts->Fill(kIN_GET_TREE_INFO);
		if (in_get_macro) hcounts->Fill(kIN_GET_MACRO);

		usleep(20000); // sample at ~50Hz
	}

	return NULL;
}

void JUNK(cMsgMessage &response, string hnamepath, uint32_t addr32, uint16_t port) {
}

//---------------------------------
// callback
//---------------------------------
//TODO: documentation comment.
void DRootSpy::callback(cMsgMessage *msg, void *userObject) {

	if (!msg) return;

	in_callback = true;

	if (VERBOSE > 1) cout << "Received message --  Subject:" << msg->getSubject() << " Type:" << msg->getType() << " Text:" << msg->getText() << endl;

	// We want all callback threads to be run with high priority
	// relative to the other (JANA) threads. This is so they can
	// be responsive to the remote process. This thread is created
	// by cMsg however so the only way we have of bumping its priority
	// is to do so from the thread itself.
	struct sched_param sp = { 16 }; // thread priority
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();

	// The actual command is always sent in the text of the message
	string cmd = msg->getText();

	// Prepare to send a response (this may or may not
	//be needed below, depending on the command)
	cMsgMessage *response;
	if (msg->isGetRequest()) {
		if (VERBOSE > 1) _DBG_ << "responding to direct message..." << endl;
		response = msg->response();
	} else {
		response = new cMsgMessage();
	}
	response->setSubject(sender);
	response->setType(myname);

	double now = GetTime(); // current time in s (arbitrary zero)

	// Copy timestamps to response
	uint64_t treceived = (uint64_t) time(NULL);
	uint64_t trequester = 0;
	try {
		trequester = msg->getUint64("time_sent");
	} catch (...) {
	}
	response->add("time_requester", trequester);
	response->add("time_received", treceived);

	// Check message queue to make sure it doesn't grow too large
	vector<int32_t> queue_counts;
	for (uint32_t i = 0; i < subscription_handles.size(); i++) {
		void *handle = subscription_handles[i];
		int32_t queue_count = cMsgSys->subscriptionQueueCount(handle);
		if (queue_count >= 32) {
			static int Nwarnings = 0;
			if (Nwarnings < 20) {
				_DBG_ << "cMsg queue " << i << " has >32 messages. Clearing ..." << endl;
				if (++Nwarnings == 20) _DBG_ << "  (last warning!" << endl;
			}
			cMsgSys->subscriptionQueueClear(handle);
		}
		//queue_counts.push_back(cMsgSys->subscriptionQueueCount(subscription_handles[i]));
	}
	if (!queue_counts.empty()) response->add("queue_counts", queue_counts);

	// Dispatch command 
	if (cmd == "who's there?") {
		response->setText("I am here");
		response->add("program", gROOTSPY_PROGRAM_NAME);
		if (VERBOSE > 1) _DBG_ << "responding to \"who's there\"..." << endl;
		uint64_t tsending = (uint64_t) time(NULL);
		response->add("time_sent", tsending);
		cMsgSys->send(response);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
		//delete msg;
		//return;
	} else if (cmd == "list hists") {
		if (VERBOSE > 1) _DBG_ << "responding to \"list hists\"..." << endl;
		listHists(*response);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
	} else if (cmd == "get hist") {
		// Get name of requested histogram
		string hnamepath = msg->getString("hnamepath");
		double delta_t = now - last_sent[sender][hnamepath];
		if (delta_t > 1.0) {
			last_sent[sender][hnamepath] = now; // record time
			try {
				uint32_t addr = msg->getUint32("udpaddr");
				uint16_t port = msg->getUint16("udpport");
				if (VERBOSE > 1) {
					struct in_addr sin_addr;
					sin_addr.s_addr = ntohl(addr);
					string ip_dotted = inet_ntoa(sin_addr);
					_DBG_ << "request has UDP info: " << ip_dotted << " : " << port << endl;
				}
				if (VERBOSE > 1) _DBG_ << "responding via UDP to \"get hist\" for " << hnamepath << " ..." << endl;
//				thread t(&DRootSpy::getHistUDP, this, *response, hnamepath, addr, port);
				thread t(&DRootSpy::getHistUDP, this, (void*) response, hnamepath, addr, port);
				t.detach();
				response = NULL; // thread owns response now
			} catch (...) {
				if (VERBOSE > 1) _DBG_ << "responding via cMsg to \"get hist\" for " << hnamepath << " ..." << endl;
				getHist(*response, hnamepath);
			}
			if (VERBOSE > 3) _DBG_ << "...done" << endl;
		} else {
			if (VERBOSE > 3) _DBG_ << "ignoring request for " << hnamepath << " (recently sent)" << endl;
		}
	} else if (cmd == "get hists") {
		// Get name of requested histograms
		vector<string> *hnamepaths = msg->getStringVector("hnamepaths");
		if (VERBOSE > 1) _DBG_ << "responding to \"get hists\" for " << hnamepaths->size() << " hnamepaths ..." << endl;
		getHists(*response, *hnamepaths);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
	} else if (cmd == "get tree") {
		// Get name of requested tree
		if (VERBOSE > 1) _DBG_ << "responding to \"get tree\"..." << endl;
		string name = msg->getString("tree_name");
		string path = msg->getString("tree_path");
		int64_t nentries = msg->getInt64("num_entries");
		getTree(*response, name, path, nentries);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
		//delete msg;
		//return;
	} else if (cmd == "get tree info") {
		// For a normal request, we send all the information about the TTrees in memory,
		// including the information about the branches, with one TTree per message.
		// For a synchronous request, we can only send one response, so we send info about
		// all of the trees without branch information.  The structure of these messages
		// is slightly different, so the receiver should handle that.
		if (VERBOSE > 1) _DBG_ << "responding to \"get tree info\"..." << endl;
		if (msg->isGetRequest()) {
			treeInfoSync(*response, sender);
		} else {
			treeInfo(sender);
			delete msg;
			in_callback = false;
			return;
		}
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
	} else if (cmd == "list macros") {
		if (VERBOSE > 1) _DBG_ << "responding to \"list macros\"..." << endl;
		listMacros(*response);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
	} else if (cmd == "get macro") {
		// Get name of requested histogram
		string hnamepath = msg->getString("hnamepath");
		if (VERBOSE > 1) _DBG_ << "sending out macro " << hnamepath << endl;
		getMacro(*response, hnamepath);
		if (VERBOSE > 3) _DBG_ << "...done" << endl;
	} else if (cmd == "provide final") {
		finalhists = msg->getStringVector("hnamepaths");
		finalsender = msg->getType();
//		sem_post(&RootSpy_final_sem);
		process_finals = true;
		if (VERBOSE > 1) cerr << "got finalhists" << endl;
		delete msg;
		in_callback = false;
		return;
	}

	// if we've handled a synchronous request, be sure to flush the output buffer
	if (msg->isGetRequest()) {
		if (VERBOSE > 1) _DBG_ << "flushing queues!" << endl;
		cMsgSys->flush();
	} else {
		if (response) delete response;
	}

	delete msg;

	in_callback = false;
}

//---------------------------------
// listHists
//---------------------------------
// Save the top-level directory used to form the list of histograms.
// This is needed because we want to "cd" into the path relative to
// this directory when servicing a "get hist" request below. Specfically,
// using gROOT->cd(PATH) will not work if the histograms are being
// written to a file. At the same time, we don't want to assume that
// the program we're attached to doesn't ever change the value of
// gDirectory while running so we save it here. Note that this doesn't
// actually guarantee it will work due to multiple threads, but it
// at least will for the simplest cases.
void DRootSpy::listHists(cMsgMessage &response) {
	in_list_hists = true;

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	hist_dir = gDirectory;

	// Add histograms to list, recursively traversing ROOT directories
	vector<hinfo_t> hinfos;
	addRootObjectsToList(hist_dir, hinfos);

	// Release lock on ROOT global
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// If any histograms were found, copy their info into the message
	if (hinfos.size() > 0) {
		// Copy elements of hinfo objects into individual vectors
		vector<string> hist_names;
		vector<string> hist_types;
		vector<string> hist_paths;
		vector<string> hist_titles;
		for (unsigned int i = 0; i < hinfos.size(); i++) {
			hist_names.push_back(hinfos[i].name);
			hist_types.push_back(hinfos[i].type);
			hist_paths.push_back(hinfos[i].path);
			hist_titles.push_back(hinfos[i].title);
		}
		response.add("hist_names", hist_names);
		response.add("hist_types", hist_types);
		response.add("hist_paths", hist_paths);
		response.add("hist_titles", hist_titles);

	} else if (hinfos.size() == 0) {

		// cMsg insists there be at least one entry in 
		// a vector<string> payload, otherwise it will
		// throw an exception. If we get here, then don't
		// add the empty payloads. Note that by sending
		// empty payloads, this will result in a 
		// message on the client complaining about a
		// mal-formed response.
	}

	response.setText("hists list");
	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	if (VERBOSE > 3) {
		_DBG_ << "listHist: --> " << response.getText() << endl;
		_DBG_ << "payload: " << response.payloadGetText() << endl;
	}
	in_list_hists = false;
}

//---------------------------------
// getHist
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getHist(cMsgMessage &response, string &hnamepath, bool send_message) {

	in_get_hist = true;

	// split hnamepath into histo name and path
	size_t pos = hnamepath.find_last_of("/");
	if (pos == string::npos) {
		in_get_hist = false;
		return;
	}
	string hname = hnamepath.substr(pos + 1, hnamepath.length() - 1);
	string path = hnamepath.substr(0, pos);
	if (path[path.length() - 1] == ':') path += "/";

	// Looks like ROOT6 has like leading " :"
	if (path.find(": ") == 0) path.erase(0, 2);

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	// Get pointer to ROOT object
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(hname.c_str());
	savedir->cd();

	// If object not found, release lock on ROOT global and return
	if (!obj) {
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		_DBG_ << "Could not find \"" << hname << "\" in path\"" << path << "\"!" << endl;
		in_get_hist = false;
		return;
	}

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_OBJECT);
	tm->WriteObject(obj);
	uint8_t *buff = (uint8_t*) tm->Buffer();
	int len = tm->Length();
	response.add("hnamepath", hnamepath);
	response.add("TMessage", buff, len);
	response.setText("histogram");

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// Send message containing histogram (asynchronously)
	if (send_message) {
		uint64_t tsending = (uint64_t) time(NULL);
		response.add("time_sent", tsending);
		cMsgSys->send(&response);
	}

	in_get_hist = false;
}

//---------------------------------
// getHistUDP
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getHistUDP(void *vresponse, string hnamepath, uint32_t addr32, uint16_t port)
// void DRootSpy::getHistUDP(cMsgMessage &response, string hnamepath, uint32_t addr32, uint16_t port)
		{
	/// This method will be run in a separate thread.
	/// It will pack up the histogram and send it to 
	/// the specified host/port directly via UDP.
	/// Once complete, the thread will exit.
	/// This is an alternative to getHist method which
	/// sends the histograms via cMsg.

	// We should now own the response message object and
	// need to make sure it is freed before returning. Thus,
	// use a unique_ptr to wrap it.
	unique_ptr<cMsgMessage> response((cMsgMessage*) vresponse);

	// Not sure if this will work for C++11 threads. Perhaps
	// yes on systems using posix threads underneath (?....)
	struct sched_param sp = { 16 }; // thread priority
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

	// split hnamepath into histo name and path
	size_t pos = hnamepath.find_last_of("/");
	if (pos == string::npos) return;
	string hname = hnamepath.substr(pos + 1, hnamepath.length() - 1);
	string path = hnamepath.substr(0, pos);
	if (path[path.length() - 1] == ':') path += "/";

	// Lock access to ROOT global while we access it
	pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);

	// Get pointer to ROOT object
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(hname.c_str());
	savedir->cd();

	// If object not found, release lock on ROOT global and return
	if (!obj) {
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		return;
	}

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_OBJECT | kMESS_ZIP);
	tm->WriteObject(obj);
	tm->SetCompressionLevel(9);
	tm->Compress();
	uint8_t *buff = (uint8_t*) tm->CompBuffer();
	int len = tm->CompLength();

	// There is a limit to how big a UDP packet can be.
	// Nominally, this is quoted at 512. another common
	// size that originates with Windows is 65507.
	// We limit the payload size to 65000 and if it is
	// larger than that, we revert to sending it as
	// a cMsg.
	if (len > 65000) {

		// Send as cMsg
		response->setText("histogram");
		response->add("hnamepath", hnamepath);
		response->add("TMessage", buff, len);

		// Finished with TMessage object. Free it and release lock on ROOT global
		delete tm;
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

		if (VERBOSE > 2) _DBG_ << "Message size too big for UDP (" << len << ">65000) sending as cMsg ..." << endl;
		uint64_t tsending = (uint64_t) time(NULL);
		response->add("time_sent", tsending);
		cMsgSys->send(*response);

	} else {

		// Send as UDP packet

		string sender = response->getType();

		// Allocate buffer for UDP packet and fill it
		// (- sizeof(uint32_t) is to remove buff_start)
		uint32_t udpbuff_len = sizeof(rs_udpmessage) - sizeof(uint32_t) + (uint32_t) len;
		uint8_t *udpbuff = new uint8_t[udpbuff_len];
		rs_udpmessage *rsudp = (rs_udpmessage*) udpbuff;
		rsudp->len = udpbuff_len;
		rsudp->mess_type = kUDP_HISTOGRAM;
		memset(rsudp->hnamepath, 0, 256);
		strcpy((char*) rsudp->hnamepath, hnamepath.c_str());
		memset(rsudp->sender, 0, 256);
		strcpy((char*) rsudp->sender, myname.c_str());
		rsudp->time_sent = (uint64_t) time(NULL);
		rsudp->time_requester = response->getUint64("time_requester");
		rsudp->time_received = response->getUint64("time_received");
		rsudp->buff_len = len;
		memcpy((uint8_t*) &rsudp->buff_start, buff, len);

		// Finished with TMessage object. Free it and release lock on ROOT global
		delete tm;
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

		// Send UDP packet
		struct sockaddr_in si_other;
		int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fd < 0) {
			_DBG_ << "Can't create socket!" << endl;
			delete[] udpbuff;
			return;
		}
		memset((char*) &si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = port;
		si_other.sin_addr.s_addr = htonl(addr32);

		if (VERBOSE > 2) _DBG_ << "Sending UDP packet with len = " << udpbuff_len << endl;
		sendto(fd, udpbuff, udpbuff_len, 0, (sockaddr*) &si_other, sizeof(si_other));

		delete[] udpbuff;
		close(fd);
	}
}

//---------------------------------
// getHists
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getHists(cMsgMessage &response, vector<string> &hnamepaths) {
	in_get_hists = true;

	// Create a new cMsgMessage for each hnamepath and add it
	// the vector of cMsgMessage's that are added a payload of
	// the cMsgMessage that is actually sent.
	vector<cMsgMessage*> cmsgs;
	for (uint32_t j = 0; j < hnamepaths.size(); j++) {

		cMsgMessage *cmsg = new cMsgMessage();

		getHist(*cmsg, hnamepaths[j], false);
		if (cmsg->payloadGetCount() < 2) {
			// No payload. Must have been a problem. Drop message.
			cout << "Unable to create cMsg for " << hnamepaths[j] << endl;
			delete cmsg;
			continue;
		}
		cmsgs.push_back(cmsg);
	}

	if (cmsgs.empty()) {
		_DBG_ << "cmsgs is empty!!" << endl;
		in_get_hists = false;
		return;
	}

	response.setText("histograms");
	response.add("histograms", cmsgs);

	// Send message containing histogram (asynchronously)
	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	in_get_hists = false;
}

//---------------------------------
// listMacros
//---------------------------------
void DRootSpy::listMacros(cMsgMessage &response) {
	in_list_macros = true;

	// If any histograms were found, copy their info into the message
	if (macros.size() > 0) {
		vector<string> macro_names;
		vector<string> macro_paths;
		for (map<string, macro_info_t>::iterator macro_itr = macros.begin(); macro_itr != macros.end(); macro_itr++) {
			macro_names.push_back(macro_itr->second.name);
			macro_paths.push_back(macro_itr->second.path);
		}
		response.add("macro_names", macro_names);
		response.add("macro_paths", macro_paths);
	}

	response.setText("macros list");
	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	in_list_macros = false;
}

//---------------------------------
// getMacro
//---------------------------------
void DRootSpy::getMacro(cMsgMessage &response, string &hnamepath) {

	in_get_macro = true;

	// find the macro
	map<string, macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if (the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		in_get_macro = false;
		return;
	}
	macro_info_t &the_macro = the_macro_itr->second;

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	TDirectory *savedir = gDirectory;

	// for the details on TMemFile usage, see getTree()
	TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");

	// fill the TMemFile with our payload:
	//  1) TObjString of the macro "code"
	//  2) TObjArray of any histograms used by the macro
	TObjString *macro_str = new TObjString(the_macro.macro.c_str());
	//cerr << " ======= MACRO =======" << endl;
	//cerr << the_macro.macro << endl;
	//cerr << " ======= TObjString MACRO =======" << endl;
	//cerr << macro_str->GetString().Data() << endl;
	macro_str->Write("macro");

	//TObjArray *hists = new TObjArray(the_macro.histograms.size());
	if (the_macro.histograms.size() > 0) {
		//TObjArray *hists = new TObjArray();
		//for(unsigned int i=0; i<the_macro.histograms.size(); i++)
		//hists->AddLast(the_macro.histograms[i]);
		//hists->Write("hists");
		for (unsigned int i = 0; i < the_macro.histograms.size(); i++)
			the_macro.histograms[i]->Write();
	}

	savedir->cd();

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_ANY);
	f->Write();
	//tm->WriteTString(f->GetName());
	tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
	f->CopyTo(*tm);

	if (VERBOSE > 1) _DBG_ << " TMemFile length = " << f->GetEND() << endl;

	response.setByteArray(tm->Buffer(), tm->Length());
	response.add("macro_name", the_macro.name);
	response.add("macro_path", the_macro.path);
	response.add("macro_version", the_macro.version);
	response.setText("macro");

	// clean up everything
	if (f) {
		f->Close();
		delete f;
		f = NULL;
	}

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// Send message containing histogram (asynchronously)
	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	in_get_macro = false;
}

//---------------------------------
// getTree
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getTree(cMsgMessage &response, string &name, string &path, int64_t nentries_to_save) {
	in_get_tree = true;

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	// Get pointer to tree
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(name.c_str());
	savedir->cd();

	// If object not found, release lock on ROOT global and return
	if (!obj) {
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		in_get_tree = false;
		return;
	}

	// Make sure this is a TTree
	if (strcmp(obj->ClassName(), "TTree")) {
		cout << "Request for tree \"" << name << "\" but that's not a TTree (it's a " << obj->ClassName() << ")" << endl;
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		in_get_tree = false;
		return;
	}

	// Cast object as TTree and decide how many entries to copy into the tree we're going
	// to package up and send.
	TTree *tree = static_cast<TTree *>(obj);
	int64_t nentries_in_tree = tree->GetEntries();
	int64_t firstentry = nentries_in_tree - nentries_to_save;
	if (firstentry < 0 || nentries_to_save <= 0) firstentry = 0;
	int64_t nentries = nentries_in_tree - firstentry;
	if (VERBOSE > 1) cout << "Sending TTree " << tree->GetName() << "  (entries: " << nentries_in_tree << "  sending: " << nentries << ")" << endl;

	// Copy TTree
	TTree *tree_copy = tree;

	// Sending the TTree object seemed to have problems if
	// the tree contained an array (e.g. "data[100]/F" in
	// the branch definition). We write the tree to a TMemFile
	// and then serialize and send that object. This follows the
	// treeClient.C and fastMergeServer.C ROOT tutorials.
	savedir = gDirectory;
	TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");

	tree_copy = tree->CloneTree(0);

	savedir->cd();

	if (!tree_copy) {
		cout << "Unable to make temporary copy of tree for transport!" << endl;
		if (f) {
			f->Close();
			delete f;
			f = NULL;
		}
		pthread_rwlock_unlock(gROOTSPY_RW_LOCK);
		in_get_tree = false;
		return;
	}

	// Copy requested number of entries
	for (int64_t i = 0; i < nentries; i++) {
		tree->GetEntry(firstentry + i);
		tree_copy->Fill();
	}

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_ANY);
	f->Write();
	tm->WriteTString(f->GetName());
	tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
	f->CopyTo(*tm);

	response.setByteArray(tm->Buffer(), tm->Length());
	response.add("tree_name", name);
	response.add("tree_path", path);
	response.setText("tree");

	// clean up temporary copy of tree
	if (tree_copy != tree) {
		// see source for TTree::MergeTrees
		tree->GetListOfClones()->Remove(tree_copy);
		tree_copy->ResetBranchAddresses();
		delete tree_copy;
	}
	if (f) {
		f->Close();
		delete f;
		f = NULL;
	}

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	in_get_tree = false;
}

//---------------------------------
// treeInfo
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfo(string sender) {
	in_get_tree_info = true;

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	hist_dir = gDirectory;
	findTreeObjForMsg(hist_dir, sender);

	// Release ROOT global
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	in_get_tree_info = false;
}

//---------------------------------
// treeInfoSync
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfoSync(cMsgMessage &response, string sender) {

	in_get_tree_info = true;

	// Lock access to ROOT global while we access it
	pthread_rwlock_rdlock(gROOTSPY_RW_LOCK);

	hist_dir = gDirectory;
	vector<string> tree_names, tree_titles, tree_paths;
	findTreeNamesForMsg(hist_dir, tree_names, tree_titles, tree_paths);

	// Look for any open files and search those as well
	TSeqCollection *coll = gROOT->GetListOfFiles();
	for (int i = 0; i < coll->GetSize(); i++) {
		TObject *obj = coll->At(i);
		TDirectory *dir = dynamic_cast<TDirectory*>(obj);
		if (!dir) continue;
		if (dir == hist_dir) continue; // already handled
		findTreeNamesForMsg(dir, tree_names, tree_titles, tree_paths);
	}

	// Release ROOT global
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// build message
	if (!tree_names.empty()) {
		response.add("tree_names", tree_names);
		response.add("tree_titles", tree_titles);
		response.add("tree_paths", tree_paths);
	}
	response.setText("tree info");
	uint64_t tsending = (uint64_t) time(NULL);
	response.add("time_sent", tsending);
	cMsgSys->send(&response);

	in_get_tree_info = false;
}

//---------------------------------
// addRootObjectsToList
//---------------------------------
//This method recursively traverses through histogram directories making hinfo_t 
//objects out of individual histograms and then adds the info_t objects to a list.
// The gROOTSPY_RW_LOCK should already be locked before calling this so we should not
// lock it here.
void DRootSpy::addRootObjectsToList(TDirectory *dir, vector<hinfo_t> &hinfos) {
	string path = dir->GetPath();

	TList *list = dir->GetList();
	TIter next(list);

	if (VERBOSE > 3) {
		_DBG_ << "addRootObjectsToList: directory is: " << endl;
		if (dir) dir->Print();
		if (dir) dir->pwd();
		if (dir) dir->ls();
	}

	while (TObject *obj = next()) {

		if (VERBOSE > 3 ) _DBG_ << "OBJ: " << obj->GetName() << " " << obj->ClassName() << endl;
		// Make hinfo_t objects out of the individual histogram information.
		// For now, we only report objects inheriting from TH1.
		TH1 *hHist = dynamic_cast<TH1*>(obj);

		if (hHist) {
			hinfo_t hi;
			hi.name = obj->GetName();
			hi.type = obj->ClassName();
			hi.path = path;
			hi.title = hHist->GetTitle();
			if (hi.title.length() == 0) hi.title = "-"; // cMsg gives "clientThread: error reading message" if title is empty string (??)

			// user may specify simple patterns for hist names
			// to exclude from list
			bool exclude = false;
			for (auto s : filter_patterns)
				if (hi.name.find(s) != s.npos) exclude = true;

			if (!exclude) hinfos.push_back(hi);
		}

		// If this happens to be a directory, recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if (subdir != NULL && subdir != dir) {

			if (VERBOSE > 3) {
				_DBG_ << "addRootObjectsToList: sub-directory is: " << endl;
				subdir->pwd();
				subdir->ls();
			}
			addRootObjectsToList(subdir, hinfos);
		}
	}
}

//---------------------------------
// findTreeNamesForMsg
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_RW_LOCK should already be locked before calling this so we should not
// lock it here.
void DRootSpy::findTreeNamesForMsg(TDirectory *dir, vector<string> &tree_names, vector<string> &tree_titles, vector<string> &tree_paths) {
	if (VERBOSE > 1) _DBG_ << "Looking for trees in: " << dir->GetName() << endl;

	TList *list = dir->GetList();
	TIter next(list);
	//search through root directories for a tree object.
	while (TObject *obj = next()) {
		TTree *hTree = dynamic_cast<TTree*>(obj);
		if (hTree) {
			tree_names.push_back(obj->GetName());
			tree_titles.push_back(obj->GetTitle());
			tree_paths.push_back(dir->GetPath());
		}

		// If this happens to be a directory,
		//recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if (subdir != NULL && subdir != dir) {
			findTreeNamesForMsg(subdir, tree_names, tree_titles, tree_paths);
		}
	}
}

//---------------------------------
// findTreeObjForMsg
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_RW_LOCK should already be locked before calling this so we should not
// lock it here.
void DRootSpy::findTreeObjForMsg(TDirectory *dir, string sender) {
	TList *list = dir->GetList();
	TIter next(list);
	//search through root directories for a tree object.
	while (TObject *obj = next()) {
		TTree *hTree = dynamic_cast<TTree*>(obj);
		if (hTree) {
			TObjArray *branch_list = hTree->GetListOfBranches();
			vector<string> branch_info;
			//Search through the found tree object for information.
			traverseTree(branch_list, branch_info);
			if (!branch_info.empty()) {
				cMsgMessage response;
				response.setSubject(sender);
				response.setType(myname);
				response.add("tree_name", obj->GetName());
				response.add("tree_title", obj->GetTitle());
				response.add("tree_path", dir->GetPath());
				response.add("branch_info", branch_info);
				response.setText("tree info");
				cMsgSys->send(&response);
			}
		}

		// If this happens to be a directory,
		//recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if (subdir != NULL && subdir != dir) {
			findTreeObjForMsg(subdir, sender);
		}
	}
}

//---------------------------------
// traverseTree
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_RW_LOCK should already be locked before calling this so we should not
// lock it here.
void DRootSpy::traverseTree(TObjArray *branch_list, vector<string> &treeinfo) {

	if (!branch_list) return;
	for (int i = 0; i < branch_list->GetSize(); i++) {
		TBranch *branch = dynamic_cast<TBranch*>(branch_list->At(i));
		if (branch) {
//			cout<<"BranchName: "<<branch->GetName()<<endl;
			treeinfo.push_back(string(branch->GetName()));
		} else
			break;
		traverseTree(branch->GetListOfBranches(), treeinfo);
	}
}

//---------------------------------
// ReturnFinals
//---------------------------------
//Justin Barry 07.08.10
//sdobbs  4/18/13
//TODO: documentation comment.
void DRootSpy::ReturnFinals(void) {

	_DBG_ << "Starting ReturnFinals()..." << endl;

	// loop waiting for final histogram requests
	while (true) {
		_DBG_ << "starting final histogram loop" << endl;

		// make sure that final histograms are filled
		while (!process_finals)
			usleep(100000);
		//sem_wait(&RootSpy_final_sem);

		_DBG_ << "Got semaphore signal" << endl;

		if (!finalhists) { // sanity check
			process_finals = false;
			continue;
		}

		_DBG_ << "building final histograms" << endl;
		_DBG_ << "saving " << finalhists->size() << " histograms..." << endl;

		gDirectory->ls(); // debug info

		// send out all the histograms
		vector<string>::iterator hnamepath = finalhists->begin();
		for (; hnamepath != finalhists->end(); hnamepath++) {

			_DBG_ << "looking for " << *hnamepath << endl;

			// split hnamepath into histo name and path
			size_t pos = hnamepath->find_last_of("/");
			if (pos == string::npos) continue;
			string hname = hnamepath->substr(pos + 1, hnamepath->length() - 1);
			string path = hnamepath->substr(0, pos);
			if (path[path.length() - 1] == ':') path += "/";
			// This seems to work, but is certainly not very thread safe.
			// perhaps there's a better way...?
			TDirectory *savedir = gDirectory;
			// use directory from last time "list hist" request was serviced.
			hist_dir->cd(path.c_str());
			TObject *obj = gROOT->FindObject(hname.c_str());
			savedir->cd();
			if (!obj) continue;

			// debugging info
			_DBG_ << "found item: " << endl;
			obj->Print();

			// Serialize object and put it into a response message
			TMessage *tm = new TMessage(kMESS_OBJECT);
			tm->WriteObject(obj);

			// Prepare to send a response
			cMsgMessage response;
			response.setSubject(finalsender);
			response.setType(myname);
			response.setByteArrayNoCopy(tm->Buffer(), tm->Length());

			_DBG_ << "sending message of length = " << tm->Length() << endl;

			response.add("hnamepath", *hnamepath);
			response.setText("final hists");
			cMsgSys->send(&response);

			_DBG_ << "sending out " << *hnamepath << endl;
		}
	}
}

//---------------------------------
// ReturnFinalsC
//---------------------------------
void *ReturnFinalsC(void * ptr) {
	DRootSpy* rsptr = (DRootSpy *) ptr;
	rsptr->ReturnFinals();

	return NULL; // avoid compiler warnings
}

//---------------------------------
// RegisterMacro
//---------------------------------
bool DRootSpy::RegisterMacro(string name, string path, string macro_data) {
	// validate name
	// right now we don't do anything special...
	if (name == "") {
		_DBG_ << "trying to register a macro with no name!" << endl;
		return false;
	}

	cout << "ROOTSPY: Registering macro \"" << name << "\"" << endl;

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if (path == "") {
		path = "/";
	}

	// add the macro to our list!
	string hnamepath = path + "/" + name;
	macros[hnamepath] = macro_info_t(name, path);
	macros[hnamepath].macro = macro_data;

	return true;
}

//---------------------------------
// AddMacroHistogram 
//---------------------------------
bool DRootSpy::AddMacroHistogram(string name, string path, TH1 *the_hist) {
	// validate name
	// right now we don't do anything special...
	if (name == "") {
		_DBG_ << "trying to add a histogram to a macro with no name!" << endl;
		return false;
	}

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if (path == "") {
		path = "/";
	}

	// find the macro
	string hnamepath = path + "/" + name;
	map<string, macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if (the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		return false;
	}

	// add the histogram!
	the_macro_itr->second.histograms.push_back(the_hist);

	return true;
}

//---------------------------------
// AddMacroHistogram 
//---------------------------------
bool DRootSpy::AddMacroHistogram(string name, string path, vector<TH1*> the_hists) {
	for (vector<TH1*>::iterator hist_itr = the_hists.begin(); hist_itr != the_hists.begin(); hist_itr++) {
		if (!AddMacroHistogram(name, path, *hist_itr)) {
			_DBG_ << "error adding histogram to macro " << name << "/" << path << endl;
			return false;
		}
	}

	return true;
}

//---------------------------------
// SetMacroVersion
//---------------------------------
bool DRootSpy::SetMacroVersion(string name, string path, int version_number) {
	// validate name
	// right now we don't do anything special...
	if (name == "") {
		_DBG_ << "trying to add a histogram to a macro with no name!" << endl;
		return false;
	}

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if (path == "") {
		path = "/";
	}

	// find the macro
	string hnamepath = path + "/" + name;
	map<string, macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if (the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		return false;
	}

	// set the value!
	the_macro_itr->second.version = version_number;

	return true;
}

