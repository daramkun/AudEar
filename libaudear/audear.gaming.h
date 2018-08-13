#ifndef __AUDEAR_GAMING_H__
#define __AUDEAR_GAMING_H__

typedef struct AEEXP
{
	float x, y, z;
} AEFLOAT3;

typedef struct AEEXP
{
	AEFLOAT3 position, forward, up, velocity;
} AELISTENER;

typedef struct AEEXP
{
	AEFLOAT3 position, forward, up, velocity;
	float dopplerScale;
} AEEMITTER;


#endif