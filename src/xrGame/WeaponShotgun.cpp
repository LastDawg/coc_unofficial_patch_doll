#include "stdafx.h"
#include "weaponshotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "Level.h"
#include "actor.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"

CWeaponShotgun::CWeaponShotgun()
{
    m_eSoundClose = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
    m_eSoundAddCartridge = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
}

CWeaponShotgun::~CWeaponShotgun() {}
void CWeaponShotgun::net_Destroy() { inherited::net_Destroy(); }
void CWeaponShotgun::Load(LPCSTR section)
{
    inherited::Load(section);

    if (pSettings->line_exist(section, "tri_state_reload"))
    {
        m_bTriStateReload = !!pSettings->r_bool(section, "tri_state_reload");
    };
    m_bOpenWeaponEmptyCartridge = READ_IF_EXISTS(pSettings, r_bool, section, "open_weapon_empty_cartridge", false);
    m_bOpenWeaponCartridge = READ_IF_EXISTS(pSettings, r_bool, section, "open_weapon_cartridge", false);

    if (m_bTriStateReload)
    {
        m_sounds.LoadSound(section, "snd_open_weapon", "sndOpen", false, m_eSoundOpen);
        m_sounds.LoadSound(section, "snd_open_weapon_empty", "sndOpenEmpty", false, m_eSoundOpen);
        m_sounds.LoadSound(section, "snd_add_cartridge", "sndAddCartridge", false, m_eSoundAddCartridge);
        m_sounds.LoadSound(section, "snd_add_cartridge_empty", "sndAddCartridgeEmpty", false, m_eSoundAddCartridge);
        m_sounds.LoadSound(section, "snd_close_weapon", "sndClose", false, m_eSoundClose);
        m_sounds.LoadSound(section, "snd_close_weapon_empty", "sndCloseEmpty", false, m_eSoundClose);
    };
}

void CWeaponShotgun::switch2_Fire()
{
    inherited::switch2_Fire();
    bWorking = false;
}

bool CWeaponShotgun::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    if (m_bTriStateReload && GetState() == eReload && cmd == kWPN_FIRE && flags & CMD_START &&
        m_sub_state == eSubstateReloadInProcess) //остановить перезагрузку
    {
        AddCartridge(1);
        m_sub_state = eSubstateReloadEnd;
        switch2_EndReload();
        return true;
    }
    return false;
}

void CWeaponShotgun::OnAnimationEnd(u32 state)
{
    if (!m_bTriStateReload || state != eReload)
        return inherited::OnAnimationEnd(state);
	CActor* A = smart_cast<CActor*>(H_Parent());
	if (A)
		A->callback(GameObject::eActorHudAnimationEnd)(smart_cast<CGameObject*>(this)->lua_game_object(), this->hud_sect.c_str(), this->m_current_motion.c_str(), state, this->animation_slot());

    switch (m_sub_state)
    {
    case eSubstateReloadBegin:
    {
        if (m_bOpenWeaponCartridge || m_bOpenWeaponEmptyCartridge && isHUDAnimationExist("anm_open_empty"))
            AddCartridge(1);

        m_sub_state = eSubstateReloadInProcess;
        SwitchState(eReload);
    }
    break;

    case eSubstateReloadInProcess:
    {
        if (0 != AddCartridge(1))
        {
            m_sub_state = eSubstateReloadEnd;
        }
        SwitchState(eReload);
    }
    break;

    case eSubstateReloadEnd:
    {
        m_sub_state = eSubstateReloadBegin;
        SwitchState(eIdle);
    }
    break;
    };
}

void CWeaponShotgun::Reload()
{
	if(m_bTriStateReload){
		if (m_pInventory)
		{
			CActor* A = smart_cast<CActor*>(H_Parent());
			if (A)
			{
				int	AC = GetSuitableAmmoTotal();
				A->callback(GameObject::eWeaponNoAmmoAvailable)(lua_game_object(), AC);
			}
		}
		TriStateReload();
	}else
		inherited::Reload();
}

