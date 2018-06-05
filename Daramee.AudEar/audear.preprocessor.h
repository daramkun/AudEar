#ifndef __AUDEAR_PREPROCESSOR_H__
#define __AUDEAR_PREPROCESSOR_H__

#if defined ( WINDOWS ) || defined ( _WINDOWS ) || defined ( WIN32 ) || defined ( WIN64 )\
		|| defined ( _WIN32 ) || defined ( _WIN64 )
#	define AEWINDOWS										1
#	if defined ( DARAMEEAUDEAR_EXPORTS )
#		define AEEXP										__declspec ( dllexport )
#	else
#		define AEEXP										__declspec ( dllimport )
#	endif
#	include <Windows.h>
#	include <Unknwnbase.h>
#	ifndef DECLSPEC_UUID
#		define DECLSPEC_UUID(x)									__declspec ( uuid ( x ) )
#	endif
#else
#	define AEWINDOWS										0
#	define AEEXP
typedef LONGLONG											long long
typedef ULONG												unsigned long
typedef HRESULT												long
#	define DECLSPEC_UUID(x)
struct IUnknown
{
public:
	virtual ULONG __stdcall AddRef () PURE;
	virtual ULONG __stdcall Release () PURE;
};
#endif

#define E_EOF												__HRESULT_FROM_WIN32 ( ERROR_HANDLE_EOF )

#endif