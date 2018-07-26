#ifndef __AUDEAR_PREPROCESSORS_H__
#define __AUDEAR_PREPROCESSORS_H__

#	define AE_COMPILER_MSVC									defined ( _MSC_VER )
#	define AE_COMPILER_GCC									defined ( __GNUC__ ) && !defined ( __clang__ )
#	define AE_COMPILER_CLANG								defined ( __clang__ )

#	if AE_COMPILER_MSVC && defined ( _DEBUG )
#		include <atlbase.h>
#		define _CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#		ifndef DBG_NEW
#			define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#			define new DBG_NEW
#		endif
#	endif

#	if ( defined ( _WINDOWS ) || defined ( _WIN32 ) || defined ( _WIN64 ) || defined ( WIN32 ) || defined ( WIN64 ) ) \
		&& !( defined ( __APPLE__ ) || defined ( __ANDROID__ ) || defined ( __unix__ ) || defined ( __linux__ ) )
#		include <Windows.h>
#		define AE_PLATFORM_WINDOWS							WINAPI_FAMILY_PARTITION ( WINAPI_PARTITION_DESKTOP ) || !defined ( WINAPI_FAMILY_DESKTOP_APP )
#		define AE_PLATFORM_UWP								WINAPI_FAMILY_PARTITION ( WINAPI_FAMILY_PC_APP )
#	else
#		define AE_PLATFORM_WINDOWS							0
#		define AE_PLATFORM_UWP								0
#	endif
#	if defined ( __APPLE__ )
#		include <TargetConditionals.h>
#		define AE_PLATFORM_MACOS							TARGET_OS_MAC && !( TARGET_OS_IOS || TARGET_OS_SIMULATOR )
#		define AE_PLATFORM_IOS								TARGET_OS_IOS || TARGET_OS_SIMULATOR
#		import <Foundation/Foundation.h>
#		include <sys/time.h>
#	else
#		define AE_PLATFORM_MACOS							0
#		define AE_PLATFORM_IOS								0
#	endif
#	define AE_PLATFORM_ANDROID								defined ( __ANDROID__ )
#	define AE_PLATFORM_UNIX									( defined ( __unix__ ) || defined ( __linux__ ) ) && !defined ( __ANDROID__ )

#	define AE_ARCH_IA32										defined ( _M_IX86 ) || defined ( __i386__ )
#	define AE_ARCH_AMD64									defined ( _M_AMD64 ) || defined ( __amd64__ )
#	define AE_ARCH_ARM										defined ( _M_ARM ) || defined ( __arm__ )
#	define AE_ARCH_ARM64									defined ( __aarch64__ )
#	define AE_ARCH_UNKNOWN									!( AE_ARCH_IA32 || AE_ARCH_AMD64 || AE_ARCH_ARM || AE_ARCH_ARM64 )

#	if AE_PLATFORM_WINDOWS
#		if defined ( LIBAUDEAR_EXPORTS ) && LIBAUDEAR_EXPORTS
#			define AEEXP									__declspec ( dllexport )
#		else
#			define AEEXP									__declspec ( dllimport )
#		endif
#	else
#		define AEEXP
#	endif

#	ifdef __cplusplus
#		define EXTC											extern "C"
#		define INLINE										inline
#	else
#		define EXTC
#		define INLINE
#	endif

#	ifndef PURE
#		define PURE											= 0
#	endif

#	ifndef IN
#		define IN
#	endif

#	ifndef OUT
#		define OUT
#	endif

#	include <stdint.h>
#	include <stdbool.h>

typedef int32_t												error_t;

#	define AEERROR_NOERROR									0
#	define AEERROR_UNKNOWN									-1
#	define AEERROR_FAIL										-2
#	define AEERROR_INVALID_ARGUMENT							-3
#	define AEERROR_ARGUMENT_IS_NULL							-4
#	define AEERROR_NOT_IMPLEMENTED							-5
#	define AEERROR_END_OF_FILE								-6
#	define AEERROR_INVALID_CALL								-7
#	define AEERROR_NOT_SUPPORTED_FORMAT						-8
#	define AEERROR_NOT_SUPPORTED_FEATURE					-9
#	define AEERROR_OUT_OF_MEMOEY							-10
#	define AERRROR_OUT_OF_RANGE								-11

#	ifndef ISERROR
#		define ISERROR(x)									( x < AEERROR_NOERROR )
#	endif
#	ifndef NOERROR
#		define NOERROR(x)									( x == AEERROR_NOERROR )
#	endif

#	if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
#		define AE_alloc										CoTaskMemAlloc
#		define AE_free										CoTaskMemFree
#	else
#		define AE_alloc										malloc
#		define AE_free										free
#	endif
#	define AE_allocType(x)									( x * ) AE_alloc ( sizeof ( x ) )

#endif