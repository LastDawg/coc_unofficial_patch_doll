////////////////////////////////////////////////////////////////////////////
//	Module 		: DynamicHudGlass.cpp
//	Created 	: 12.05.2021
//  Modified 	: 11.07.2021
//	Author		: Dance Maniac (M.F.S. Team)
//	Description : Dynamic HUD glass functions and variables
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DynamicHudGlass.h"
#include "ActorHelmet.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Inventory.h"

namespace DynamicHudGlass
{
	bool DynamicHudGlassEnabled = false;
	int	HudGlassElement = 0;
	int NightVisionType = 0;

	void UpdateDynamicHudGlass()
	{
		CHelmet* helmet = smart_cast<CHelmet*>(Actor()->inventory().ItemFromSlot(HELMET_SLOT));
		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(Actor()->inventory().ItemFromSlot(OUTFIT_SLOT));
        CTorch* torch = smart_cast<CTorch*>(Actor()->inventory().ItemFromSlot(TORCH_SLOT));

		if (helmet)
		{
			float condition = helmet->GetCondition();
			HudGlassElement = 0;
			bool HelmetHasGlass = helmet->m_b_HasGlass;

            if (torch)
                NightVisionType = torch->m_NightVisionType;
            else
			    NightVisionType = helmet->m_NightVisionType;

			if (HelmetHasGlass)
			{
				DynamicHudGlassEnabled = true;
			}
			else
			{
				DynamicHudGlassEnabled = false;
			}

			if (condition < 0.85)
			{
				if (condition > 0.75)
					HudGlassElement = 1;
				else if (condition > 0.65)
					HudGlassElement = 2;
				else if (condition > 0.45)
					HudGlassElement = 3;
				else if (condition > 0.25)
					HudGlassElement = 4;
				else
					HudGlassElement = 5;
			}
		}
		else
		if (outfit)
		{
			float condition = outfit->GetCondition();
			bool OutfitHasGlass = outfit->m_b_HasGlass;
			HudGlassElement = 0;

            if (torch)
                NightVisionType = torch->m_NightVisionType;
            else
			    NightVisionType = outfit->m_NightVisionType;

			if (OutfitHasGlass)
			{
				DynamicHudGlassEnabled = true;
			}
			else
			{
				DynamicHudGlassEnabled = false;
			}

			if (condition < 0.85)
			{
				if (condition > 0.75)
					HudGlassElement = 1;
				else if (condition > 0.65)
					HudGlassElement = 2;
				else if (condition > 0.45)
					HudGlassElement = 3;
				else if (condition > 0.25)
					HudGlassElement = 4;
				else
					HudGlassElement = 5;
			}
		}
		else
		{
			HudGlassElement = 0;
			DynamicHudGlassEnabled = false;

            if (torch)
                NightVisionType = torch->m_NightVisionType;
            else
                NightVisionType = 0;
		}
	}
}
