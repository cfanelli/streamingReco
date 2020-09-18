// $Id$
//
//    File: RSTab.cc
// Created: Sat Sep 13 19:34:26 EDT 2014
// Creator: davidl (on Darwin harriet.local 13.3.0 i386)
//

#include "RSTab.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"
#include "Dialog_IndivHists.h"
#include "Dialog_SelectHists.h"
#include "Dialog_ShowMacro.h"
#include "Dialog_ShowGuidance.h"

#include "TGaxis.h"

#include <stdlib.h>
static int VERBOSE = 0;

//---------------------------------
// RSTab    (Constructor)
//---------------------------------
RSTab::RSTab(rs_mainframe *rsmf, string title)
{
	//- - - - - - - - - - - - - - - - - - - - - - - - - 
	// Build all GUI elements in a single tab
	
	const char *ROOTSPY_VERBOSE = getenv("ROOTSPY_VERBOSE");
	if(ROOTSPY_VERBOSE) VERBOSE = atoi(ROOTSPY_VERBOSE);
 	
	// Copy point to TGTab object to member variable
	fTab = rsmf->fMainTab;

	// Create new tab
	string title_padded = title + string(title.length()/2, ' ');
	TGCompositeFrame *f = fTab->AddTab(title_padded.c_str());
	this->title = title;
	
	// Hoizontal frame (controls to left, canvas to the right)
	fTabMain = new TGHorizontalFrame(f);
	f->AddFrame(fTabMain, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));
	
	//...... Controls ......
	fTabMainLeft = new TGVerticalFrame(fTabMain);
	fTabMain->AddFrame(fTabMainLeft, new TGLayoutHints(kLHintsLeft | kLHintsTop ,2,2,2,2));
	
	
	// Info. on what's currently displayed
	//TGVerticalFrame *fTabMainLeftInfo = new TGVerticalFrame(fTabMainLeft);	
	//fTabMainLeft->AddFrame(fTabMainLeftInfo, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));
	
	TGHorizontalFrame *fTabMainLeftInfoHistLabel = new TGHorizontalFrame(fTabMainLeft);
	fTabMainLeftInfoHistLabel->SetHeight(50);
	fTabMainLeft->AddFrame(fTabMainLeftInfoHistLabel, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX ,2,2,2,2));
	lType      = AddLabel(fTabMainLeftInfoHistLabel, "Histogram:" ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY );
	bViewMacro = AddButton(fTabMainLeftInfoHistLabel, "view", kLHintsTop| kLHintsExpandX | kLHintsExpandY);

	TGHorizontalFrame *fTabMainLeftGuidance = new TGHorizontalFrame(fTabMainLeft);
	fTabMainLeftGuidance->SetHeight(50);
	fTabMainLeft->AddFrame(fTabMainLeftGuidance, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX ,2,2,2,2));
	bViewGuidance = AddButton(fTabMainLeftGuidance, "Guidance", kLHintsTop| kLHintsExpandX | kLHintsExpandY);

	lHistogram = AddLabel(fTabMainLeft, string(25, '-'), kTextLeft, kLHintsLeft | kLHintsTop);
	AddSpacer(fTabMainLeft, 1, 5);
	AddLabel(fTabMainLeft, "Received:"  ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);
	lReceived  = AddLabel(fTabMainLeft, string(25, '-'), kTextLeft, kLHintsLeft | kLHintsExpandX);
	AddSpacer(fTabMainLeft, 1, 5);

	// Add some space between labels and controls
	AddSpacer(fTabMainLeft, 1, 10);
	
	// Previous, Next, and Refresh buttons
	TGHorizontalFrame *fTabMainLeftPrevNext = new TGHorizontalFrame(fTabMainLeft);
	TGPictureButton *bPrev = AddPictureButton(fTabMainLeftPrevNext, "GoBack.gif"   , "go to previous histogram");
	AddSpacer(fTabMainLeftPrevNext, 20, 1); // x-space between previous and next buttons
	TGPictureButton *bNext = AddPictureButton(fTabMainLeftPrevNext, "GoForward.gif", "go to next histogram");
	fTabMainLeft->AddFrame(fTabMainLeftPrevNext, new TGLayoutHints(kLHintsCenterX,2,2,2,2));
	TGPictureButton *bUpdate = AddPictureButton(fTabMainLeft, "ReloadPage.gif"   , "refresh current histogram", kLHintsCenterX);

	AddSpacer(fTabMainLeft, 1, 10); // y-space between prev/next and refresh

	// Display Type
	TGGroupFrame *fDisplayOptions = new TGGroupFrame(fTabMainLeft, "Display Options     ", kVerticalFrame);
	TGTextButton *bSelect = AddButton(fDisplayOptions, "Select", kLHintsExpandX);
	AddSpacer(fDisplayOptions, 1, 5);
	TGTextButton *bReset = AddButton(fDisplayOptions, "Reset", kLHintsExpandX);
	TGTextButton *bRestore = AddButton(fDisplayOptions, "Restore", kLHintsExpandX);
	fTabMainLeft->AddFrame(fDisplayOptions, new TGLayoutHints(kLHintsCenterX | kLHintsTop| kLHintsExpandX, 2,2,2,2));
	
	// Add some more space
	AddSpacer(fTabMainLeft, 1, 10);

	// Buttons at bottom left
	TGVerticalFrame *fTabMainLeftBottom = new TGVerticalFrame(fTabMainLeft);
	TGTextButton *bShowIndiv = AddButton(fTabMainLeftBottom, "Show Individual", kLHintsTop| kLHintsExpandX);
	TGTextButton *bSaveCanvas = AddButton(fTabMainLeftBottom, "Save Canvas", kLHintsTop| kLHintsExpandX);	
	fTabMainLeft->AddFrame(fTabMainLeftBottom, new TGLayoutHints(kLHintsCenterX | kLHintsTop| kLHintsExpandX, 2,2,2,2));

	// Add some more space
	AddSpacer(fTabMainLeft, 1, 10);
	
	AddLabel(fTabMainLeft, "Server:"    ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);
	lServer    = AddLabel(fTabMainLeft, string(25, '-'),kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);

	// Add some more space at bottom
	AddSpacer(fTabMainLeft, 1, 10, kLHintsCenterX | kLHintsCenterY | kLHintsExpandY);
	
	//...... Embedded canvas ......
	char cname[256];
	static int cntr = 1;
	sprintf(cname, "canvas%02d", cntr++);
	TRootEmbeddedCanvas *embeddedcanvas = new TRootEmbeddedCanvas(0,fTabMain,900,600);	
	Int_t id = embeddedcanvas->GetCanvasWindowId();
	canvas = new TCanvas(cname, 10, 10, id);
	canvas->SetTicks();
	canvas->SetDoubleBuffer();           // for smoother updates
	embeddedcanvas->AdoptCanvas(canvas);
	fTabMain->AddFrame(embeddedcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

	rsmf->rstabs.push_back(this);
	rsmf->current_tab = this;
	fTab->SetTab(rsmf->rstabs.size() - 1);
	
	// ----------- Connect GUI elements to methods -----------
	bNext->Connect("Clicked()","RSTab", this, "DoNext()");
	bPrev->Connect("Clicked()","RSTab", this, "DoPrev()");
	bShowIndiv->Connect("Clicked()","RSTab", this, "DoShowIndiv()");
	bSaveCanvas->Connect("Clicked()","RSTab", this, "DoSaveCanvas()");
	bUpdate->Connect("Clicked()","RSTab", this, "DoUpdateWithFollowUp()");
	bSelect->Connect("Clicked()","RSTab", this, "DoSelectHists()");
	bReset->Connect("Clicked()","RSTab", this, "DoReset()");
	bRestore->Connect("Clicked()","RSTab", this, "DoRestore()");
	bViewMacro->Connect("Clicked()","RSTab", this, "DoViewMacro()");
	bViewGuidance->Connect("Clicked()","RSTab", this, "DoViewGuidance()");
	
	// Set some defaults
	config = title;
	currently_displayed = 0;
	currently_displayed_modified = 0.0;
	last_update = 0.0;
	last_request_sent = -10.0;
	
}

