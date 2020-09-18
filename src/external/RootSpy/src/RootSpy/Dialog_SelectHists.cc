// $Id: Dialog_SelectHists.cc 21880 2017-01-24 22:31:10Z davidl $
//
//    File: Dialog_SelectHists.cc
// Created: Mon Sep 14 08:54:47 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_SelectHists.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TTimer.h>
#include <TFile.h>
#include <TDirectory.h>
#include <unistd.h>
#include <TGFileDialog.h>

//---------------------------------
// Dialog_SelectHists    (Constructor)
//---------------------------------
Dialog_SelectHists::Dialog_SelectHists(list<string> *hnamepaths, const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	this->rstab = NULL;
	Init(hnamepaths);
}

//---------------------------------
// Dialog_SelectHists    (Constructor)
//---------------------------------
Dialog_SelectHists::Dialog_SelectHists(RSTab *rstab, const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	this->rstab = rstab;
	Init(&rstab->hnamepaths);
}

//---------------------------------
// Init
//---------------------------------
void Dialog_SelectHists::Init(list<string> *hnamepaths)
{
	// Store pointer to the container where we should store
	// the results after the user hits "OK"
	this->hnamepaths = hnamepaths;
//_DBG_<<"Creating select hists dialog with: " << hnamepaths->size() << " entries" <<endl;

	// Define all of the of the graphics objects. 
	CreateGUI();
	
	// The item_checked map keeps track of the checked status
	// of all items, even ones not currently displayed in the
	// listbox due to filters. Initialize it with the input
	// hnamepaths list since those should all be checked initially
	list<string>::iterator it = hnamepaths->begin();
	for(; it!=hnamepaths->end(); it++) item_checked[*it] = true;

for(it=hnamepaths->begin(); it!=hnamepaths->end(); it++) _DBG_<<"hnamepaths="<<(*it)<<endl;

	// Set up timer to call the DoTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_SelectHists", this, "DoTimer()");
	sleep_time = 500;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL) - 3;
	last_called = now;
	last_ping_time = now;
	last_hist_time = now;
	last_type_filters = 0;

	filter_str = "";
	last_filter_str = "";
	
	//viewStyle = rs_info::kViewByServer;
	viewStyle = rs_info::kViewByObject;
	
	DoTimer();
	
	// Make initial window height something resonable
	TGDimension dim = GetDefaultSize();
	UInt_t N = RS_INFO->histdefs.size();
	if(N>40) N=40; // limit how tall we make the window
	if(N>11) dim.fHeight += (N-11)*19; // increase window's height if needed

	// Finish up and map the window
	SetWindowName("RootSpy Select Server/Histogram");
	SetIconName("SelectHist");
	MapSubwindows();
	Resize(dim);
	MapWindow();

}

//---------------------------------
// ~Dialog_SelectHists    (Destructor)
//---------------------------------
Dialog_SelectHists::~Dialog_SelectHists()
{

}

//-------------------
// CloseWindow
//-------------------
void Dialog_SelectHists::CloseWindow(void)
{
	if(timer){
		timer->Stop();
		delete timer;
		timer = NULL;
	}
	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
}

