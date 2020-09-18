// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME srcdIexternaldIRootSpydIsrcdIdOdOLinux_x86_64dIRootSpydIDialog_ShowMacro_Dict

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "RConfig.h"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// Since CINT ignores the std namespace, we need to do so in this file.
namespace std {} using namespace std;

// Header files passed as explicit arguments
#include "src/external/RootSpy/src/RootSpy/Dialog_ShowMacro.h"

// Header files passed via #pragma extra_include

namespace ROOT {
   static void delete_Dialog_ShowMacro(void *p);
   static void deleteArray_Dialog_ShowMacro(void *p);
   static void destruct_Dialog_ShowMacro(void *p);
   static void streamer_Dialog_ShowMacro(TBuffer &buf, void *obj);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::Dialog_ShowMacro*)
   {
      ::Dialog_ShowMacro *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::Dialog_ShowMacro >(0);
      static ::ROOT::TGenericClassInfo 
         instance("Dialog_ShowMacro", ::Dialog_ShowMacro::Class_Version(), "RootSpy/src/RootSpy/Dialog_ShowMacro.h", 17,
                  typeid(::Dialog_ShowMacro), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::Dialog_ShowMacro::Dictionary, isa_proxy, 16,
                  sizeof(::Dialog_ShowMacro) );
      instance.SetDelete(&delete_Dialog_ShowMacro);
      instance.SetDeleteArray(&deleteArray_Dialog_ShowMacro);
      instance.SetDestructor(&destruct_Dialog_ShowMacro);
      instance.SetStreamerFunc(&streamer_Dialog_ShowMacro);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::Dialog_ShowMacro*)
   {
      return GenerateInitInstanceLocal((::Dialog_ShowMacro*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal((const ::Dialog_ShowMacro*)0x0); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr Dialog_ShowMacro::fgIsA(0);  // static to hold class pointer

//______________________________________________________________________________
const char *Dialog_ShowMacro::Class_Name()
{
   return "Dialog_ShowMacro";
}

//______________________________________________________________________________
const char *Dialog_ShowMacro::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Dialog_ShowMacro*)0x0)->GetImplFileName();
}

//______________________________________________________________________________
int Dialog_ShowMacro::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::Dialog_ShowMacro*)0x0)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *Dialog_ShowMacro::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Dialog_ShowMacro*)0x0)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *Dialog_ShowMacro::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::Dialog_ShowMacro*)0x0)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void Dialog_ShowMacro::Streamer(TBuffer &R__b)
{
   // Stream an object of class Dialog_ShowMacro.

   UInt_t R__s, R__c;
   if (R__b.IsReading()) {
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c); if (R__v) { }
      TGMainFrame::Streamer(R__b);
      R__b.CheckByteCount(R__s, R__c, Dialog_ShowMacro::IsA());
   } else {
      R__c = R__b.WriteVersion(Dialog_ShowMacro::IsA(), kTRUE);
      TGMainFrame::Streamer(R__b);
      R__b.SetByteCount(R__c, kTRUE);
   }
}

namespace ROOT {
   // Wrapper around operator delete
   static void delete_Dialog_ShowMacro(void *p) {
      delete ((::Dialog_ShowMacro*)p);
   }
   static void deleteArray_Dialog_ShowMacro(void *p) {
      delete [] ((::Dialog_ShowMacro*)p);
   }
   static void destruct_Dialog_ShowMacro(void *p) {
      typedef ::Dialog_ShowMacro current_t;
      ((current_t*)p)->~current_t();
   }
   // Wrapper around a custom streamer member function.
   static void streamer_Dialog_ShowMacro(TBuffer &buf, void *obj) {
      ((::Dialog_ShowMacro*)obj)->::Dialog_ShowMacro::Streamer(buf);
   }
} // end of namespace ROOT for class ::Dialog_ShowMacro

namespace {
  void TriggerDictionaryInitialization_Dialog_ShowMacro_Dict_Impl() {
    static const char* headers[] = {
"src/external/RootSpy/src/RootSpy/Dialog_ShowMacro.h",
0
    };
    static const char* includePaths[] = {
"/root/jana2/Linux_Ubuntu16.04-x86_64-gcc5.4.0//src/libraries",
"/usr/include/boost/include",
"/root/ccdb/include",
"/home/microway/Downloads/NEWROOT/root-build/include",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/libraries",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/plugins",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external/cMsg-3.6/src/regexp",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external/cMsg-3.6/src/libsrc",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external/cMsg-3.6/src/libsrc++",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external/RootSpy/src/libRootSpy",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/external/RootSpy/src/libRootSpy-client",
"/home/microway/Downloads/NEWROOT/root-build/include",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/",
0
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "Dialog_ShowMacro_Dict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_Autoloading_Map;
class __attribute__((annotate("$clingAutoload$src/external/RootSpy/src/RootSpy/Dialog_ShowMacro.h")))  Dialog_ShowMacro;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "Dialog_ShowMacro_Dict dictionary payload"

#ifndef G__VECTOR_HAS_CLASS_ITERATOR
  #define G__VECTOR_HAS_CLASS_ITERATOR 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H
#include "src/external/RootSpy/src/RootSpy/Dialog_ShowMacro.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[]={
"Dialog_ShowMacro", payloadCode, "@",
nullptr};

    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("Dialog_ShowMacro_Dict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_Dialog_ShowMacro_Dict_Impl, {}, classesHeaders, /*has no C++ module*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_Dialog_ShowMacro_Dict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_Dialog_ShowMacro_Dict() {
  TriggerDictionaryInitialization_Dialog_ShowMacro_Dict_Impl();
}
