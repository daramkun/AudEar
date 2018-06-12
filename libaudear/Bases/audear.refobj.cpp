#include "../audear.h"

AERefObj::AERefObj () : _refCount ( 1 ) { }
AERefObj::~AERefObj () { }

uint32_t AERefObj::retain ()
{
#if AE_PLATFORM_WINDOWS
	return InterlockedIncrement ( &_refCount );
#endif
}

uint32_t AERefObj::release ()
{
	uint32_t temp =
#if AE_PLATFORM_WINDOWS
		InterlockedDecrement ( &_refCount );
#endif
	if ( temp == 0 )
		delete this;

	return temp;
}