//-------------------
// DoTimer
//-------------------
void Dialog_SelectHists::DoTimer(void)
{
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	
	time_t now = time(NULL);

	// Ping servers occasionally to make sure our server list is up-to-date
	if(now-last_ping_time >= 3){
		RS_CMSG->PingServers();
		last_ping_time = now;
	}
	
	// Ping server occasionally to make sure our histogram list is up-to-date
	if(now-last_hist_time >= 3){
		// Loop over servers
		RS_INFO->Lock();
		map<string,server_info_t>::iterator iter = RS_INFO->servers.begin();
		for(; iter!=RS_INFO->servers.end(); iter++){
			string servername = iter->first;
			if(servername!=""){
				RS_CMSG->RequestHists(servername);
				RS_CMSG->RequestMacroList(servername);
			}
		}
		RS_INFO->Unlock();
		last_hist_time = now;
	}

	// Get list of active hnamepaths on active servers
	// We get a copy of the hinfo_t objects so we don't
	// have to hold a mutex lock to prevent them from
	// changing on us while we're working with them.
	vector<hid_t> hids;
	GetAllHistos(hids);

	// Check if list of histos has changed
	bool hists_changed = false;
	if(last_hids.size() == hids.size()){
		for(unsigned int i=0; i<hids.size(); i++){
			if(hists_changed)break;
			if(hids[i] !=last_hids[i])hists_changed=true;
		}
	}else{
		hists_changed = true;
	}
	
	// The type filters are applied in UpdateListTree. Just check here
	// if the filter settings have changed.
	bool filterTH1 = bFilterTH1->GetState() == kButtonUp;
	bool filterTH2 = bFilterTH2->GetState() == kButtonUp;
	bool filterTProfile = bFilterTProfile->GetState() == kButtonUp;
	bool filterMacro = bFilterMacro->GetState() == kButtonUp;
	int type_filters = (filterTH1<<0) + (filterTH2<<1) + (filterTProfile<<2) + (filterMacro<<3);
	bool filters_changed = (type_filters!=last_type_filters) || (filter_str != last_filter_str);
	last_type_filters = type_filters;
	last_filter_str = filter_str;

	// Refill TGListTree if needed
	if(hists_changed || filters_changed)UpdateListTree(hids);

	last_called = now;
}

//---------------------------------
// DoOK
//---------------------------------
void Dialog_SelectHists::DoOK(void)
{	
//_DBG_<<"Replacing select hists dialog that has: " << hnamepaths->size() << " entries" <<endl;

	GetChecked(listTree->GetFirstItem());

	// Replace existing list of hnamepaths
	hnamepaths->clear();

	map<string,bool>::iterator iter = item_checked.begin();
	for(; iter!=item_checked.end(); iter++){
		if(iter->second) hnamepaths->push_back(iter->first);
	}

//_DBG_<<"item_checked.size(): " << 	item_checked.size() << endl;
//_DBG_<<" hnamepaths->size(): " << 	hnamepaths->size() << endl;
	
#if 0
	map<hid_t, TGListTreeItem*>::iterator hist_items_iter = hist_items.begin();
	for(; hist_items_iter!=hist_items.end(); hist_items_iter++){
	
		const hid_t &hid = hist_items_iter->first;
		TGListTreeItem *item = hist_items_iter->second;
		
		if(	item->IsChecked() ) hnamepaths->push_back(hid.hnamepath);
		
//		// Look for this item in the list of hnamepaths to display
//		list<string>::iterator it = find(hnamepaths->begin(), hnamepaths->end(), hid.hnamepath);
//		if(it == hnamepaths->end()){
//			if(	item->IsChecked() ) hnamepaths->push_back(hid.hnamepath);
//		}else{
//			if(!item->IsChecked() ) hnamepaths->erase(it);
//		}
	}
#endif
	
	// If we were given an RSTab pointer, use it to update the current tab
	if(rstab) rstab->DoUpdateWithFollowUp();
	
	DoCancel();
}

//---------------------------------
// DoSelectSingleHist
//---------------------------------
void Dialog_SelectHists::DoSelectSingleHist(TGListTreeItem* entry, Int_t btn)
{
    char path[512];
    listTree->GetPathnameFromItem(entry, path);
	 string hnamepath(path);
	 
	 // Find if this is a histogram/macro by checking if there is an
	 // entry in the item_checked map. If not, then ignore the double-click
	 if(item_checked.find(hnamepath) == item_checked.end()) return;
	 
	 RS_INFO->Lock();
	 
	 // Now, check if this histogram is already in the list passed to us by
	 // the RSTab. If not, then add it.
	 list<string>::iterator it = hnamepaths->begin();
	 bool found = false;
	 for(; it!=hnamepaths->end(); it++){
	 	if(*it == hnamepath){
			found = true;
			break;
		}
	 }
	 if(!found) hnamepaths->push_back(hnamepath);
	 
	 // Tell RSTab to make this the currently displayed histo/macro
	 if(rstab) rstab->SetTo(hnamepath);
	 
	 RS_INFO->Unlock();

    CloseWindow();
}


