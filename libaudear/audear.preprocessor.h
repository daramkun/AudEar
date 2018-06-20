#ifndef		__AUDEAR_PREPROCESSOR_H__
#	define	__AUDEAR_PREPROCESSOR_H__

// Microsoft Visual C++ Compiler
#	define AE_COMPILER_MSVC									defined ( _MSC_VER )
// GNU Compiler Collection
#	define AE_COMPILER_GCC									defined ( __GNUC__ ) && !defined ( __clang__ )
// LLVM Clang
#	define AE_COMPILER_CLANG								defined ( __clang__ )

#	if ( defined ( _WINDOWS ) || defined ( _WIN32 ) || defined ( _WIN64 ) || defined ( WIN32 ) || defined ( WIN64 ) ) \
		&& !( defined ( __APPLE__ ) || defined ( __ANDROID__ ) || defined ( __unix__ ) || defined ( __linux__ ) )
#		include <Windows.h>
// Microsoft Windows NT
#		define AE_PLATFORM_WINDOWS							WINAPI_FAMILY_PARTITION ( WINAPI_PARTITION_DESKTOP ) || !defined ( WINAPI_FAMILY_DESKTOP_APP )
// Microsoft Windows RT(Windows 8(.1) Apps and UWP)
#		define AE_PLATFORM_UWP								WINAPI_FAMILY_PARTITION ( WINAPI_FAMILY_PC_APP )
#	else
// Microsoft Windows NT
#		define AE_PLATFORM_WINDOWS							0
// Microsoft Windows RT(Windows 8(.1) Apps and UWP)
#		define AE_PLATFORM_UWP								0
#	endif
#	if defined ( __APPLE__ )
#		include <TargetConditionals.h>
// Apple macOS
#		define AE_PLATFORM_MACOS							TARGET_OS_MAC && !( TARGET_OS_IOS || TARGET_OS_SIMULATOR )
// Apple iOS
#		define AE_PLATFORM_IOS								TARGET_OS_IOS || TARGET_OS_SIMULATOR
#		import <Foundation/Foundation.h>
#		include <sys/time.h>
#	else
// Apple macOS
#		define AE_PLATFORM_MACOS							0
// Apple iOS
#		define AE_PLATFORM_IOS								0
#	endif
// Google Android
#	define AE_PLATFORM_ANDROID								defined ( __ANDROID__ )
// UNIX and Linux without Android
#	define AE_PLATFORM_UNIX									( defined ( __unix__ ) || defined ( __linux__ ) ) && !defined ( __ANDROID__ )

// Intel x86 Architecture
#	define AE_ARCH_IA32										defined ( _M_IX86 ) || defined ( __i386__ )
// AMD x64 Architecture
#	define AE_ARCH_AMD64									defined ( _M_AMD64 ) || defined ( __amd64__ )
// ARM Architecture
#	define AE_ARCH_ARM										defined ( _M_ARM ) || defined ( __arm__ )
// ARM64 Architecture
#	define AE_ARCH_ARM64									defined ( __aarch64__ )
// Unknown Architecture
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
#	define EXTC												extern "C"

#	ifndef PURE
#		define PURE
#	endif

#	ifndef IN
#		define IN
#	endif

#	ifndef OUT
#		define OUT
#	endif

#	ifndef STDCALL
#		define STDCALL										__stdcall
#	endif

#	include <cstdint>

typedef int32_t												AEERROR;
#	define AEERROR_SUCCESS									0
#	define AEERROR_UNKNOWN									-1
#	define AEERROR_FAIL										-2
#	define AEERROR_INVALID_ARGUMENT							-3
#	define AEERROR_NOT_IMPLEMENTED							-4
#	define AEERROR_END_OF_FILE								-5
#	define AEERROR_INVALID_CALL								-6
#	define AEERROR_NOT_SUPPORTED_FORMAT						-7
#	define AEERROR_NOT_SUPPORTED_FEATURE					-8
#	ifndef SUCCEEDED
#		define SUCCEEDED(x)									( x == AEERROR_SUCCESS )
#	endif
#	ifndef FAILED
#		define FAILED(x)									( x != AEERROR_SUCCESS )
#	endif

#endif