//---------------------------------
// ~RSTab    (Destructor)
//---------------------------------
RSTab::~RSTab()
{

}

//-------------------
// SetTo
//-------------------
void RSTab::SetTo(string hnamepath)
{
	int idx = 0;
	list<string>::iterator it = hnamepaths.begin();
	for(; it!=hnamepaths.end(); it++, idx++){
		if(*it == hnamepath){
			if(idx != currently_displayed){
				currently_displayed = idx;
				TTimer::SingleShot(1, "RSTab", this, "DoUpdate()"); // have canvas updated quickly
				TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
				return;
			}
		}
	}

/**
	// If this wasn't in the current list, then add it to our list!
	hnamepaths.push_back(hnamepath);
	currently_displayed = 0;
	TTimer::SingleShot(1, "RSTab", this, "DoUpdate()"); // have canvas updated quickly
	TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
**/
	return;
}

//-------------------
// AddLabel
//-------------------
TGLabel* RSTab::AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints)
{
	TGLabel *lab = new TGLabel(frame, text.c_str());
	lab->SetTextJustify(mode);
	lab->SetMargins(0,0,0,0);
	lab->SetWrapLength(-1);
	frame->AddFrame(lab, new TGLayoutHints(hints,2,2,2,2));

	return lab;
}

//-------------------
// AddButton
//-------------------
TGTextButton* RSTab::AddButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGTextButton *b = new TGTextButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	b->Resize(100,22);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddCheckButton
//-------------------
TGCheckButton* RSTab::AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGCheckButton *b = new TGCheckButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddPictureButton
//-------------------
TGPictureButton* RSTab::AddPictureButton(TGCompositeFrame* frame, string picture, string tooltip, ULong_t hints)
{
	TGPictureButton *b = new TGPictureButton(frame, gClient->GetPicture(picture.c_str()));
	if(tooltip.length()>0) b->SetToolTipText(tooltip.c_str());
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddSpacer
//-------------------
TGFrame* RSTab::AddSpacer(TGCompositeFrame* frame, UInt_t w, UInt_t h, ULong_t hints)
{
	/// Add some empty space. Usually, you'll only want to set w or h to
	/// reserve width or height pixels and set the other to "1".

	TGFrame *f =  new TGFrame(frame, w, h);
	frame->AddFrame(f, new TGLayoutHints(hints ,2,2,2,2));
	
	return f;
}

//-------------------
// DoNext
//-------------------
void RSTab::DoNext(void)
{
	currently_displayed++;
	if((uint32_t)currently_displayed >= hnamepaths.size()) currently_displayed = 0;

	if(VERBOSE>1){
		_DBG_ << " Advanced to next item:" << endl;
		list<string>::iterator it = hnamepaths.begin();
		for(int i=0; it!=hnamepaths.end(); it++, i++){
			string prefix = i==currently_displayed ? "   ---> ":"        ";
			cout << prefix <<*it<<endl;
		}
	}

	DoUpdateWithFollowUp();
}

//-------------------
// DoPrev
//-------------------
void RSTab::DoPrev(void)
{
	currently_displayed--;
	if(currently_displayed < 0){
		if(hnamepaths.empty()){
			currently_displayed = 0;
		}else{
			currently_displayed = hnamepaths.size() -1;
		}
	}
	DoUpdateWithFollowUp();
}

//-------------------
// DoShowIndiv
//-------------------
void RSTab::DoShowIndiv(void)
{
	new Dialog_IndivHists(currently_displayed_hnamepath, gClient->GetRoot(), 10, 10);
}

//----------
// DoSaveCanvas
//----------
//added by JustinBarry 06.14.10
//Save the current canvas as an image file.
void RSTab::DoSaveCanvas(void)
{
	TGFileInfo* fileinfo = new TGFileInfo();
	new TGFileDialog(gClient->GetRoot(), gClient->GetRoot(), kFDSave, fileinfo);
	canvas->SaveAs(fileinfo->fFilename, "");

	delete fileinfo;
}

//----------
// DoUpdate
//----------
void RSTab::DoUpdate(void)
{
	/// This is called by the rs_mainframe::DoTimer about
	/// every 250 milliseconds to refresh the canvas as needed.
	/// It is also called if the histogram/macro has changed
	/// and DoUpdateWithFollowUp gets called which schedules
	/// this to be called a few extra times.

	double now = RS_CMSG->GetTime();

	// If we have no histograms in our list to display, clear the canvas
	// and return immediately. Only clear the canvas once every 3 seconds
	if(hnamepaths.empty()){
		if( (now-currently_displayed_modified) > 3.0 ){
			canvas->cd();
			canvas->Clear();
			canvas->Update();
			currently_displayed_modified = now;
		}
		
		return;
	}

	// If the currently displayed histogram index is out of range, force it into range
	if((uint32_t)currently_displayed >= hnamepaths.size()) currently_displayed = 0;

	// Get hnamepath of currently displayed histo/macro
	list<string>::iterator h_it = hnamepaths.begin();
	advance(h_it, currently_displayed);
	string &hnamepath = *h_it;

	// Send request if at least 0.5 seconds has passed sent we last sent a request
	// or the thing we are displaying has changed
	if( ((now-last_request_sent) > 0.5) || (hnamepath != last_request_hnamepath) ){
		int Nrequests = RS_INFO->RequestHistograms(hnamepath);
		last_request_sent = now;
		last_request_hnamepath = hnamepath;
		//cout << "Sent " << Nrequests << " requests sent for " << hnamepath << endl;
		Nrequests++; // avoid compiler warnings if above line is commented out
	}

	// Get Pointer to histogram to display the type of histogram (possibly a macro)
	hdef_t::histdimension_t type = hdef_t::noneD;
	double sum_hist_modified = 0.0;
	string servers_str;
	TH1* sum_hist = RS_INFO->GetSumHist(hnamepath, &type, &sum_hist_modified, &servers_str);
	string servers_str_padded = servers_str;
	
	// Update labels
	string hnamepath_padded = hnamepath;
	if(hnamepath.length()<80) hnamepath_padded += string(80-hnamepath.length(), ' ');
	time_t t = (unsigned long)floor(sum_hist_modified+rs_cmsg::start_time); // seconds since 1970
	string tstr = ctime(&t);
	tstr[tstr.length()-1] = 0; // chop off last "\n"
	string tstr_padded = string(tstr) + string(6, ' ');
	lServer->SetText(TString(servers_str_padded));
	lReceived->SetText(TString(tstr_padded));
	lReceived->SetWidth(500);
	lHistogram->SetText(TString(hnamepath_padded));
	lHistogram->SetWidth(500);
	lType->SetText(type==hdef_t::macro ? "Macro:        ":"Histogram:   ");
	bViewMacro->SetEnabled(type==hdef_t::macro);
	//lReceived->Resize();
	//fTabMainLeft->Resize();

	// Draw the histogram/macro
	map<string,hdef_t>::iterator hdef_it;
	switch(type){
		case hdef_t::oneD:
		case hdef_t::twoD:
		case hdef_t::threeD:
		case hdef_t::profile:
			if(sum_hist){
				if(sum_hist_modified > currently_displayed_modified){

					// Draw histogram
					pthread_rwlock_wrlock(ROOT_MUTEX);
					canvas->cd();
					canvas->Clear();
					if(type == hdef_t::twoD)
						sum_hist->Draw("COLZ");
					else
						sum_hist->Draw();
					DoOverlay();
					sum_hist->UseCurrentStyle();   // updates in case the style paramteres change
					canvas->Update();
					currently_displayed_hnamepath = hnamepath;
					pthread_rwlock_unlock(ROOT_MUTEX);				
				}
			}
			break;
		case hdef_t::macro:
			canvas->cd();
			canvas->Clear();
			currently_displayed_modified = now;
			RS_INFO->Lock();
			hdef_it = RS_INFO->histdefs.find(hnamepath);
			if(hdef_it != RS_INFO->histdefs.end()) RSMF->DrawMacro(canvas, hdef_it->second);
			RS_INFO->Unlock();
			canvas->Update();
			currently_displayed_hnamepath = hnamepath;
			RequestUpdatedMacroHists((hdef_it->second).macro_hnamepaths);
			break;
		default:
			//cout << "histogram/macro not available (is producer program running?)" << endl;
			;
		
	}

	last_update = now;
}


//----------
// DoOverlay
// NOTE: Currently only works for summed histograms
//----------
void RSTab::DoOverlay(void)
{
	//cerr << "In DoOverlay()..." << endl;
	// reset some display defaults that might get changed
	gStyle->SetStatX(0.95);
	gPad->SetTicks();

	// check to see if we should be overlaying archived histograms
	bool overlay_enabled = (RSMF->bShowOverlays->GetState()==kButtonDown);
	if(!overlay_enabled)
		return;
	if(RSMF->archive_file == NULL)
		return;
	
	// make sure that there is an active pad
	if(!gPad)
		return;

	//cerr << "Searching gPad..." << endl;

	// look for the histograms in the current pad
	TIter nextObj(gPad->GetListOfPrimitives());
	TObject *obj;
	TH1 *horig = NULL;
	while ((obj = nextObj())) {
		//obj->Draw(next.GetOption());
		TNamed *namedObj = dynamic_cast<TNamed*>(obj);
		if(namedObj != NULL) {
			//cerr << namedObj->GetName() << endl;
			horig = static_cast<TH1*>(namedObj);
			break;
		}

		// check to make sure this is actually a histogram
		//h = dynamic_cast<TH1*>(h);
		//if(h!=NULL)
		//	break;
	}
	
	if(horig == NULL) {
		cerr << "Could not find histogram info in gPad!" << endl;
		return;
	}	
	
	// find the histogram in our list of histogram defintions
	// so that we can find its full path
	RS_INFO->Lock();
	
	string hnamepath = "";
	hdef_t::histdimension_t type = hdef_t::noneD;
	hdisplay_info_t display_info;
	for(map<string,hdef_t>::iterator hdef_itr = RS_INFO->histdefs.begin();
	    hdef_itr != RS_INFO->histdefs.end(); hdef_itr++) {
		if( horig == hdef_itr->second.sum_hist ) {
			hnamepath = hdef_itr->first;
			type = hdef_itr->second.type;
			display_info = hdef_itr->second.display_info;
			break;
		}
	}

	RS_INFO->Unlock();
	
	if(hnamepath == "") {
		cerr << "Could not find histogram information in hdef list!" << endl;
		return;
	} 
	//else {
	//	cerr << "Found info for histogram = " << hnamepath << endl;
	//}
	
	
	// overlaying only makes sense for 1D histograms
	if(type != hdef_t::oneD)
		return;

	TH1 *h = static_cast<TH1*>(horig->Clone());  // memory leak??
	
	// try to find a corresponding one in the archived file
	// we assume that the archived file has the same hnamepath structure
	// as the histograms in memory
	TH1 *h_over_orig = static_cast<TH1*>(RSMF->archive_file->Get(hnamepath.c_str()));
	TH1 *h_over = static_cast<TH1*>(h_over_orig->Clone());   // memory leak??
        if(h_over == NULL) { 
		cerr << "Could not find corresponding archived histogram!" << endl;
		return;
	}

	// set axis ranges so that we can sensibly overlay the histograms
	double hist_ymax = 1.1*h->GetMaximum();
	double hist_ymin = h->GetMinimum();
	// make sure we at least go down to zero, so that we are displaying the whole
	// distribution - assumes we are just looking at frequency histograms
	if(hist_ymin > 0)
		hist_ymin = 0;
	// add in a fix for logarithmic y axes
	if(display_info.use_logy && hist_ymin == 0)
		hist_ymin = 0.1;
	
	h->GetYaxis()->SetRangeUser(hist_ymin, hist_ymax); 
	h->SetLineWidth(2);
	
        // format overlay histogram and extract parameters
	gStyle->SetStatX(0.85); 
	gPad->SetTicky(0);
        h_over->SetStats(0);
        h_over->SetFillColor(5);
        h_over->SetLineWidth(0);

	// set overlay axis ranges too
        double overlay_ymin = h_over->GetMinimum(); 
        double overlay_ymax = 1.1*h_over->GetMaximum();
	if(overlay_ymin > 0)
		overlay_ymin = 0;
	// add in a fix for logarithmic y axes
	if(display_info.use_logy && overlay_ymin == 0)
		overlay_ymin = 0.1;
	

        // Draw the archived histogram underneath the main histogram
	// This is needed since we are drawing the archived histogram as shaded
        //h_over->Scale( h->Integral() / h_over->Integral() );
        float scale = hist_ymax/overlay_ymax;
        h_over->Scale( scale );
	h_over->GetYaxis()->SetRangeUser(hist_ymin, hist_ymax); 
        h_over->Draw();

	// Draw main histogram over the top
	h->Draw("SAME");

	// Add in an axis for the overlaid histogram on the right side of the plot
        TGaxis *overlay_yaxis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),
				   gPad->GetUxmax(),gPad->GetUymax(),
				   overlay_ymin,overlay_ymax,505,"+L");
        overlay_yaxis->SetLabelFont( h->GetLabelFont() );                                                                                                                                          
        overlay_yaxis->SetLabelSize( h->GetLabelSize() );                                                                                                                                          
        overlay_yaxis->Draw();                                                                                                                                                                        

}

