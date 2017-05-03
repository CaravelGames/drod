// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Assert.h
//Assertion checking and debugging info.

#ifndef ASSERT_H
#define ASSERT_H

#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include <string>
using std::string;

#ifndef DONT_RELEASE_WITH_DEBUG_INFO
#	define RELEASE_WITH_DEBUG_INFO //Uncomment for assert messages in release builds.
#endif

#undef USE_LOGCONTEXT

#ifdef RELEASE_WITH_DEBUG_INFO
#   ifndef _DEBUG
#       define _DEBUG
#   endif
#endif

#ifdef ENABLE_BREAKPOINTS  // don't enable this for release builds!
#   ifdef _MSC_VER
#       include <intrin.h>
#       define MY_BREAKPOINT() __debugbreak()
#   else
#       include <signal.h>
#       define MY_BREAKPOINT() raise(SIGTRAP)
        // (gcc has a breakpoint intrinsic, but we don't use it because it's treated as
        //  never-returns by the optimizer for some bizarre reason)
#   endif
#else
#   define MY_BREAKPOINT() (void)0
#endif

//Assertion-reporting macros.  Avoid simply calling ASSERT(false) because after changes to source code,
//the file and line# may not be sufficient to track the location in current source of the assertion.  Use
//either ASSERT(expression) or ASSERTP(false, "Description of error.").
#define _ASSERT_VOID_CAST(exp)   static_cast<void>(exp)
#ifdef _DEBUG
# if defined(WIN32) && !defined(__GNUC__)
#   define ASSERT(exp)          _ASSERT_VOID_CAST( (exp) ? 0 : AssertErr(__FILE__,__LINE__,#exp) )
#   define ASSERTP(exp, desc)   _ASSERT_VOID_CAST( (exp) ? 0 : AssertErr(__FILE__,__LINE__,(desc)) )
# else
#   define ASSERT(exp)          do { if (!(exp)) { AssertErr(__FILE__,__LINE__,#exp); MY_BREAKPOINT(); }} while (0)
#   define ASSERTP(exp, desc)   do { if (!(exp)) { AssertErr(__FILE__,__LINE__,(desc)); MY_BREAKPOINT(); }} while (0)
# endif
#else
#   define ASSERT(exp)
#   define ASSERTP(exp,desc)
#endif

//Log context is applied to error logging to show a history of what happened before an error.
//When CLogText() is is instanced, error logging will show "BEGIN The task", and when it goes out
//of scope, error logging will show "END The task".  These messages are only shown when an error
//occurs.
#if defined(USE_LOGCONTEXT) && defined(_DEBUG)
#  define LOGCONTEXT(desc) CLogContext _LogContext((desc))

	class CLogContext
	{
	public:
		string strDesc;
		CLogContext(const char *pszDesc);
		~CLogContext();
	};
#else
#  define LOGCONTEXT(desc)
#endif

#ifdef _DEBUG
#  define DYN_CAST(totype,fromtype,source) \
	DynCastAssert<totype,fromtype>(source, __FILE__, __LINE__)
#else
#  define DYN_CAST(totype,fromtype,source) \
	dynamic_cast<totype>(source)
#endif

//Use to assert on a statement that should execute in retail as well as debug builds.
#ifdef _DEBUG
#  define VERIFY(exp) \
		do { if (!(exp)) { AssertErr(__FILE__, __LINE__,#exp); MY_BREAKPOINT(); }} while (0)
#else
#  define VERIFY(exp) exp
#endif   

//Log an error to the error log.
#ifdef _DEBUG
#  define LOGERR(x) LogErr(x)
#else
#  define LOGERR(x)
#endif

//Show a message in debug output.
#ifdef _DEBUG
#  define DEBUGPRINT(x) DebugPrint(x)
#else
#  define DEBUGPRINT(x)
#endif

#ifdef _DEBUG
#  define SETLOGERRORS(x) SetLogErrors(x)
#else
#  define SETLOGERRORS(x)
#endif

//Prototypes.
//Debug-mode.
#ifdef _DEBUG
	 void AssertErr(const char *pszFile, int nLine, const char *pszDesc);
	 void LogErr(const char *pszMessage);
	 void DebugPrint(const char *pszMessage);

	 template<typename To_p, typename From_p>
	 inline To_p DynCastAssert(From_p pSource, const char *pszFile, int nLine)
	 {
		 To_p pTarget = dynamic_cast<To_p>(pSource);
		 if (pSource && !pTarget) AssertErr(pszFile, nLine, "Dynamic cast failed.");
		 return pTarget;
	 }
#endif

void SetLogErrors(const bool bVal);

//Prevent accidental assigns and copies by declaring private methods in a class for
//copy constructor and assign operator.
#define PREVENT_DEFAULT_COPY(ClassName) \
  private: \
	 ClassName (const ClassName &Src); \
	 ClassName &operator= (const ClassName &Src)


//Limited unique-like pointer (we can't rely on C++11 support yet)
template <typename T, void(*deleter)(T*)>
struct UniquePtr
{
	UniquePtr (T* ptr) : ptr(ptr) {}
	~UniquePtr () { if (ptr) deleter(ptr); }
	void reset (T* newptr) { if (ptr) deleter(ptr); ptr = newptr; }
	T* release () { T* result = ptr; ptr = NULL; return result; }
	T* get () const { return ptr; }
private:
	T* ptr;
	PREVENT_DEFAULT_COPY(UniquePtr);
};

#endif //...#ifndef ASSERT_H