void CWeaponShotgun::TriStateReload()
{
    if (m_magazine.size() == (u32)iMagazineSize || !HaveCartridgeInInventory(1))
        return;

    if (isHUDAnimationExist("anm_reload_misfire"))
    {
        if (IsMisfire() && m_ammoElapsed.type1 == 0 || !IsMisfire())
        {
            CWeapon::Reload();
            m_sub_state = eSubstateReloadBegin;
            SwitchState(eReload);
        }
        else if (IsMisfire() && m_ammoElapsed.type1 > 0)
        {
            CWeapon::Reload();
            m_sub_state = eSubstateUnMisfire;
            SwitchState(eUnMisfire);
        }
    }

    if (!isHUDAnimationExist("anm_reload_misfire"))
    {
        CWeapon::Reload();
        m_sub_state = eSubstateReloadBegin;
        SwitchState(eReload);
    }
}

void CWeaponShotgun::OnStateSwitch(u32 S, u32 oldState)
{
    if (!m_bTriStateReload || S != eReload)
    {
        inherited::OnStateSwitch(S, oldState);
        return;
    }

    CWeapon::OnStateSwitch(S, oldState);

    if (m_magazine.size() == (u32)iMagazineSize || !HaveCartridgeInInventory(1))
    {
        switch2_EndReload();
        m_sub_state = eSubstateReloadEnd;
        return;
    };

    switch (m_sub_state)
    {
    case eSubstateReloadBegin:
        if (HaveCartridgeInInventory(1))
            switch2_StartReload();
        break;
    case eSubstateReloadInProcess:
        if (HaveCartridgeInInventory(1))
            switch2_AddCartgidge();
        break;
    case eSubstateReloadEnd: 
        switch2_EndReload(); 
        break;
    case eSubstateUnMisfire:
        if (IsMisfire() && m_ammoElapsed.type1 > 0 && isHUDAnimationExist("anm_reload_misfire"))
            switch2_UnMisfire();
        break;
    };
}

void CWeaponShotgun::switch2_StartReload()
{
	if (isHUDAnimationExist("anm_open_empty") && m_sounds.FindSoundItem("sndOpenEmpty", false) && m_ammoElapsed.type1 == 0)
        PlaySound("sndOpenEmpty", get_LastFP());
    else
        PlaySound("sndOpen", get_LastFP());

    PlayAnimOpenWeapon();
    SetPending(TRUE);
}

void CWeaponShotgun::switch2_AddCartgidge()
{
    if (!m_bOpenWeaponEmptyCartridge && isHUDAnimationExist("anm_add_cartridge_empty") && m_sounds.FindSoundItem("sndAddCartridgeEmpty", false) && m_ammoElapsed.type1 == 0)
        PlaySound("sndAddCartridgeEmpty", get_LastFP());
    else if (m_bOpenWeaponEmptyCartridge && isHUDAnimationExist("anm_add_cartridge_empty") && m_sounds.FindSoundItem("sndAddCartridgeEmpty", false) && m_ammoElapsed.type1 == 1)
        PlaySound("sndAddCartridgeEmpty", get_LastFP());
    else
        PlaySound("sndAddCartridge", get_LastFP());

    PlayAnimAddOneCartridgeWeapon();
    SetPending(TRUE);
}

void CWeaponShotgun::switch2_EndReload()
{
	if (isHUDAnimationExist("anm_close_empty") && m_sounds.FindSoundItem("sndCloseEmpty", false) && m_ammoElapsed.type1 == 0)
        PlaySound("sndCloseEmpty", get_LastFP());
    else 
        PlaySound("sndClose", get_LastFP());

    SetPending(FALSE);
    PlayAnimCloseWeapon();
}

void CWeaponShotgun::switch2_UnMisfire()
{
    if (m_sounds_enabled)
    {
        if (m_sounds.FindSoundItem("sndReloadMisfire", false) && isHUDAnimationExist("anm_reload_misfire"))
            PlaySound("sndReloadMisfire", get_LastFP());
        else
            PlaySound("sndAddCartridge", get_LastFP());
    }
    PlayAnimUnMisfire();
    SetPending(TRUE);
}