//---------------------------------
// DoCancel
//---------------------------------
void Dialog_SelectHists::DoCancel(void)
{
	// The following will also call the destructor
	CloseWindow();
}

//---------------------------------
// DoClickedEntry
//---------------------------------
void Dialog_SelectHists::DoClickedEntry(TGListTreeItem* entry, Int_t btn)
{
	// This seems to be called when the text or icon but not the actual
	// checkbox is clicked.

	bool check = !entry->IsChecked();
	
	listTree->CheckAllChildren(entry, check);
	
	// Loop over all children, setting the item_checked 
	GetChecked(entry, false);
}

//---------------------------------
// DoCheckedEntry
//---------------------------------
void Dialog_SelectHists::DoCheckedEntry(TObject* obj, Int_t check)
{
	// Called whenever a checkbox is toggled in listTree.
	// Unbelievably enough, the value of "obj" is the parent
	// and NOT the actual item next to the checkbox being clicked.
	// In order to get the correct item, we have to ask for it
	// via the mouse location. (What an incredible waste of time
	// to figure this out!)
	TGListTreeItem *item = listTree->GetBelowMouse();
	if(!item) return;

	listTree->CheckAllChildren(item, check);
	
	// Loop over all children, setting the item_checked 
	GetChecked(item, false);
}

//---------------------------------
// GetChecked
//---------------------------------
void Dialog_SelectHists::GetChecked(TGListTreeItem *item, bool check_siblings)
{

	while(item){
		// Set checked state, but only if this is currently listed in item_checked
		// Otherwise, it is probably not a valid hnamepath (just a partial)
		char str[512] = "";
		listTree->GetPathnameFromItem(item, str);
		string hnamepath(str);

		// Look if the specified item is in the item_checked map
		bool item_exists = item_checked.find(hnamepath) != item_checked.end();
		if(!item_exists){
			// macros start with 2 slashes (e.g. "//CDC_occupancy")
			hnamepath = string("/") + hnamepath;
			item_exists = item_checked.find(hnamepath) != item_checked.end();
		}
		
		// Update item_checked with current checked status
		if(item_exists) {
			item_checked[hnamepath] = item->IsChecked();
		}
//_DBG_<<"  checking: " << (item_exists ? (item_checked[hnamepath] ? "Y":"N"):"X") << " " << hnamepath << endl;
		if(item->GetFirstChild()) GetChecked(item->GetFirstChild());
		
		item = check_siblings ? item->GetNextSibling():NULL;
	}
}

//---------------------------------
// DoSetViewByObject
//---------------------------------
void Dialog_SelectHists::DoSetViewByObject(void)
{
	bool changed = (viewStyle != rs_info::kViewByObject);
	viewStyle = rs_info::kViewByObject;
	viewByObject->SetState(viewStyle==rs_info::kViewByObject ? kButtonDown:kButtonUp);
	viewByServer->SetState(viewStyle==rs_info::kViewByObject ? kButtonUp:kButtonDown);
	if(changed){
		vector<hid_t> hids;
		GetAllHistos(hids);
		UpdateListTree(hids);
	}
}

//---------------------------------
// DoSetViewByServer
//---------------------------------
void Dialog_SelectHists::DoSetViewByServer(void)
{
	bool changed = (viewStyle != rs_info::kViewByServer);
	viewStyle = rs_info::kViewByServer;
	viewByObject->SetState(viewStyle==rs_info::kViewByObject ? kButtonDown:kButtonUp);
	viewByServer->SetState(viewStyle==rs_info::kViewByObject ? kButtonUp:kButtonDown);
	if(changed){
		vector<hid_t> hids;
		GetAllHistos(hids);
		UpdateListTree(hids);
	}
}

//---------------------------------
// DoFilterHists
//---------------------------------
void Dialog_SelectHists::DoFilterHists(void)
{
    // save string to filter against
    filter_str = filterTextBox->GetText();

    vector<hid_t> hids, good_hids;
    GetAllHistos(hids);
    UpdateListTree(hids);
}

