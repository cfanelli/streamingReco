// $Id: Dialog_NewReference.cc 15950 2017-01-24 09:42:00Z davidl $
//
//    File: Dialog_NewReference.cc
// Creator: sdobbs
//

#include <sys/stat.h>

#include <algorithm>
#include <iostream>
#include <sstream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_NewReference.h"
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
// Dialog_NewReference    (Constructor)
//---------------------------------
Dialog_NewReference::Dialog_NewReference(const TGWindow *p, UInt_t w, UInt_t h, string hnamepath, string tmp_fname):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	/// Presents a dialog to the user asking them to confirm setting of
	/// the new reference plot. The hnamepath and the filename of a temporary
	/// file that will become the new reference are passed in. Prior to
	/// instantiation, the canvas will have been saved (from rs_mainframe::DoMakeReferencePlot).
	/// If confirmed, then the old reference (if any) will be retired to the 
	/// archive directory and the temporary file installed as the new
	/// reference. Also, if any e-mail addresses were specified in the macro
	/// used to draw the reference, an e-mail is sent to them notifying them
	/// of the change.
	
	this->hnamepath = hnamepath;
	this->tmp_fname = tmp_fname;

	fname = Dialog_ReferencePlot::MakeReferenceFilename(hnamepath);
	bool old_ref_exists = (access(fname.c_str(), F_OK) != -1);

	// --------------- Create the GUI 

	// Main vertical frame
	TGVerticalFrame *fMain = new TGVerticalFrame(this);
	this->AddFrame(fMain, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));

	// Top message
	stringstream mess;
	mess << "You are about to install a new reference for the plot " << hnamepath << endl;
	if(old_ref_exists){
		mess << "This will replace the old reference shown below.";
	}else{
		mess << "A current reference plot for this does not exist.";
	}
	AddLabel(fMain, mess.str(), kTextLeft, kLHintsLeft| kLHintsTop| kLHintsExpandX);

	// Central content frame
	TGHorizontalFrame *fMainCenter = new TGHorizontalFrame(fMain);
	fMain->AddFrame(fMainCenter, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
	
	// Old reference (if it exists)
	if(old_ref_exists){
	
		// Modification time of old reference file
		struct stat mystat;
		stat(fname.c_str(), &mystat);
		auto ftime = localtime(&mystat.st_mtime);
		char tmbuf[256];
		strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", ftime);
		image_mod_time = tmbuf;
		string image_mod_time_padded = image_mod_time;
		image_mod_time_padded += string(image_mod_time.length()/2, ' ');

		// Vertical frame
		TGVerticalFrame *fOldRef = new TGVerticalFrame(fMainCenter);
		fMainCenter->AddFrame(fOldRef, new TGLayoutHints(kLHintsLeft | kLHintsTop| kLHintsExpandX,2,2,2,2));

		// Labels above canvas
		AddLabel(fOldRef, "------- OLD REFERENCE -----      " , kTextRight, kLHintsRight| kLHintsExpandX); 
		AddLabel(fOldRef, fname + string(fname.length()/2, ' '), kTextRight, kLHintsRight| kLHintsExpandX);
		AddLabel(fOldRef, "Modified: " + image_mod_time_padded, kTextRight, kLHintsRight| kLHintsExpandX);

		// Canvas for old reference
		TRootEmbeddedCanvas *embeddedcanvas = new TRootEmbeddedCanvas(0,fOldRef,600,400);	
		Int_t id = embeddedcanvas->GetCanvasWindowId();
		TCanvas *canvas = new TCanvas("oldReferencePlotCanvas", 10, 10, id);
		canvas->SetTicks();
		canvas->SetDoubleBuffer();           // for smoother updates
		canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
		embeddedcanvas->AdoptCanvas(canvas);
		fOldRef->AddFrame(embeddedcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

		// Read in old reference image and draw to canvas
		TImage *timage = TImage::Create();
		timage->ReadImage(fname.c_str());
		_DBG_<<"Loading: " << fname << endl;
		timage->Scale(canvas->GetWw(), canvas->GetWh());
		timage->Draw();
		canvas->Update();
	}
	
	// New reference is always drawn. Place in brackets though to give
	// private scope that matches above if() statement for old reference
	{
	
		// Modification time of old reference file
		struct stat mystat;
		stat(tmp_fname.c_str(), &mystat);
		auto ftime = localtime(&mystat.st_mtime);
		char tmbuf[256];
		strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", ftime);
		string image_mod_time = tmbuf;
		string image_mod_time_padded = image_mod_time;
		image_mod_time_padded += string(image_mod_time.length()/2, ' ');

		// Vertical frame
		TGVerticalFrame *fNewRef = new TGVerticalFrame(fMainCenter);
		fMainCenter->AddFrame(fNewRef, new TGLayoutHints(kLHintsLeft | kLHintsTop| kLHintsExpandX,2,2,2,2));

		// Labels above canvas
		AddLabel(fNewRef, "------- NEW REFERENCE -----      " , kTextRight, kLHintsRight| kLHintsExpandX); 
		AddLabel(fNewRef, tmp_fname + string(tmp_fname.length()/2, ' '), kTextRight, kLHintsRight| kLHintsExpandX);
		AddLabel(fNewRef, "Modified: " + image_mod_time_padded, kTextRight, kLHintsRight| kLHintsExpandX);

		// Canvas for old reference
		TRootEmbeddedCanvas *embeddedcanvas = new TRootEmbeddedCanvas(0,fNewRef,600,400);	
		Int_t id = embeddedcanvas->GetCanvasWindowId();
		TCanvas *canvas = new TCanvas("newReferencePlotCanvas", 10, 10, id);
		canvas->SetTicks();
		canvas->SetDoubleBuffer();           // for smoother updates
		canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
		embeddedcanvas->AdoptCanvas(canvas);
		fNewRef->AddFrame(embeddedcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

		// Read in old reference image and draw to canvas
		TImage *timage = TImage::Create();
		timage->ReadImage(tmp_fname.c_str());
		_DBG_<<"Loading: " << tmp_fname << endl;
		timage->Scale(canvas->GetWw(), canvas->GetWh());
		timage->Draw();
		canvas->Update();
	}

	// Write message regarding notification e-mails
	auto it = RS_INFO->histdefs.find(hnamepath);
	if(it != RS_INFO->histdefs.end()) emails = it->second.macro_emails;
	
	stringstream email_mess;
	if(emails.empty()){
		email_mess << "n.b. no email addresses specified for this macro so no notification will be sent                         ";
	}else{
		email_mess << "n.b. a notification will be sent to the following e-mail addresses:          " << endl;
		for(string s : emails) email_mess << endl << s;
	}
	AddLabel(fMain, email_mess.str() , kTextRight, kLHintsRight| kLHintsExpandX);
	
	// Checkbox indicating agreement
	cbagree = AddCheckButton(fMain, "I AGREE to the above stated terms and conditions (i.e. \"yeah go ahead and do it\")", kLHintsRight | kLHintsTop | kLHintsExpandX);
	
	// Bottom frame
	TGHorizontalFrame *fMainBot = new TGHorizontalFrame(fMain);
	fMain->AddFrame(fMainBot, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
	TGTextButton *bCancel = AddButton(fMainBot, "Cancel  ", kLHintsLeft);
	bCancel->Connect("Clicked()","Dialog_NewReference", this, "DoCancel()");
	TGTextButton *bOK = AddButton(fMainBot, "OK  ", kLHintsRight);
	bOK->Connect("Clicked()","Dialog_NewReference", this, "DoOK()");
	
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();	
}

//---------------------------------
// ~Dialog_NewReference    (Destructor)
//---------------------------------
Dialog_NewReference::~Dialog_NewReference()
{

}

//---------------------------------
// CloseWindow
//---------------------------------
void Dialog_NewReference::CloseWindow(void)
{
	RSMF->dialog_newreference = NULL;
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
}

//-------------------
// AddLabel
//-------------------
TGLabel* Dialog_NewReference::AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints)
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
TGTextButton* Dialog_NewReference::AddButton(TGCompositeFrame* frame, string text, ULong_t hints)
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
TGCheckButton* Dialog_NewReference::AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGCheckButton *b = new TGCheckButton(frame, text.c_str());
	b->SetTextJustify(kTextRight);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//---------------------------------
// DoCancel
//---------------------------------
void Dialog_NewReference::DoCancel(void)
{
	CloseWindow();
}

//---------------------------------
// DoOK
//---------------------------------
void Dialog_NewReference::DoOK(void)
{
	if( ! cbagree->IsOn() ){
		cerr << "YOU MUST CHECK THE I AGREE BOX OR CANCEL!!" << endl;
		return;
	}

	bool old_ref_exists = (access(fname.c_str(), F_OK) != -1);

	// Generate archive file name
	string archivename = "";
	if(old_ref_exists){
	
		// Get archive directory name and make sure it exists
		string dirname = Dialog_ReferencePlot::GetReferenceArchiveDir();
		mkdir(dirname.c_str(), 0777); // make directory if it doesn't exist

		// Generate filename based on current time
		time_t now = time(NULL);
		auto nowlt = localtime(&now);
		char tmbuf[256];
		strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d_%H:%M:%S", nowlt);
		string time_prefix(tmbuf);
		
		// Strip path off of image filename
		size_t pos = fname.find_last_of('/');
		if(pos != string::npos){
			archivename = dirname + "/" + time_prefix + fname.substr(pos+1);
		}
	}

	// Archive the old reference
	if(archivename != ""){
		cout << "Archiving old reference:" << endl;
		cout << "    " << fname << " -> " << archivename << endl;
		rename(fname.c_str(), archivename.c_str());
	}
	
	// Rename temporary filename to permanent one
	rename(tmp_fname.c_str(), fname.c_str()); 
	cout<<"Dialog_NewReference DoOK 1"<<endl;fflush(stdout);


	/*
	// Create command to send e-mail (excluding the e-mail address)
	stringstream cmd;
	cmd<<"(echo \"Reference histogram changed for "<<hnamepath<<":\"; ";
	cmd<<"echo \" \"; echo \" \"; ";
	cmd<<"echo \"This e-mail auto-generated from RootSpy. If you do not\"; ";
	cmd<<"echo \"wish to receive these notifications then please modify\"; ";
	cmd<<"echo \"the appropriate macro or contact davidl@jlab.org .\"; ";
	cmd<<"echo \" \"; echo \" \"; ";
	
	// Add host and user
	const char *host = getenv("HOST");
	const char *user = getenv("USER");
	cmd<<"echo \"Host: " << (host ? host:"unknown") << "\"; ";
	cmd<<"echo \"User: " << (user ? user:"unknown") << "\"; ";
	cmd<<"echo \" \"; echo \" \"; ";
	cout<<"Dialog_NewReference DoOK 2"<<endl;fflush(stdout);
	// Add current run if non-zero
	if(RSMF->epics_run_number > 0){
		cmd<<"echo \"Current run number from EPICS: " << RSMF->epics_run_number << "\"; ";
		cmd<<"echo \"(n.b. this may NOT actually be the run from which the new reference was dervied!)\"; ";
		cmd<<"echo \" \"; echo \" \"; ";
	}
	
	if(old_ref_exists){
		cmd<<"echo \"PREVIOUS REFERENCE PLOT from "<<image_mod_time<<"\"; ";
		cmd<<"echo \"(archived to: " << archivename << ")\"; ";
		//cmd<<"uuencode --base64 "<<fname<<" previous.png; ";
	}
	cout<<"Dialog_NewReference DoOK 3"<<endl;fflush(stdout);
	cmd<<"echo \" \"; echo \" \"; ";
	cmd<<"echo \"NEW REFERENCE PLOT:\"; ";
	cmd<<"echo \"(installed as: " << fname << ")\"; ";
	//cmd<<"uuencode "<<tmp_fname<<" new.png; ";

	cmd<<") | mail -s \"New Reference Plot for " << hnamepath << "\" ";
	if(old_ref_exists) cmd << " -a " << archivename;
	cmd << " -a " << fname;
	
	cout<<"Dialog_NewReference DoOK 4"<<endl;fflush(stdout);
	
	// Make list of all addresses
	stringstream sstr;
	for(auto s : emails) sstr << s << ",";
	string addresses(sstr.str());
	addresses.pop_back();
	
	// Make and execute full shell command
	stringstream my_cmd;
	my_cmd << cmd.str() << addresses;
	cout << my_cmd.str() << endl;
	
	cout<<"Dialog_NewReference DoOK 5"<<endl;fflush(stdout);

#if __APPLE__
	_DBG_ << "mail command not run for Mac OS X since this will almost certainly" << endl;
	_DBG_ << "just put this on a queue where no mail agent will ever pick it up and" << endl;
	_DBG_ << "send it. For all other OSes (e.g. Linux) it will attempt to mail." << endl;
#else
	int res = system(my_cmd.str().c_str());
	if(res!=0) cerr << "Error executing \""<<my_cmd.str()<<"\"" << endl;
#endif
	*/
// 	// Loop over e-mail addresses, sending the message to each one
// 	for(auto s : emails){
// 		stringstream my_cmd;
// 		my_cmd<<cmd.str()<<s;
// 		cout<<my_cmd.str()<<endl;
// 
// 	}

	CloseWindow();
}

// //---------------------------------
// // Base64Encode
// //---------------------------------
// void Dialog_NewReference::Base64Encode(string filename, stringstream &ss)
// {
// 
// 	// Read in input file
// 	ifstream ifs(filename.c_str());
// 	if(!ifs.is_open()){
// 		_DBG_<<"Unable to open file \""<< filename << "\" for base64 encoding!" << endl;
// 		return;
// 	}
// 	ifs.seekg(0, ifs.end);
// 	size_t fsize = ifs.tellg();
// 	ifs.seekg(0, ifs.beg);
// 	
// 	char *buff = new char[fsize];
// 	ifs.read(buff, fsize);
// 
// 	char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
//                              'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
//                              'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
//                              'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
//                              'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
//                              'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
//                              'w', 'x', 'y', 'z', '0', '1', '2', '3',
//                              '4', '5', '6', '7', '8', '9', '+', '/'};
// 
// 	for (int i = 0, j = 0; i < fsize;) {
// 
// 		uint32_t octet_a = i < fsize ? (unsigned char)data[i++] : 0;
// 		uint32_t octet_b = i < fsize ? (unsigned char)data[i++] : 0;
// 		uint32_t octet_c = i < fsize ? (unsigned char)data[i++] : 0;
// 
// 		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
// 
// 		ss << encoding_table[(triple >> 3 * 6) & 0x3F];
// 		ss << encoding_table[(triple >> 2 * 6) & 0x3F];
// 		ss << encoding_table[(triple >> 1 * 6) & 0x3F];
// 		ss << encoding_table[(triple >> 0 * 6) & 0x3F];
// 	}
// 
// 	if( (fsize%3) == 1 ) ss << "==";
// 	if( (fsize%3) == 2 ) ss << "=";
// 
// 	// Free file buffer
// 	delete[] buff;
// }


