#ifndef __AUDEAR_THREAD_HPP__
#define __AUDEAR_THREAD_HPP__

#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
#	include <Windows.h>
#else
#	include <pthread.h>
#endif

class __Runnable
{
public:
	virtual void Run ( void * obj, const bool & terminate ) PURE;
};

class __Thread
{
public:
	inline __Thread ( __Runnable * runnable )
		: runnable ( runnable )
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		, hThread ( INVALID_HANDLE_VALUE )
#else
		, threadId ( 0 )
#endif
	{

	}
	inline ~__Thread ()
	{
		Terminate ();
	}

public:
	inline bool Run ( void * obj )
	{
		if ( IsThreadAlive () )
			return false;

		this->obj = obj;
		this->terminate = false;
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		DWORD threadId;
		hThread = CreateThread ( nullptr, 0, ThreadStart, this, 0, &threadId );
		return ( hThread != INVALID_HANDLE_VALUE );
#else
		pthread_create ( &threadId, nullptr, ThreadStart, this );
#endif
	}

	inline void Terminate ( bool safeTerminateOnly = false )
	{
		terminate = true;
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		if ( WaitForSingleObject ( hThread, 500 ) != WAIT_OBJECT_0 && !safeTerminateOnly )
			TerminateThread ( hThread, 0 );
		hThread = INVALID_HANDLE_VALUE;
#else
		int state;
		if ( safeTerminateOnly )
			pthread_join ( threadId, &state );
		else
			pthread_cancel ( threadId );
#endif
	}

	inline void Join ()
	{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		if ( hThread == INVALID_HANDLE_VALUE ) return;
		if ( !IsThreadAlive () ) return;
		WaitForSingleObject ( hThread, INFINITE );
		hThread = INVALID_HANDLE_VALUE;
#else
		if ( threadId == 0 ) return;
		if ( !IsThreadAlive () ) return;
		int state;
		pthread_join ( threadId, &state );
		threadId = 0;
#endif
	}

public:
	inline bool IsThreadAlive ()
	{
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		if ( hThread == INVALID_HANDLE_VALUE ) return false;
		if ( WaitForSingleObject ( hThread, 0 ) != WAIT_OBJECT_0 )
			return true;
		hThread = INVALID_HANDLE_VALUE;
		return false;
#else
		if ( threadId == 0 ) return false;
		//if ( pthread_kill ( threadId, 0 ) == 0 )
		if ( threadId != 0 )
			return true;
		threadId = 0;
		return false;
#endif
	}

private:
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	static DWORD CALLBACK ThreadStart ( LPVOID ptr )
#else
	static void * ThreadStart ( void * ptr )
#endif
	{
		__Thread * thread = reinterpret_cast< __Thread* >( ptr );
		thread->runnable->Run ( thread->obj, thread->terminate );
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
		thread->hThread = INVALID_HANDLE_VALUE;
		return 0;
#else
		thread->threadId = 0;
		pthread_exit ( 0 );
#endif
	}

private:
#if AE_PLATFORM_WINDOWS || AE_PLATFORM_UWP
	HANDLE hThread;
#else
	pthread_t threadId;
#endif
	__Runnable * runnable;
	void * obj;
	bool terminate;
};

#endif