//----------
// DoUpdateWithFollowUp
//----------
void RSTab::DoUpdateWithFollowUp(void)
{
	/// This is generally called when the selection of histogram has changed
	/// and needs to be requested and then updated on the screen. Since there
	/// is some delay between the requests going out and the results coming
	/// back, DoUpdate can't immediately update the screen in one call so multiple
	/// calls to it are needed with some delay in between. This will call it 
	/// once immediately(-ish), but then schedule two other calls to it for 250ms
	/// and 1s later. That should give enough time for a response to come back.

	// NOTE: Calling DoUpdate() directly here was resulting in se.g faults on
	// OS X in the rs_cmg::RegisterHistogram() method (??!!) Register for ROOT
	// to call it in 10ms 

	//TTimer::SingleShot(10, "RSTab", this, "DoUpdate()");
	TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
	//TTimer::SingleShot(1000, "RSTab", this, "DoUpdate()");
}

//----------
// DoUpdateCanvas
//----------
void RSTab::DoUpdateCanvas(void)
{
	/// call canvas->Update();
	canvas->Update();
}

//----------
// DoSelectHists
//----------
void RSTab::DoSelectHists(void)
{
	new Dialog_SelectHists(this, gClient->GetRoot(), 10, 10);
}

//----------
// DoReset
//----------
void RSTab::DoReset(void)
{
	// Originally, this called GetMacroHists()
	// to try and automatically figure out which
	// histograms to reset so that only the currently
	// displayed ones were saved. User feedback though
	// indicated that their expectation was for all
	// histograms to be reset. Thus, we get a list 
	// of all displayed hnamepaths for all tabs. Note 
	// that any hnamepaths that are specified by macros
	// are also included in this list.
	
	set<string> hnamepaths;
	RSMF->GetAllDisplayedHNamepaths(hnamepaths);
	for( string hnamepath : hnamepaths ) RS_INFO->ResetHisto(hnamepath);
	
	DoUpdate();
}