//---------------------------------
// DoSelectAll
//---------------------------------
void Dialog_SelectHists::DoSelectAll(void)
{
	TGListTreeItem *my_item = listTree->GetFirstItem();
	while(my_item){
		my_item->CheckAllChildren(kTRUE);
		my_item = my_item->GetNextSibling();
	}
	
	Redraw(this);
}

//---------------------------------
// DoSelectNone
//---------------------------------
void Dialog_SelectHists::DoSelectNone(void)
{
	TGListTreeItem *my_item = listTree->GetFirstItem();
	while(my_item){
		my_item->CheckAllChildren(kFALSE);
		my_item = my_item->GetNextSibling();
	}

	Redraw(this);
}

//---------------------------------
// Redraw
//---------------------------------
void Dialog_SelectHists::Redraw(TGFrame *f)
{
	// This unfortunate business seems to be needed to get the listbox to redraw itself
	TGDimension dim = f->GetSize();
	dim.fWidth+=1;
	f->Resize(dim);
	dim.fWidth-=1;
	f->Resize(dim);
}

//---------------------------------
// GetAllHistos
//---------------------------------
void Dialog_SelectHists::GetAllHistos(vector<hid_t> &hids)
{
	RS_INFO->Lock();

	// Make list of all histograms from all servers
	map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.begin();
	for(; server_info_iter!=RS_INFO->servers.end(); server_info_iter++){//iterates over servers
		const string &server = server_info_iter->first;
		const vector<string> &hnamepaths = server_info_iter->second.hnamepaths;
		for(unsigned int j=0; j<hnamepaths.size(); j++){//iterates over histogram paths

			// add this hinfo_t object to the list
			hids.push_back(hid_t(server, hnamepaths[j]));
		}
	}
	RS_INFO->Unlock();
}

