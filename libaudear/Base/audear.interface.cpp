#include "../audear.h"

#include <atomic>

struct AEINTERFACE
{
	std::atomic<int32_t> refCount;
	void * object;
	void ( *dispose ) ( void * obj );
	const char * tag;
};

void * AE_allocInterface ( size_t cb )
{
	void * ret = AE_alloc ( cb );
	if ( ret == nullptr ) return nullptr;

	memset ( ret, 0, cb );

	reinterpret_cast< AEINTERFACE* >( ret )->refCount = 1;

	return ret;
}

int32_t AE_retainInterface ( void * obj )
{
	if ( obj == nullptr ) return 0;
	AEINTERFACE * i = reinterpret_cast< AEINTERFACE* >( obj );
	++i->refCount;
	return i->refCount;
}

int32_t AE_releaseInterface ( void ** obj )
{
	if ( obj == nullptr || *obj == nullptr ) return -1;
	auto aeInterface = reinterpret_cast< AEINTERFACE* >( *obj );
	int32_t ret = --aeInterface->refCount;
	if ( ret == 0 )
	{
		if ( aeInterface->dispose )
			aeInterface->dispose ( aeInterface->object );
		AE_free ( aeInterface );
	}
	*obj = nullptr;

	return ret;
}