//----------
// DoRestore
//----------
void RSTab::DoRestore(void)
{
	// See comments for DoReset()

	set<string> hnamepaths;
	RSMF->GetAllDisplayedHNamepaths(hnamepaths);
	for( string hnamepath : hnamepaths ) RS_INFO->RestoreHisto(hnamepath);
	
	DoUpdate();
}

//----------
// DoViewMacro
//----------
void RSTab::DoViewMacro(void)
{
	// If the currently displayed histogram index is out of range, do nothing
	if((uint32_t)currently_displayed >= hnamepaths.size()) return;

	// Get hnamepath of currently displayed histo/macro
	list<string>::iterator h_it = hnamepaths.begin();
	advance(h_it, currently_displayed);
	string hnamepath = *h_it;
	
//	RS_INFO->Lock();
//	RS_INFO->Unlock();
	
	// Get hdef_t of currently displayed macro. Ignore if zero servers
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
    if(hdef_iter==RS_INFO->histdefs.end()) return;
	if(hdef_iter->second.hists.empty()) return;
	
	// Get pointer to macro data
	hinfo_t &hinfo = hdef_iter->second.hists.begin()->second;
	string &macro_str = hinfo.macroString;

	string title = "Macro : " + hnamepath;
	
	new Dialog_ShowMacro(title, macro_str);

}

