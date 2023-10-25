#include "stdafx.h"
#include "WeaponRevolver.h"
#include "ParticlesObject.h"
#include "Actor.h"

CWeaponRevolver::CWeaponRevolver()
{
    m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    SetPending(false);
}

CWeaponRevolver::~CWeaponRevolver() {}

void CWeaponRevolver::net_Destroy()
{
    inherited::net_Destroy();
}


void CWeaponRevolver::Load(LPCSTR section)
{
    inherited::Load(section);

    m_sounds.LoadSound(section, "snd_reload_less_1", "sndReloadRevolver1", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_2", "sndReloadRevolver2", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_3", "sndReloadRevolver3", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_4", "sndReloadRevolver4", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_5", "sndReloadRevolver5", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_6", "sndReloadRevolver6", false, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reload_less_7", "sndReloadRevolver7", false, m_eSoundReload);
}

void CWeaponRevolver::OnH_B_Chield()
{
    inherited::OnH_B_Chield();
}

void CWeaponRevolver::PlayAnimShow()
{
    inherited::PlayAnimShow();
}

void CWeaponRevolver::PlayAnimBore()
{
    inherited::PlayAnimBore();
}

void CWeaponRevolver::PlayAnimIdleSprint()
{
    inherited::PlayAnimIdleSprint();
}

void CWeaponRevolver::PlayAnimIdleMoving()
{
    inherited::PlayAnimIdleMoving();
}

void CWeaponRevolver::PlayAnimIdle()
{
    inherited::PlayAnimIdle();
}

void CWeaponRevolver::PlayAnimAim()
{
    inherited::PlayAnimAim();
}

void CWeaponRevolver::PlayAnimReload()
{
    int full_drum = m_ammoElapsed.type1 == iMagazineSize;

    auto state = GetState();
    VERIFY(state  == eReload);

    if (bMisfire)
    {
        if (isHUDAnimationExist("anm_reload_misfire"))
            PlayHUDMotion("anm_reload_misfire", true, this, state);
        else if (isHUDAnimationExist("anm_reload_unjam"))
            PlayHUDMotion("anm_reload_unjam", true, this, state);
        else if (isHUDAnimationExist("anm_unjam"))
            PlayHUDMotion("anm_unjam", true, this, state);
        else if (isHUDAnimationExist("anm_reload_jammed"))
            PlayHUDMotion("anm_reload_jammed", true, this, state);
        else
            PlayHUDMotion("anm_reload_empty", true, this, state);
    }
    else
    {
        if (full_drum - 1 && isHUDAnimationExist("anm_reload_less_1"))
            PlayHUDMotion("anm_reload_less_1", true, this, state);
        else if (full_drum - 2 && isHUDAnimationExist("anm_reload_less_2"))
            PlayHUDMotion("anm_reload_less_2", true, this, state);
        else if (full_drum - 3 && isHUDAnimationExist("anm_reload_less_3"))
            PlayHUDMotion("anm_reload_less_3", true, this, state);
        else if (full_drum - 4 && isHUDAnimationExist("anm_reload_less_4"))
            PlayHUDMotion("anm_reload_less_4", true, this, state);
        else if (full_drum - 5 && isHUDAnimationExist("anm_reload_less_5"))
            PlayHUDMotion("anm_reload_less_5", true, this, state);
        else if (full_drum - 6 && isHUDAnimationExist("anm_reload_less_6"))
            PlayHUDMotion("anm_reload_less_6", true, this, state);
        else if (full_drum - 7 && isHUDAnimationExist("anm_reload_less_7"))
            PlayHUDMotion("anm_reload_less_7", true, this, state);
        else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_reload_empty"))
            PlayHUDMotion("anm_reload_empty", true, this, state);
        else
            PlayHUDMotion("anm_reload_empty", true, this, state);
    }
}


void CWeaponRevolver::PlayAnimHide()
{
    inherited::PlayAnimHide();
}

void CWeaponRevolver::PlayAnimShoot()
{
    inherited::PlayAnimShoot();
}


void CWeaponRevolver::switch2_Reload()
{
    inherited::switch2_Reload();
}

void CWeaponRevolver::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
}

void CWeaponRevolver::OnShot()
{
    inherited::OnShot(); //Alundaio: not changed from inherited, so instead of copying changes from weaponmagazined, we just do this
}

void CWeaponRevolver::UpdateSounds()
{
    inherited::UpdateSounds();
    m_sounds.SetPosition("sndReloadRevolver1", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver2", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver3", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver4", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver5", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver6", get_LastFP());
    m_sounds.SetPosition("sndReloadRevolver7", get_LastFP());
}

void CWeaponRevolver::PlayReloadSound()
{
    int full_drum = m_ammoElapsed.type1 == iMagazineSize;

    if (m_sounds_enabled)
    {
        if (bMisfire)
        {
            if (m_sounds.FindSoundItem("sndReloadMisfire", false))
                PlaySound("sndReloadMisfire", get_LastFP());
            else
                PlaySound("sndReloadEmpty", get_LastFP());
        }
        else
        {
            if (full_drum - 1 && m_sounds.FindSoundItem("sndReloadRevolver1", false))
                PlaySound("sndReloadRevolver1", get_LastFP());
            else if (full_drum - 2 && m_sounds.FindSoundItem("sndReloadRevolver2", false))
                PlaySound("sndReloadRevolver2", get_LastFP());
            else if (full_drum - 3 && m_sounds.FindSoundItem("sndReloadRevolver3", false))
                PlaySound("sndReloadRevolver3", get_LastFP());
            else if (full_drum - 4 && m_sounds.FindSoundItem("sndReloadRevolver4", false))
                PlaySound("sndReloadRevolver4", get_LastFP());
            else if (full_drum - 5 && m_sounds.FindSoundItem("sndReloadRevolver5", false))
                PlaySound("sndReloadRevolver5", get_LastFP());
            else if (full_drum - 6 && m_sounds.FindSoundItem("sndReloadRevolver6", false))
                PlaySound("sndReloadRevolver6", get_LastFP());
            else if (full_drum - 7 && m_sounds.FindSoundItem("sndReloadRevolver7", false))
                PlaySound("sndReloadRevolver7", get_LastFP());
            else if (m_ammoElapsed.type1 == 0 && m_sounds.FindSoundItem("sndReloadEmpty", false))
                PlaySound("sndReloadEmpty", get_LastFP());
            else
                PlaySound("sndReloadEmpty", get_LastFP());
        }
    }
}