//---------------------------------
// UpdateListTree
//---------------------------------
void Dialog_SelectHists::UpdateListTree(vector<hid_t> hids)
{
_DBG__;
	// Delete any existing items (seems like there should be a better way)
	TGListTreeItem *my_item = listTree->GetFirstItem();
	while(my_item){
		TGListTreeItem *next_item = my_item->GetNextSibling();
		listTree->DeleteItem(my_item);
		my_item = next_item;
	}
	server_items.clear();
	hist_items.clear();

	// Filter by type
	bool filterTH1 = bFilterTH1->GetState() == kButtonUp;
	bool filterTH2 = bFilterTH2->GetState() == kButtonUp;
	bool filterTProfile = bFilterTProfile->GetState() == kButtonUp;
	bool filterMacro = bFilterMacro->GetState() == kButtonUp;

	// Loop over histograms
	for(unsigned int i=0; i<hids.size(); i++){
		string &hnamepath = hids[i].hnamepath;
		string &server = hids[i].serverName;
		
		// Get hdef_t object for this hist
		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hids[i].hnamepath);
		if(hdef_iter==RS_INFO->histdefs.end()){
			_DBG_<<"No histo definition for hid: server=\""<<server<<"\" hnamepath=\""<<hnamepath<<"\"!"<<endl;
			continue;
		}
		hdef_t *hdef = &(hdef_iter->second);
		
		// Apply type filters
		if(filterTH1 && hdef->type==hdef_t::oneD) continue;
		if(filterTH2 && hdef->type==hdef_t::twoD) continue;
		if(filterTProfile && hdef->type==hdef_t::profile) continue;
		if(filterMacro && hdef->type==hdef_t::macro) continue;
		if(filter_str!="")
			if(hnamepath.find(filter_str) == string::npos) continue;
		
		// Check if this hnamepath is already in the item_checked map
		// and add it (unchecked) if it is not
		if(item_checked.find(hnamepath) == item_checked.end()) item_checked[hnamepath] = false;

		// Here we want to create a vector with each of the path
		// elements (folders) to be displayed and then the final
		// element (either histogram or server). 
		vector<string> path;
		string final;
		if(viewStyle==rs_info::kViewByObject){
			path.insert(path.end(), hdef->dirs.begin(), hdef->dirs.end());
			path.push_back(hdef->name);
			final = server;
		}else{
			path.push_back(server);
			path.insert(path.end(), hdef->dirs.begin(), hdef->dirs.end());
			final = hdef->name;
		}
		path.push_back(final);

		// Loop over path elements, adding them as needed
		vector<TGListTreeItem *> items;
		string mypath = "";
		for(unsigned int j=0; j<path.size(); j++){
			mypath += "/" + path[j];
			TGListTreeItem *item = listTree->FindItemByPathname(mypath.c_str());
			if(!item){
				TGListTreeItem *last_item = items.size()>0 ? items[items.size()-1]:NULL;
				item = listTree->AddItem(last_item, path[j].c_str());
			 	item->SetUserData(last_item);
				listTree->SetCheckBox(item, kTRUE);
				//listTree->SetCheckBoxMode(item, TGListTree::kSimple);
				item->SetCheckBoxPictures(checked_t, unchecked_t);

				bool show_open = false;
				/*
				// Sets the directories to show all, with the 
				// exception of the servers if viewStyle is by object
				if((j+2)<path.size()) show_open = true; // show open if not last item in path (i.e. server)
				if(viewStyle!=rs_info::kViewByObject) show_open = true; // show eveything open if viewing by server
				*/

				// by default, leave all directories collapsed except the first level
				// to prevent overwhelming the user
				if( viewStyle==rs_info::kViewByObject ) {
					if( j==0 && ((j+2)<path.size()) )
						show_open = true;   // show first level, and not servers
				} else {
					// if viewing by server, show everything
					show_open = true; 
				}
				item->SetOpen(show_open);
			}
			items.push_back(item);
		}
		
		// Make sure we have at least 2 items (server and hist)
		if(items.size()<2)continue;
		
		// Set appropriate pictures and checkbox status for hist and 
		// server elements
		TGListTreeItem *server_item=NULL;
		TGListTreeItem *hist_item=NULL;
		bool server_checkbox=true;
		//bool hist_checkbox=true;
		
		// If hnamepaths list passed to us is empty, assume it is new and 
		// check it only if is marked as active. Otherwise, turn off all hists
		// no appearing in the hnamepaths list
		if(!hnamepaths->empty()){
			list<string>::iterator it = find(hnamepaths->begin(), hnamepaths->end(), hnamepath);
			//hist_checkbox = (it != hnamepaths->end());
		} else {
			//hist_checkbox = hdef->active;
		}

		if(viewStyle==rs_info::kViewByObject){
			server_item = items[items.size()-1];
			hist_item = items[items.size()-2];
			server_item->SetCheckBox(kFALSE); // don't draw checkbox next to server
			//server_checkbox = hdef->servers[server];
			//hist_checkbox = hdef->active;
		}else{
			server_item = items[0];
			hist_item = items[items.size()-1];
			//server_checkbox = RS_INFO->servers[server].active;
			//hist_checkbox = hdef->servers[server];
		}
		server_item->SetPictures(hdisk_t, hdisk_t);
		switch(hdef->type){
			case hdef_t::noneD:   hist_item->SetPictures(redx_t, redx_t); break;
			case hdef_t::oneD:    hist_item->SetPictures(h1_t, h1_t); break;
			case hdef_t::twoD:    hist_item->SetPictures(h2_t, h2_t); break;
			case hdef_t::threeD:  hist_item->SetPictures(h3_t, h3_t); break;
			case hdef_t::profile: hist_item->SetPictures(profile_t, profile_t); break;
			case hdef_t::macro:   hist_item->SetPictures(package_t, package_t); break;
		}
		if(!server_checkbox)server_item->Toggle();
		if(item_checked[hnamepath] != hist_item->IsChecked())hist_item->Toggle();

		server_items[hid_t(server,hnamepath)] = server_item;
		hist_items[hid_t(server,hnamepath)] = hist_item;
	}
	
	// Remember the this list so we can easily check for changes
	last_hids = hids;

	// Default to only showing histograms that are explicitly selected
	//DoSelectNone();
	
	// This is needed to force the list tree to draw its contents
	// without being clicked on. Without it (on at least one platform)
	// the window will simply appear blank until an invisible entry
	// is clicked on.
	listTree->ClearHighlighted();
}


