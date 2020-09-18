// $Id: Dialog_ConfigMacros.h 21852 2017-01-22 01:52:11Z davidl $

//    File: Dialog_ConfigMacros.h
// Creator: sdobbs


#ifndef _Dialog_ConfigMacros_
#define _Dialog_ConfigMacros_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListBox.h>
#include <TGPicture.h>
#include <TGTextEntry.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <TGFileDialog.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "hinfo_t.h"

using namespace std;

class Dialog_ConfigMacros:public TGMainFrame{
	public:
		Dialog_ConfigMacros(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_ConfigMacros();
		
		
		void DoAdd(void);
		void DoDelete(void);
		void DoSelectEntry(Int_t id);
		void DoSaveEntry();

		void CloseWindow(void);

	private:

		void CreateGUI(void);

		TGListBox *listBox;
		TGTextEntry *hnamepathBox;
		TGTextEntry *macroBox;

		map<string,string> new_macro_files;

		ClassDef(Dialog_ConfigMacros,1)
};

#endif // _Dialog_ConfigMacros_

