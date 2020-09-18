// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME srcdIpluginsdIrootspymIclientdIRScint_Dict

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
#include "src/plugins/rootspy-client/RScint.h"

// Header files passed via #pragma extra_include

namespace ROOT {
   static void *new_RScint(void *p = 0);
   static void *newArray_RScint(Long_t size, void *p);
   static void delete_RScint(void *p);
   static void deleteArray_RScint(void *p);
   static void destruct_RScint(void *p);
   static void streamer_RScint(TBuffer &buf, void *obj);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::RScint*)
   {
      ::RScint *ptr = 0;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::RScint >(0);
      static ::ROOT::TGenericClassInfo 
         instance("RScint", ::RScint::Class_Version(), "RScint.h", 29,
                  typeid(::RScint), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::RScint::Dictionary, isa_proxy, 16,
                  sizeof(::RScint) );
      instance.SetNew(&new_RScint);
      instance.SetNewArray(&newArray_RScint);
      instance.SetDelete(&delete_RScint);
      instance.SetDeleteArray(&deleteArray_RScint);
      instance.SetDestructor(&destruct_RScint);
      instance.SetStreamerFunc(&streamer_RScint);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::RScint*)
   {
      return GenerateInitInstanceLocal((::RScint*)0);
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal((const ::RScint*)0x0); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr RScint::fgIsA(0);  // static to hold class pointer

//______________________________________________________________________________
const char *RScint::Class_Name()
{
   return "RScint";
}

//______________________________________________________________________________
const char *RScint::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RScint*)0x0)->GetImplFileName();
}

//______________________________________________________________________________
int RScint::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::RScint*)0x0)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *RScint::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RScint*)0x0)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *RScint::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::RScint*)0x0)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void RScint::Streamer(TBuffer &R__b)
{
   // Stream an object of class RScint.

   UInt_t R__s, R__c;
   if (R__b.IsReading()) {
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c); if (R__v) { }
      TObject::Streamer(R__b);
      R__b >> initialized;
      R__b.CheckByteCount(R__s, R__c, RScint::IsA());
   } else {
      R__c = R__b.WriteVersion(RScint::IsA(), kTRUE);
      TObject::Streamer(R__b);
      R__b << initialized;
      R__b.SetByteCount(R__c, kTRUE);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_RScint(void *p) {
      return  p ? new(p) ::RScint : new ::RScint;
   }
   static void *newArray_RScint(Long_t nElements, void *p) {
      return p ? new(p) ::RScint[nElements] : new ::RScint[nElements];
   }
   // Wrapper around operator delete
   static void delete_RScint(void *p) {
      delete ((::RScint*)p);
   }
   static void deleteArray_RScint(void *p) {
      delete [] ((::RScint*)p);
   }
   static void destruct_RScint(void *p) {
      typedef ::RScint current_t;
      ((current_t*)p)->~current_t();
   }
   // Wrapper around a custom streamer member function.
   static void streamer_RScint(TBuffer &buf, void *obj) {
      ((::RScint*)obj)->::RScint::Streamer(buf);
   }
} // end of namespace ROOT for class ::RScint

namespace {
  void TriggerDictionaryInitialization_RScint_Dict_Impl() {
    static const char* headers[] = {
"src/plugins/rootspy-client/RScint.h",
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
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/src/plugins/rootspy-client",
"/home/microway/Downloads/NEWROOT/root-build/include",
"/home/microway/Desktop/projects/tridas/test_streamFeb2020/tests/test6_hdbscan/streamingReco/",
0
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "RScint_Dict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_Autoloading_Map;
class __attribute__((annotate("$clingAutoload$src/plugins/rootspy-client/RScint.h")))  RScint;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "RScint_Dict dictionary payload"

#ifndef G__VECTOR_HAS_CLASS_ITERATOR
  #define G__VECTOR_HAS_CLASS_ITERATOR 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H
#include "src/plugins/rootspy-client/RScint.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[]={
"RScint", payloadCode, "@",
nullptr};

    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("RScint_Dict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_RScint_Dict_Impl, {}, classesHeaders, /*has no C++ module*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_RScint_Dict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_RScint_Dict() {
  TriggerDictionaryInitialization_RScint_Dict_Impl();
}