//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_SelectHists::CreateGUI(void)
{
	TGMainFrame *fMainFrame984 = this;

	TGFont *ufont;         // will reflect user font changes
	//TGFont *ufont_bold;         // will reflect user font changes
	ufont = gClient->GetFont("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
	//ufont_bold = gClient->GetFont("-*-helvetica-bold-r-*-*-12-*-*-*-*-*-iso8859-1");

	TGGC   *uGC;           // will reflect user GC changes
	// graphics context changes
	GCValues_t valEntry689;
	valEntry689.fMask = kGCForeground | kGCBackground | kGCFillStyle | kGCFont | kGCGraphicsExposures;
	gClient->GetColorByName("#000000",valEntry689.fForeground);
	gClient->GetColorByName("#e0e0e0",valEntry689.fBackground);
	valEntry689.fFillStyle = kFillSolid;
	valEntry689.fFont = ufont->GetFontHandle();
	valEntry689.fGraphicsExposures = kFALSE;
	uGC = gClient->GetGC(&valEntry689, kTRUE);


   // composite frame
   TGCompositeFrame *fMainFrame800 = new TGCompositeFrame(fMainFrame984,300,390,kVerticalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame801 = new TGVerticalFrame(fMainFrame800,396,386,kVerticalFrame);

   // vertical frame
   TGHorizontalFrame *fHorizontalFrame802 = new TGHorizontalFrame(fVerticalFrame801,200,42,kHorizontalFrame);
   //TGHorizontalFrame *fHorizontalFrame802 = new TGHorizontalFrame(fVerticalFrame801,96,42,kHorizontalFrame);

   //TGVerticalFrame *fVerticalFrame802 = new TGVerticalFrame(fVerticalFrame801,96,42,kVerticalFrame);
   TGVerticalFrame *fVerticalFrame802 = new TGVerticalFrame(fHorizontalFrame802,96,42,kVerticalFrame);
   TGRadioButton *fRadioButton803 = new TGRadioButton(fVerticalFrame802,"View By Object");
   fRadioButton803->SetState(kButtonDown);
   //fRadioButton803->SetState(kButtonUp);
   fRadioButton803->SetTextJustify(36);
   fRadioButton803->SetMargins(0,0,0,0);
   fRadioButton803->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton803, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGRadioButton *fRadioButton804 = new TGRadioButton(fVerticalFrame802,"View By Server");
   //fRadioButton804->SetState(kButtonDown);
   fRadioButton804->SetState(kButtonUp);
   fRadioButton804->SetTextJustify(36);
   fRadioButton804->SetMargins(0,0,0,0);
   fRadioButton804->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton804, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fHorizontalFrame802->AddFrame(fVerticalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

	// Filter by type checkboxes
	TGVerticalFrame *fTypeFilters = new TGVerticalFrame(fHorizontalFrame802);
	bFilterTH1 = new TGCheckButton(fTypeFilters, "TH1");
	bFilterTH2 = new TGCheckButton(fTypeFilters, "TH2");
	bFilterTProfile = new TGCheckButton(fTypeFilters, "TProfile");
	bFilterMacro = new TGCheckButton(fTypeFilters, "Macros");
	bFilterTH1->SetOn();
	bFilterTH2->SetOn();
	bFilterTProfile->SetOn();
	bFilterMacro->SetOn();
	fTypeFilters->AddFrame(bFilterTH1, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fTypeFilters->AddFrame(bFilterTH2, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fTypeFilters->AddFrame(bFilterTProfile, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fTypeFilters->AddFrame(bFilterMacro, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontalFrame802->AddFrame(fTypeFilters, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));


   TGHorizontalFrame *fSelectAllNone = new TGHorizontalFrame(fVerticalFrame802);
   fVerticalFrame802->AddFrame(fSelectAllNone, new TGLayoutHints(kLHintsCenterX | kLHintsExpandX,2,2,2,2));
   TGTextButton *bSelectAll = new TGTextButton(fSelectAllNone, "Select All");
   TGTextButton *bSelectNone = new TGTextButton(fSelectAllNone, "Select None");
   fSelectAllNone->AddFrame(bSelectAll, new TGLayoutHints(kLHintsLeft,2,2,2,2));
   fSelectAllNone->AddFrame(bSelectNone, new TGLayoutHints(kLHintsRight,2,2,2,2));

   TGTextEntry *fTextEntry689 = new TGTextEntry(fHorizontalFrame802, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
   fTextEntry689->SetMaxLength(4096);
   fTextEntry689->SetAlignment(kTextLeft);
   fTextEntry689->SetText("");
   fTextEntry689->Resize(150,fTextEntry689->GetDefaultHeight());
   //fHorizontalFrame802->AddFrame(fTextEntry689, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kFitWidth,2,2,2,2));
   fHorizontalFrame802->AddFrame(fTextEntry689, new TGLayoutHints(kLHintsLeft | kLHintsCenterY | kLHintsExpandX | kFitWidth,2,2,2,2));

   TGTextButton *fTextButton685 = new TGTextButton(fHorizontalFrame802,"Filter");
   fTextButton685->SetTextJustify(36);
   fTextButton685->SetMargins(0,0,0,0);
   fTextButton685->SetWrapLength(-1);
   fTextButton685->Resize(97,22);
   //fHorizontalFrame802->AddFrame(fTextButton685, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fHorizontalFrame802->AddFrame(fTextButton685, new TGLayoutHints(kLHintsLeft | kLHintsCenterY,2,2,2,2));
   
   fVerticalFrame801->AddFrame(fHorizontalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kFitWidth,2,2,2,2));

   //fVerticalFrame801->AddFrame(fVerticalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fVerticalFrame801->AddFrame(fHorizontalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // canvas widget
   TGCanvas *fCanvas805 = new TGCanvas(fVerticalFrame801,384,200);

   // canvas viewport
   TGViewPort *fViewPort806 = fCanvas805->GetViewPort();

   // list tree
   TGListTree *fListTree815 = new TGListTree(fCanvas805,kHorizontalFrame);

   const TGPicture *popen;       //used for list tree items
   const TGPicture *pclose;      //used for list tree items

   //TGListTreeItem *item0 = fListTree815->AddItem(NULL,"Entry 1");
   popen = gClient->GetPicture("ofolder_t.xpm");
   pclose = gClient->GetPicture("folder_t.xpm");
   //item0->SetPictures(popen, pclose);
   //fListTree815->CloseItem(item0);

   fViewPort806->AddFrame(fListTree815,  new TGLayoutHints(kLHintsExpandY));
   fListTree815->SetLayoutManager(new TGHorizontalLayout(fListTree815));
   fListTree815->MapSubwindows();
   fCanvas805->SetContainer(fListTree815);
   fCanvas805->MapSubwindows();
   fVerticalFrame801->AddFrame(fCanvas805, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

   // horizontal frame
   //TGHorizontalFrame *fHorizontalFrame816 = new TGHorizontalFrame(fVerticalFrame801,392,32,kHorizontalFrame);
   TGHorizontalFrame *fHorizontalFrame816 = new TGHorizontalFrame(fVerticalFrame801,302,32,kHorizontalFrame);
   //fHorizontalFrame816->SetLayoutBroken(kTRUE);
   TGTextButton *fTextButton817 = new TGTextButton(fHorizontalFrame816,"Cancel");
   fTextButton817->SetTextJustify(36);
   fTextButton817->SetMargins(0,0,0,0);
   fTextButton817->SetWrapLength(-1);
   fTextButton817->Resize(56,22);
   fHorizontalFrame816->AddFrame(fTextButton817, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fTextButton817->MoveResize(2,2,56,22);
   TGTextButton *fTextButton818 = new TGTextButton(fHorizontalFrame816,"OK");
   fTextButton818->SetTextJustify(36);
   fTextButton818->SetMargins(0,0,0,0);
   fTextButton818->SetWrapLength(-1);
   //fTextButton818->Resize(55,22);
   fTextButton818->Resize(100,22);
   fHorizontalFrame816->AddFrame(fTextButton818, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
   //fHorizontalFrame816->AddFrame(fTextButton818, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fTextButton818->MoveResize(320,0,55,22);

   fVerticalFrame801->AddFrame(fHorizontalFrame816, new TGLayoutHints(kLHintsLeft | kLHintsTop | kFitWidth,2,2,2,2));

   fMainFrame800->AddFrame(fVerticalFrame801, new TGLayoutHints(kLHintsLeft | kLHintsTop | kFitWidth | kFitHeight | kLHintsExpandY,2,2,2,2));

   fMainFrame984->AddFrame(fMainFrame800, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight));
   //fMainFrame800->MoveResize(0,0,400,290);

   fMainFrame984->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame984->MapSubwindows();

   fMainFrame984->Resize(fMainFrame984->GetDefaultSize());
   fMainFrame984->MapWindow();
   //fMainFrame984->Resize(490,372);
	//==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &ok = fTextButton818;
	TGTextButton* &cancel = fTextButton817;
	TGTextButton* &filter_button = fTextButton685;
	this->canvas = fCanvas805;
	this->listTree = fListTree815;
	this->viewByObject = fRadioButton803;
	this->viewByServer = fRadioButton804;
	this->filterTextBox = fTextEntry689;
	
	ok->Connect("Clicked()","Dialog_SelectHists", this, "DoOK()");
	cancel->Connect("Clicked()","Dialog_SelectHists", this, "DoCancel()");
	this->listTree->Connect("Clicked(TGListTreeItem*, Int_t)","Dialog_SelectHists", this, "DoClickedEntry(TGListTreeItem*, Int_t)");
	this->listTree->Connect("DoubleClicked(TGListTreeItem*, Int_t)","Dialog_SelectHists", this, "DoSelectSingleHist(TGListTreeItem*, Int_t)");
	this->listTree->Connect("Checked(TObject*, Bool_t)","Dialog_SelectHists", this, "DoCheckedEntry(TObject*, Int_t)");
	this->viewByObject->Connect("Clicked()","Dialog_SelectHists", this, "DoSetViewByObject()");
	this->viewByServer->Connect("Clicked()","Dialog_SelectHists", this, "DoSetViewByServer()");
	//Connect("CloseWindow()", "Dialog_SelectHists", this, "DoCancel()");	
	filter_button->Connect("Clicked()","Dialog_SelectHists", this, "DoFilterHists()");
	bSelectAll->Connect("Clicked()","Dialog_SelectHists", this, "DoSelectAll()");
	bSelectNone->Connect("Clicked()","Dialog_SelectHists", this, "DoSelectNone()");

	folder_t = pclose;
	ofolder_t = popen;
	redx_t = gClient->GetPicture("sm_delete.xpm");
	package_t = gClient->GetPicture("package.xpm");
	h1_s = gClient->GetPicture("h1_s.xpm");
	h1_t = gClient->GetPicture("h1_t.xpm");
	h2_s = gClient->GetPicture("h2_s.xpm");
	h2_t = gClient->GetPicture("h2_t.xpm");
	h3_s = gClient->GetPicture("h3_s.xpm");
	h3_t = gClient->GetPicture("h3_t.xpm");
	profile_t = gClient->GetPicture("profile_t.xpm");
	pack_t = gClient->GetPicture("pack_t.xpm");
	hdisk_t = gClient->GetPicture("hdisk_t.xpm");
	checked_t = gClient->GetPicture("checked_t.xpm");
	unchecked_t = gClient->GetPicture("unchecked_t.xpm");
	
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

