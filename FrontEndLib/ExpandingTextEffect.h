// $Id: ExpandingTextEffect.h 9773 2011-11-10 16:19:01Z mrimer $

#ifndef CEXPANDINGTEXTEFFECT_H
#define CEXPANDINGTEXTEFFECT_H

#include "Effect.h"

#include <BackEndLib/Wchar.h>
#include <string>

//******************************************************************************
class CExpandingTextEffect : public CEffect
{
public:
	CExpandingTextEffect(CWidget *pSetWidget, const WCHAR* text, 
			UINT eFont,
			const int xOffset, const int yOffset,
			const float scaleIncrease,
			const Uint32 wDuration = 1000, const Uint32 fadeTime = 900);
	virtual ~CExpandingTextEffect();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	void RenderText();

	SDL_Surface *pTextSurface;  //text to display
	SDL_Rect base_size;
	SDL_Rect screenRect;
	WSTRING wstrText;
	UINT eFont;
	int xOffset, yOffset;
	float scaleIncrease;
	Uint32 fadeTime;

	Uint8 nOpacity;
	SDL_Rect drawRect;
};

#endif   //...#ifndef CEXPANDINGTEXTEFFECT_H
