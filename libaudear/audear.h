#ifndef __AUDEAR_H__
#define __AUDEAR_H__

#include "audear.preprocessors.h"

#ifdef __cplusplus
extern "C"
{
#endif

	EXTC AEEXP void AE_init ();

#include "audear.struct.h"
#include "audear.interface.h"

#ifdef __cplusplus
}
#endif

#include "Streams/audear.streams.h"
#include "Decoders/audear.decoders.h"
#include "Players/audear.players.h"

#endif