void CWeaponShotgun::PlayAnimOpenWeapon()
{
    VERIFY(GetState() == eReload);

	if (isHUDAnimationExist("anm_open_empty") && m_ammoElapsed.type1 == 0)
        PlayHUDMotion("anm_open_empty", FALSE, this, GetState());
    else
        PlayHUDMotion("anm_open", FALSE, this, GetState());
}
void CWeaponShotgun::PlayAnimAddOneCartridgeWeapon()
{
    VERIFY(GetState() == eReload);

	if (!m_bOpenWeaponEmptyCartridge && isHUDAnimationExist("anm_add_cartridge_empty") && m_ammoElapsed.type1 == 0)
        PlayHUDMotion("anm_add_cartridge_empty", FALSE, this, GetState());
    else if (m_bOpenWeaponEmptyCartridge && isHUDAnimationExist("anm_add_cartridge_empty") && m_ammoElapsed.type1 == 1)
        PlayHUDMotion("anm_add_cartridge_empty", FALSE, this, GetState());
    else
        PlayHUDMotion("anm_add_cartridge", FALSE, this, GetState());
}
void CWeaponShotgun::PlayAnimCloseWeapon()
{
    VERIFY(GetState() == eReload);

	if (isHUDAnimationExist("anm_close_empty") && m_ammoElapsed.type1 == 0)
        PlayHUDMotion("anm_close_empty", FALSE, this, GetState());
    else
        PlayHUDMotion("anm_close", FALSE, this, GetState());
}

void CWeaponShotgun::PlayAnimUnMisfire()
{
    VERIFY(GetState() == eUnMisfire);

    if (isHUDAnimationExist("anm_reload_misfire") && m_ammoElapsed.type1 > 0)
        PlayHUDMotion("anm_reload_misfire", TRUE, this, GetState());
    else
        PlayHUDMotion("anm_add_cartridge", FALSE, this, GetState());
}

u8 CWeaponShotgun::AddCartridge(u8 cnt)
{
    if (IsMisfire())
        bMisfire = false;

    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
    {
        m_ammoType.type1 = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = undefined_ammo_type;
    }

    if (!HaveCartridgeInInventory(1))
        return 0;

    m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType.type1].c_str()));
    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());


	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType.type1)
		m_DefaultCartridge.Load(m_ammoTypes[m_ammoType.type1].c_str(), m_ammoType.type1, m_APk);

    CCartridge l_cartridge = m_DefaultCartridge;
    while (cnt)
    {
        if (!unlimited_ammo())
        {
            if (!m_pCurrentAmmo->Get(l_cartridge))
                break;
        }
        --cnt;
        ++m_ammoElapsed.type1;
        l_cartridge.m_LocalAmmoType = m_ammoType.type1;
        m_magazine.push_back(l_cartridge);
        //		m_fCurrentCartirdgeDisp = l_cartridge.m_kDisp;
    }

    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());

    //выкинуть коробку патронов, если она пустая
    if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
        m_pCurrentAmmo->SetDropManual(TRUE);

    return cnt;
}

void CWeaponShotgun::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    P.w_u8(u8(m_magazine.size()));
    for (u32 i = 0; i < m_magazine.size(); i++)
    {
        CCartridge& l_cartridge = *(m_magazine.begin() + i);
        P.w_u8(l_cartridge.m_LocalAmmoType);
    }
}

void CWeaponShotgun::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
    u8 AmmoCount = P.r_u8();
    for (u32 i = 0; i < AmmoCount; i++)
    {
        u8 LocalAmmoType = P.r_u8();
        if (i >= m_magazine.size())
            continue;
        CCartridge& l_cartridge = *(m_magazine.begin() + i);
        if (LocalAmmoType == l_cartridge.m_LocalAmmoType)
            continue;
#ifdef DEBUG
        Msg("! %s reload to %s", *l_cartridge.m_ammoSect, m_ammoTypes[LocalAmmoType].c_str());
#endif
		l_cartridge.Load( m_ammoTypes[LocalAmmoType].c_str(), LocalAmmoType, m_APk );
	}
}