//----------
// DoViewGuidance
//----------
void RSTab::DoViewGuidance(void)
{
	// If the currently displayed histogram index is out of range, do nothing
	if((uint32_t)currently_displayed >= hnamepaths.size()) return;

	// Get hnamepath of currently displayed histo/macro
	list<string>::iterator h_it = hnamepaths.begin();
	advance(h_it, currently_displayed);
	string hnamepath = *h_it;
	
	// Get hdef_t of currently displayed macro. Ignore if zero servers
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
	if(hdef_iter==RS_INFO->histdefs.end()) return;
	if(hdef_iter->second.hists.empty()) return;

	string title = "Guidance : " + hnamepath;	
	new Dialog_ShowGuidance(title, hdef_iter->second.macro_guidance);

}

//----------
// GetMacroHists
//----------
void RSTab::GetMacroHists(set<string> &hnamepaths)
{
	/// Look through all of the pads and find the hnamepath for all TH1
	/// objects associated with them. This allows us to figure out which 
	/// histograms were used in a macro. This only looks for hists existing
	/// in the sum hist directory ("RootSpy:/rootspy/").
	///
	/// Also, look for the macro string and identify any hnamepaths in
	/// it and add those to the list.

	pthread_rwlock_rdlock(ROOT_MUTEX);

	// Look for primatives in currently drawn pads
	for(int ipad=0; ipad<100; ipad++){
		TVirtualPad *pad = canvas->GetPad(ipad);
		if(!pad) break;
		
		pad->cd();
		TIter next(pad->GetListOfPrimitives());
		TObject *obj;
		while ( (obj = next()) ){
			TH1 *h = dynamic_cast<TH1*>(obj);
			if(h != NULL){
				TDirectory *dir = h->GetDirectory();
				if(!dir) continue;
				string path = dir->GetPath();
				string prefix = "RootSpy:/rootspy/";
				if(path.find(prefix) == 0){
					string hnamepath = path.substr(prefix.length()-1) + "/" + h->GetName();
					hnamepaths.insert(hnamepath);
				}
			}			
		}
	}
	
	pthread_rwlock_unlock(ROOT_MUTEX);
}

//----------
// RequestUpdatedMacroHists
//----------
void RSTab::RequestUpdatedMacroHists(set<string> &hnamepaths)
{
	/// Request updated histograms based on what is currently displayed in
	/// the canvas. This used when a macro has been used to draw on the canvas
	/// and we need to get updated histograms from the servers. It looks through 
	/// all of the pads and finds all TH1 objects associated with them. If the
	/// TH1 is located in the sum hist directory ("RootSpy:/rootspy/") then a request
	/// is sent out to all servers with the appropriate namepath.

	set<string> myhnamepaths = hnamepaths; // initialize with hnamepaths found in macro string
	GetMacroHists(myhnamepaths); // Add hnamepaths for histograms found on canvas
	set<string>::iterator iter = myhnamepaths.begin();
	for(; iter!=myhnamepaths.end(); iter++){
		RS_INFO->RequestHistograms(*iter);
	}
}


