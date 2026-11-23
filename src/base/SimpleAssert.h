




#ifndef __SimpleAssert_H__
#define __SimpleAssert_H__

#include <conio.h>
#include <stdlib.h>







extern bool g_assert_message ;
#if defined(BF_PORT)

  #include "bf_trace.h"

  #define sAssert(exp)        { if (!(exp)) bf_assert_failed(#exp, NULL, __FILE__, __LINE__); }
  #define sAssertM(exp, msg)  { if (!(exp)) bf_assert_failed(#exp, msg, __FILE__, __LINE__); }
  #define sThrow( str )  if( _sThrowBox( str , __FILE__, __LINE__ ) ) { __builtin_debugtrap(); }

#else
#ifdef ASSERT_MESSAGE

  bool _sAssertDlg_(const char*, const char*,
					const char*, int, bool*);


  #define sAssert(exp) { \
	static bool _ignoreAlways_ = false; \
	if (g_assert_message && !_ignoreAlways_ && !(exp)) { \
	  if (_sAssertDlg_(#exp, NULL, __FILE__, __LINE__, &_ignoreAlways_)) \
		{ _asm { int 3 } } \
	} \
  }


  #define sAssertM(exp, msg) { \
	static bool _ignoreAlways_ = false; \
	if (g_assert_message && !_ignoreAlways_ && !(exp)) { \
	  if (_sAssertDlg_(#exp, msg, __FILE__, __LINE__, &_ignoreAlways_)) \
		{ _asm { int 3 } } \
	} \
  }

#else





  #define sAssert(exp)
  #define sAssertM(exp, msg)
#endif


#define sThrow( str )  if( _sThrowBox( str , __FILE__, __LINE__ ) ) {_asm { int 3 }}
#endif

bool _sThrowBox(const char*, const char*, int) ;

#endif
