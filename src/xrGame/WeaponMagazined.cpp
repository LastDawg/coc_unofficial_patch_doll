#include "pch_script.h"

#include "WeaponMagazined.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "inventory.h"
#include "InventoryOwner.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "UIGameCustom.h"
#include "Common/object_broker.h"
#include "string_table.h"
#include "ui/UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "HudSound.h"
#include "XrayGameConstants.h"

CWeaponMagazined::CWeaponMagazined(ESoundTypes eSoundType) : CWeapon()
{
    m_eSoundShow = ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
    m_eSoundHide = ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
    m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
    m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
    m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#ifdef NEW_SOUNDS
    m_eSoundReloadEmpty = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#endif
    m_sounds_enabled = true;

    m_sSndShotCurrent = nullptr;
    m_sSilencerFlameParticles = m_sSilencerSmokeParticles = nullptr;

    m_bFireSingleShot = false;
    m_iShotNum = 0;
    bullet_cnt = 0;
    m_fOldBulletSpeed = 0.f;
    m_iQueueSize = WEAPON_ININITE_QUEUE;
    m_bLockType = false;
    m_bHasDifferentFireModes = false;
    m_iCurFireMode = -1;
    m_iPrefferedFireMode = -1;

	m_bNeedBulletInGun = false;
    bHasBulletsToHide = false;
    m_bHasDistantShotSound = false;
    m_bLaserShaderOn = false;
}

CWeaponMagazined::~CWeaponMagazined()
{
    // sounds
}

void CWeaponMagazined::net_Destroy() { inherited::net_Destroy(); }

//AVO: for custom added sounds check if sound exists
bool CWeaponMagazined::WeaponSoundExist(pcstr section, pcstr sound_name) const
{
    pcstr str;
    bool sec_exist = process_if_exists_set(section, sound_name, &CInifile::r_string, str, true);
    if (sec_exist)
        return true;
#ifdef DEBUG
    Msg("~ [WARNING] ------ Sound [%s] does not exist in [%s]", sound_name, section);
#endif
    return false;
}

//-AVO

void CWeaponMagazined::Load(LPCSTR section)
{
    inherited::Load(section);

    // Sounds
    m_sounds.LoadSound(section, "snd_draw", "sndShow", false, m_eSoundShow);
    m_sounds.LoadSound(section, "snd_holster", "sndHide", false, m_eSoundHide);

    //Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
    m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", true, m_eSoundShot);
#else
    m_sounds.LoadSound(section, "snd_shoot", "sndShot", true, m_eSoundShot);  //Alundaio: Set exclusive to true
#endif
    //-Alundaio

	if (pSettings->line_exist(section, "snd_shoot_distant")) // distant sound
    {
        m_sounds.LoadSound(section, "snd_shoot_distant", "sndShotDist", false, m_eSoundShot);
        m_bHasDistantShotSound = true;
    }
    if (pSettings->line_exist(section, "snd_shoot_distant_far")) // distant sound
    {
        m_sounds.LoadSound(section, "snd_shoot_distant_far", "sndShotDistFar", false, m_eSoundShot);
        m_bHasDistantShotSound = true;
    }

    m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", false, m_eSoundEmptyClick);
    m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_reflect", "sndReflect", true, m_eSoundReflect);

	if (WeaponSoundExist(section, "snd_ammo_check"))
        m_sounds.LoadSound(section, "snd_ammo_check", "sndAmmoCheck", false, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_ammo_check_empty"))
        m_sounds.LoadSound(section, "snd_ammo_check_empty", "sndAmmoCheckEmpty", false, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_ammo_check_jammed"))
        m_sounds.LoadSound(section, "snd_ammo_check_jammed", "sndAmmoCheckJammed", false, m_eSoundEmptyClick);
	if (WeaponSoundExist(section, "snd_weapon_jam"))
        m_sounds.LoadSound(section, "snd_weapon_jam", "sndWpnJam", false, m_eSoundEmptyClick);
	if (WeaponSoundExist(section, "snd_changefiremode"))
        m_sounds.LoadSound(section, "snd_changefiremode", "sndFireModes", false, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_addon_switch"))
        m_sounds.LoadSound(section, "snd_addon_switch", "sndAddonSwitch", false, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_pump_gun"))
        m_sounds.LoadSound(section, "snd_pump_gun", "sndPumpGun", true, m_eSoundShot);
    if (WeaponSoundExist(section, "snd_pump_gun_l"))
        m_sounds.LoadSound(section, "snd_pump_gun_l", "sndPumpGunLast", true, m_eSoundShot);
    if (WeaponSoundExist(section, "snd_draw_empty"))
        m_sounds.LoadSound(section, "snd_draw_empty", "sndShowEmpty", false, m_eSoundShowEmpty);
    if (WeaponSoundExist(section, "snd_holster_empty"))
        m_sounds.LoadSound(section, "snd_holster_empty", "sndHideEmpty", false, m_eSoundHideEmpty);
    if (WeaponSoundExist(section, "snd_aim_start"))
        m_sounds.LoadSound(section, "snd_aim_start", "sndAimStart", true, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_aim_end"))
        m_sounds.LoadSound(section, "snd_aim_end", "sndAimEnd", true, m_eSoundEmptyClick);
    if (WeaponSoundExist(section, "snd_tape"))
        m_sounds.LoadSound(section, "snd_tape", "sndTape", true, m_eSoundShot);
    if (WeaponSoundExist(section, "snd_shoot_auto"))
        m_sounds.LoadSound(section, "snd_shoot_auto", "sndShot_a", false, m_eSoundShot);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    if (WeaponSoundExist(section, "snd_reload_empty"))
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    if (WeaponSoundExist(section, "snd_reload_misfire"))
        m_sounds.LoadSound(section, "snd_reload_misfire", "sndReloadMisfire", true, m_eSoundReloadMisfire);
#endif //-NEW_SOUNDS

    m_sSndShotCurrent = IsSilencerAttached() ? "sndSilencerShot" : "sndShot";

    //звуки и партиклы глушителя, если такой есть
    if (m_eSilencerStatus == ALife::eAddonAttachable || m_eSilencerStatus == ALife::eAddonPermanent)
    {
        if (pSettings->line_exist(section, "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
        if (pSettings->line_exist(section, "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");

    //Alundaio: LAYERED_SND_SHOOT Silencer
 #ifdef LAYERED_SND_SHOOT
        m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
        if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
            m_layered_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
 #else
        m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
        if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
            m_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
 #endif
    //-Alundaio
    }

    m_iBaseDispersionedBulletsCount = READ_IF_EXISTS(pSettings, r_u8, section, "base_dispersioned_bullets_count", 0);
    m_fBaseDispersionedBulletsSpeed = READ_IF_EXISTS(pSettings, r_float, section, "base_dispersioned_bullets_speed", m_fStartBulletSpeed);

	if (pSettings->line_exist(section, "bullet_bones"))
    {
        bHasBulletsToHide = true;
        LPCSTR str = pSettings->r_string(section, "bullet_bones");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 bullet_bone_name;
            _GetItem(str, i, bullet_bone_name);
            bullets_bones.push_back(bullet_bone_name);
            bullet_cnt++;
        }
    }

    if (pSettings->line_exist(section, "fire_modes"))
    {
        m_bHasDifferentFireModes = true;
        shared_str FireModesList = pSettings->r_string(section, "fire_modes");
        int ModesCount = _GetItemCount(FireModesList.c_str());
        m_aFireModes.clear();

        for (int i = 0; i < ModesCount; i++)
        {
            string16 sItem;
            _GetItem(FireModesList.c_str(), i, sItem);
            m_aFireModes.push_back	((s8)atoi(sItem));
        }

        m_iCurFireMode = ModesCount - 1;
        m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
    }
    else
    {
        m_bHasDifferentFireModes = false;
    }
    LoadSilencerKoeffs();

	empty_click_layer = READ_IF_EXISTS(pSettings, r_string, *hud_sect, "empty_click_anm", nullptr);

    if (empty_click_layer)
    {
        empty_click_speed = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "empty_click_anm_speed", 1.f);
        empty_click_power = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "empty_click_anm_power", 1.f);
    }

    m_bCartridgeInTheChamber = READ_IF_EXISTS(pSettings, r_bool, section, "CartridgeInTheChamberEnabled", true);
}

bool CWeaponMagazined::UseScopeTexture()
{
	return bScopeIsHasTexture;
}

void CWeaponMagazined::FireStart()
{
    if (!IsMisfire() && (!strstr(Core.Params, "-dev") || !strstr(Core.Params, "-dbg") && GetCondition() > 0.0f))
    { 
        if (IsValid())
        {
            if (!IsWorking() || AllowFireWhileWorking())
            {
                if (GetState() == eReload) return;
                if (GetState() == eShowing) return;
                if (GetState() == eHiding) return;
                if (GetState() == eMisfire) return;
                if (GetState() == eUnMisfire) return;
                if (GetState() == eFiremodePrev) return;
                if (GetState() == eFiremodeNext) return;
                if (GetState() == eSwitchAddon) return;
                if (GetState() == eAmmoCheck) return;

                inherited::FireStart();
				
				if (m_ammoElapsed.type1 == 0)
                    OnMagazineEmpty();
                else
                {
                    R_ASSERT(H_Parent());
                    SwitchState(eFire);
                }
            }
        }
        else
        {
            if (eReload != GetState())
                OnMagazineEmpty();
        }
    }
    else // misfire
    {
#ifdef EXTENDED_WEAPON_CALLBACKS //Alundaio
        IGameObject	*object = smart_cast<IGameObject*>(H_Parent());
        if (object)
            object->callback(GameObject::eOnWeaponJammed)(object->lua_game_object(), this->lua_game_object());
#endif

        if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()) && !m_sounds.FindSoundItem("sndWpnJam", false))
            CurrentGameUI()->AddCustomStatic("gun_jammed", true);

        OnEmptyClick();
    }
}

void CWeaponMagazined::FireEnd() 
{ 
    inherited::FireEnd(); 

	if (GameConstants::GetAutoreload())
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());

        if (m_pInventory && !m_ammoElapsed.type1 && actor && GetState() != eReload)
            Reload();
    }
}

void CWeaponMagazined::Reload() { inherited::Reload(); TryReload(); }

bool CWeaponMagazined::TryReload()
{
    if (m_pInventory)
    {
        if (IsGameTypeSingle() && ParentIsActor())
        {
            int AC = GetSuitableAmmoTotal();
            Actor()->callback(GameObject::eWeaponNoAmmoAvailable)(lua_game_object(), AC);
        }

        m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType.type1].c_str()));

        if (IsMisfire() && m_ammoElapsed.type1 && isHUDAnimationExist("anm_reload_misfire"))
        {
            SetPending(true);
            SwitchState(eUnMisfire);
            return true;
        }
        else if (IsMisfire() && m_ammoElapsed.type1 && !isHUDAnimationExist("anm_reload_misfire"))
        {
            SetPending(true);
            SwitchState(eReload);
            return true;
        }

        if (m_pCurrentAmmo || unlimited_ammo())
        {
            SetPending(true);
            SwitchState(eReload);
            return true;
        }
        else for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
        {
            m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
            if (m_pCurrentAmmo)
            {
                m_set_next_ammoType_on_reload = i;
                SetPending(true);
                SwitchState(eReload);
                return true;
            }
        }
    }

    if (GetState() != eIdle)
        SwitchState(eIdle);

    return false;
}

bool CWeaponMagazined::IsAmmoAvailable()
{
    if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType.type1].c_str())))
        return (true);
    else
        for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str())))
                return (true);
    return (false);
}

void CWeaponMagazined::OnMagazineEmpty()
{
#ifdef EXTENDED_WEAPON_CALLBACKS
    if (IsGameTypeSingle() && ParentIsActor())
    {
        int AC = GetSuitableAmmoTotal();
        Actor()->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
    }
#endif
    if (GetState() == eIdle)
    {
        OnEmptyClick();
        return;
    }

    if (GetNextState() != eMagEmpty && GetNextState() != eReload)
    {
        SwitchState(eMagEmpty);
    }

    inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo)
{
    last_hide_bullet = -1;
    HUD_VisualBulletUpdate();

    xr_map<LPCSTR, u16> l_ammo;

    while (!m_magazine.empty())
    {
        CCartridge& l_cartridge = m_magazine.back();
        xr_map<LPCSTR, u16>::iterator l_it;
        for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
        {
            if (!xr_strcmp(*l_cartridge.m_ammoSect, l_it->first))
            {
                ++(l_it->second);
                break;
            }
        }

        if (l_it == l_ammo.end()) l_ammo[*l_cartridge.m_ammoSect] = 1;
        m_magazine.pop_back();
        --m_ammoElapsed.type1;
    }

    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());

#ifdef EXTENDED_WEAPON_CALLBACKS
    if (IsGameTypeSingle() && ParentIsActor())
    {
        int AC = GetSuitableAmmoTotal();
        Actor()->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
    }
#endif

    if (!spawn_ammo)
        return;

    xr_map<LPCSTR, u16>::iterator l_it;
    for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
    {
        if (m_pInventory)
        {
            CWeaponAmmo* l_pA = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(l_it->first));
            if (l_pA)
            {
                u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
                l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
                l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
            }
        }
        if (l_it->second && !unlimited_ammo()) SpawnAmmo(l_it->second, l_it->first);
    }

    //устранить осечку при разряжании при условии того, что это не ПГ
    if (IsMisfire() && IsGrenadeLauncherAttached() && !m_bGrenadeMode)
        bMisfire = false;
    else if (IsMisfire() && !IsGrenadeLauncherAttached())
        bMisfire = false;
}

int CWeaponMagazined::CheckAmmoBeforeReload(u8& v_ammoType)
{
    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
        v_ammoType = m_set_next_ammoType_on_reload;

    //Msg("Ammo type in next reload : %d", m_set_next_ammoType_on_reload);

    if (m_ammoTypes.size() <= v_ammoType)
    {
        Msg("Ammo type is wrong : %d", v_ammoType);
        return 0;
    }

    LPCSTR tmp_sect_name = m_ammoTypes[v_ammoType].c_str();

    if (!tmp_sect_name)
    {
        Msg("Sect name is wrong");
        return 0;
    }

    CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(tmp_sect_name));

    if (!ammo && !m_bLockType)
    {
        for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
        {
            //проверить патроны всех подходящих типов
            ammo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
            if (ammo)
            {
                v_ammoType = i;
                break;
            }
        }
    }

    //Msg("Ammo type %d", v_ammoType);

    return GetAmmoCount(v_ammoType);
}

void CWeaponMagazined::ReloadMagazine()
{
    m_BriefInfo_CalcFrame = 0;

    //устранить осечку при перезарядке при условии того, что это не ПГ
    if (IsMisfire() && IsGrenadeLauncherAttached() && !m_bGrenadeMode) 
        bMisfire = false;
    else if (IsMisfire() && !IsGrenadeLauncherAttached()) 
        bMisfire = false;

    if (!m_bLockType)
    {
        m_pCurrentAmmo = nullptr;
    }

    if (!m_pInventory) return;

    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
    {
        m_ammoType.type1 = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = undefined_ammo_type;
    }

    if (!unlimited_ammo())
    {
        if (m_ammoTypes.size() <= m_ammoType.type1)
            return;

        LPCSTR tmp_sect_name = m_ammoTypes[m_ammoType.type1].c_str();

        if (!tmp_sect_name)
            return;

        //попытаться найти в инвентаре патроны текущего типа
        m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(tmp_sect_name));

        if (!m_pCurrentAmmo && !m_bLockType)
        {
            for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
            {
                //проверить патроны всех подходящих типов
                m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
                if (m_pCurrentAmmo)
                {
                    m_ammoType.type1 = i;
                    break;
                }
            }
        }
    }

    //нет патронов для перезарядки
    if (!m_pCurrentAmmo && !unlimited_ammo()) return;

    //разрядить магазин, если загружаем патронами другого типа
    if (!m_bLockType && !m_magazine.empty() &&
        (!m_pCurrentAmmo || xr_strcmp(m_pCurrentAmmo->cNameSect(), *m_magazine.back().m_ammoSect)))
        UnloadMagazine();

    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());

    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType.type1)
		m_DefaultCartridge.Load(m_ammoTypes[m_ammoType.type1].c_str(), m_ammoType.type1, m_APk);

    CCartridge l_cartridge = m_DefaultCartridge;
    while (m_ammoElapsed.type1 < iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pCurrentAmmo->Get(l_cartridge)) break;
        }
        ++m_ammoElapsed.type1;
        l_cartridge.m_LocalAmmoType = m_ammoType.type1;
        m_magazine.push_back(l_cartridge);
    }

    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());

    //выкинуть коробку патронов, если она пустая
    if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
        m_pCurrentAmmo->SetDropManual(true);

    if (iMagazineSize > m_ammoElapsed.type1)
    {
        m_bLockType = true;
        ReloadMagazine();
        m_bLockType = false;
    }

    VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());
}

void CWeaponMagazined::OnStateSwitch(u32 S, u32 oldState)
{
    HUD_VisualBulletUpdate();
    inherited::OnStateSwitch(S, oldState);
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
    switch (S)
    {
    case eIdle: switch2_Idle(); break;
    case eFire: switch2_Fire(); break;
    case eMisfire: break;
    case eUnMisfire:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        switch2_Unmis();
        break;
    case eMagEmpty: 
        switch2_Empty(); 
        break;
    case eReload:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        switch2_Reload();
        break;
    case eShowing:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        switch2_Showing();
        break;
    case eHiding:
        if (owner)
            m_sounds_enabled = owner->CanPlayShHdRldSounds();
        if (oldState != eHiding && !psActorFlags.test(AF_FAST_WEAPON_SELECT))
            switch2_Hiding();
        else if (oldState != eHiding && psActorFlags.test(AF_FAST_WEAPON_SELECT))
        {
            OnZoomOut();
            SwitchState(eHidden);
        }
        break;
    case eHidden: 
        switch2_Hidden(); 
        break;
    case eFiremodeNext: {
        PlaySound("sndFireModes", get_LastFP());
        switch2_ChangeFireMode();
    }
    break;
    case eFiremodePrev: {
        PlaySound("sndFireModes", get_LastFP());
        switch2_ChangeFireMode();
    }
    break;
    case eSwitchAddon: {
        PlaySound("sndAddonSwitch", get_LastFP());
        switch2_AddonSwitch();
    }
    break;
    case eAmmoCheck: {
        switch2_AmmoCheck();
    }
    break;
    }
}

void CWeaponMagazined::UpdateCL()
{
    inherited::UpdateCL();
    float dt = Device.fTimeDelta;

    //когда происходит апдейт состояния оружия
    //ничего другого не делать
    if (GetNextState() == GetState())
    {
        switch (GetState())
        {
        case eShowing:
        case eHiding:
        case eReload:
        case eSprintStart:
        case eSprintEnd:
        case eIdle:
        {
            fShotTimeCounter -= dt;
            clamp(fShotTimeCounter, 0.0f, flt_max);
        }
        break;
        case eFire: 
        { 
            state_Fire(dt);
        }
        break;
        case eMisfire: state_Misfire(dt); break;
        case eMagEmpty: state_MagEmpty(dt); break;
        case eHidden: break;
        }
    }

    UpdateSounds();
}

void CWeaponMagazined::UpdateSounds()
{
    if (Device.dwFrame == dwUpdateSounds_Frame)
        return;

    dwUpdateSounds_Frame = Device.dwFrame;

    Fvector P = get_LastFP();
    m_sounds.SetPosition("sndShow", P);
    m_sounds.SetPosition("sndHide", P);
    m_sounds.SetPosition("sndShowEmpty", P);

    if (isHUDAnimationExist("anm_hide_empty") && WeaponSoundExist(m_section_id.c_str(), "snd_holster_empty"))
        m_sounds.SetPosition("sndHideEmpty", P);
    m_sounds.SetPosition("sndHideEmpty", P);
    if (isHUDAnimationExist("anm_show_empty") && WeaponSoundExist(m_section_id.c_str(), "snd_draw_empty"))
        m_sounds.SetPosition("sndShowEmpty", P);

    m_sounds.SetPosition("sndReload", P);

	if (WeaponSoundExist(m_section_id.c_str(), "snd_changefiremode"))
        m_sounds.SetPosition("sndFireModes", P);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    if (m_sounds.FindSoundItem("sndReloadEmpty", false))
        m_sounds.SetPosition("sndReloadEmpty", P);
#endif //-NEW_SOUNDS

    //. nah	m_sounds.SetPosition("sndEmptyClick", P);
}

void CWeaponMagazined::state_Fire(float dt)
{
    if (m_ammoElapsed.type1 > 0)
    {
        if (!H_Parent())
        {
            StopShooting();
            return;
        }



        CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
        if (!io->inventory().ActiveItem())
        {
            StopShooting();
            return;
        }

        CEntity* E = smart_cast<CEntity*>(H_Parent());
        if (!E->g_stateFire())
        {
            StopShooting();
            return;
        }

        Fvector p1, d;
        p1.set(get_LastFP());
        d.set(get_LastFD());

        E->g_fireParams(this, p1, d);

        if (m_iShotNum == 0)
        {
            m_vStartPos = p1;
            m_vStartDir = d;
        };

        VERIFY(!m_magazine.empty());

        while (!m_magazine.empty() && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
        {
#ifndef COC_EDITION
            if (CheckForMisfire())
            {
                StopShooting();
                return;
            }
#endif

            m_bFireSingleShot = false;

            //Alundaio: Use fModeShotTime instead of fOneShotTime if current fire mode is 2-shot burst
            //Alundaio: Cycle down RPM after two shots; used for Abakan/AN-94
            if (GetCurrentFireMode() == 2 || (cycleDown == true && m_iShotNum < 1) || IsDiffShotModes() && GetCurrentFireMode() == -1)
                fShotTimeCounter = modeShotTime;
            else
                fShotTimeCounter = fOneShotTime;
            //Alundaio: END

            ++m_iShotNum;

            OnShot();

            if (m_iShotNum > m_iBaseDispersionedBulletsCount)
                FireTrace(p1, d);
            else
                FireTrace(m_vStartPos, m_vStartDir);

#ifdef COC_EDITION
            if (CheckForMisfire())
            {
                StopShooting();
                return;
            }
#endif
        }

        if (m_iShotNum == m_iQueueSize)
            m_bStopedAfterQueueFired = true;

        UpdateSounds();
    }

    if (fShotTimeCounter < 0)
    {
        /*
                if(bDebug && H_Parent() && (H_Parent()->ID() != Actor()->ID()))
                {
                    Msg("stop shooting w=[%s] magsize=[%d] sshot=[%s] qsize=[%d] shotnum=[%d]",
                            IsWorking()?"true":"false",
                            m_magazine.size(),
                            m_bFireSingleShot?"true":"false",
                            m_iQueueSize,
                            m_iShotNum);
                }
        */
        if (m_ammoElapsed.type1 == 0)
            OnMagazineEmpty();

        StopShooting();
    }
    else
    {
        fShotTimeCounter -= dt;
    }
}

void CWeaponMagazined::state_Misfire(float dt)
{
    OnEmptyClick();
    SwitchState(eIdle);

    bMisfire = true;

    UpdateSounds();
}

void CWeaponMagazined::state_MagEmpty(float dt) {}

void CWeaponMagazined::SetDefaults() 
{ 
    CWeapon::SetDefaults(); 
}

void CWeaponMagazined::OnShot()
{
    if (IsOutScopeAfterShot()) // Принудительно выходим из зума, если out_scope_after_shot = true (для болтовок)
        OnZoomOut();

    //Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
    if (m_bHasDistantShotSound && !IsSilencerAttached() && GameConstants::GetWeaponDistantSounds() && Position().distance_to(Device.vCameraPosition) > GameConstants::GetWeaponSoundDist() && Position().distance_to(Device.vCameraPosition) < GameConstants::GetWeaponSoundDistFar())
        PlaySound("sndShotDist", get_LastFP());
    else if (m_bHasDistantShotSound && !IsSilencerAttached() && GameConstants::GetWeaponDistantSounds() && Position().distance_to(Device.vCameraPosition) > GameConstants::GetWeaponSoundDistFar())
        PlaySound("sndShotDistFar", get_LastFP());
    else
        m_layered_sounds.PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
#else
    if (m_bHasDistantShotSound && GameConstants::GetWeaponDistantSounds() && Position().distance_to(Device.vCameraPosition) > GameConstants::GetWeaponSoundDist() && Position().distance_to(Device.vCameraPosition) < GameConstants::GetWeaponSoundDistFar())
        PlaySound("sndShotDist", get_LastFP());
    else if (m_bHasDistantShotSound && GameConstants::GetWeaponDistantSounds() && Position().distance_to(Device.vCameraPosition) > GameConstants::GetWeaponSoundDistFar())
        PlaySound("sndShotDistFar", get_LastFP());
    else
        PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), (u8)-1); //Alundaio: Play sound at index (ie. snd_shoot, snd_shoot1, snd_shoot2, snd_shoot3)
#endif

    // Звук лязгающей ленты для пулемёта
    if (m_sounds.FindSoundItem("sndTape", false) && m_ammoElapsed.type1 > 3)
    {
        PlaySound("sndTape", get_LastFP());
    }

    //-Alundaio

    // Camera
    AddShotEffector();

    // Animation
    PlayAnimShoot();

    // Bullets to hide
    HUD_VisualBulletUpdate();

    // Shell Drop
    Fvector vel;

    if (!IsMotionMarkShell())
    {
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }

    // Огонь из ствола
    StartFlameParticles();

    //дым из ствола
    ForceUpdateFireParticles();
    StartSmokeParticles(get_LastFP(), vel);

	if (IsSilencerAttached() == false)
    {
        bool bIndoor = false;
        if (H_Parent() != nullptr)
        {
            bIndoor = H_Parent()->renderable_ROS()->get_luminocity_hemi() < WEAPON_INDOOR_HEMI_FACTOR;
        }

        if (bIndoor && m_sounds.FindSoundItem("sndReflect", false))
        {
            if (IsHudModeNow())
            {
                HUD_SOUND_ITEM::SetHudSndGlobalVolumeFactor(WEAPON_SND_REFLECTION_HUD_FACTOR);
            }
            PlaySound("sndReflect", get_LastFP());
            // m_sSndShotCurrent = "sndReflect";
            HUD_SOUND_ITEM::SetHudSndGlobalVolumeFactor(1.0f);
        }
        // else
        // m_sSndShotCurrent = "sndShot";
    }

    // Эффект сдвига (отдача)
    AddHUDShootingEffect();

    // Передёргивание затвора отдельным звуком
    if (!IsDiffShotModes() || IsDiffShotModes() && GetCurrentFireMode() == 1)
    {
        if (m_sounds.FindSoundItem("sndPumpGun", false) && m_ammoElapsed.type1 > 1)
            PlaySound("sndPumpGun", get_LastFP());
        else if (m_sounds.FindSoundItem("sndPumpGunLast", false) && isHUDAnimationExist("anm_shot_l") &&
            m_ammoElapsed.type1 == 1)
            PlaySound("sndPumpGunLast", get_LastFP());
        else
            PlaySound("sndPumpGun", get_LastFP());
    }

#ifdef EXTENDED_WEAPON_CALLBACKS
	IGameObject	*object = smart_cast<IGameObject*>(H_Parent());
	if (object)
        object->callback(GameObject::eOnWeaponFired)(object->lua_game_object(), this->lua_game_object(), m_ammoElapsed.data, m_ammoType.data);
#endif
}

void CWeaponMagazined::OnEmptyClick() 
{ 
    float random_voice;
    random_voice = Random.randF(0.0f, 1.0f);

    PlaySound("sndEmptyClick", get_LastFP()); // Рычаг спуска вхолостую

    if (m_sounds.FindSoundItem("sndWpnJam", false) && bMisfire) // С шансом в 30% играем фразу при клине оружия
        if (random_voice > 0.7f)
            PlaySound("sndWpnJam", get_LastFP());

	if (empty_click_layer)
        PlayBlendAnm(empty_click_layer, empty_click_speed, empty_click_power);
}

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eReload: {
        CheckMagazine(); // Основано на механизме из Lost Alpha: New Project // Авторы: rafa & Kondr48

        CCartridge FirstBulletInGun;

        bool bNeedputBullet = m_ammoElapsed.type1 > 0;

        if (m_bNeedBulletInGun && bNeedputBullet)
        {
            FirstBulletInGun = m_magazine.back();
            m_magazine.pop_back();
            --m_ammoElapsed.type1;
        }

        ReloadMagazine();

        if (m_bNeedBulletInGun && bNeedputBullet)
        {
            m_magazine.push_back(FirstBulletInGun);
            ++m_ammoElapsed.type1;
        }

        SwitchState(eIdle);
    }
    break;

    case eHiding:	
        SwitchState(eHidden);   
    break;
    case eShowing:	
        SwitchState(eIdle);		
    break;
    case eIdle:		
        switch2_Idle();			
    break;
    case eUnMisfire: {
        bMisfire = false;
        // Здесь -1 патрон при расклине, ставим в условие опцию из конфига, чтобы имелось оружие, не сбрасывающее патрон
        if (IsMisfireOneCartRemove() && m_ammoElapsed.type1 > 0 && isHUDAnimationExist("anm_reload_empty"))
        {
            --m_ammoElapsed.type1;
            m_magazine.pop_back();
        }

        SwitchState(eIdle);
    }
    break;
    case eFiremodePrev: {
        SwitchState(eIdle);
        break;
    }
    case eFiremodeNext: {
        SwitchState(eIdle);
        break;
    } 
    case eSwitchAddon: {
        SwitchState(eIdle);
        break;
    }
    case eAmmoCheck: {
        SwitchState(eIdle);

        if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
        {
            if (m_ammoElapsed.type1 >= iMagazineSize)
                CurrentGameUI()->AddCustomStatic("weapon_ammo_check_full", true);
            else if (m_ammoElapsed.type1 >= iMagazineSize / 2)
                CurrentGameUI()->AddCustomStatic("weapon_ammo_check_over_half", true);
            else if (m_ammoElapsed.type1 != 0 && (m_ammoElapsed.type1 < iMagazineSize / 2))
                CurrentGameUI()->AddCustomStatic("weapon_ammo_check_under_half", true);
            else if (m_ammoElapsed.type1 == 0)
                CurrentGameUI()->AddCustomStatic("weapon_ammo_check_empty", true);    
        }

        break;
    } // End
    }
    inherited::OnAnimationEnd(state);
}

void CWeaponMagazined::switch2_Idle()
{
    m_iShotNum = 0;
    if (m_fOldBulletSpeed != 0.f)
        SetBulletSpeed(m_fOldBulletSpeed);

    SetPending(false);
    PlayAnimIdle();
}

void CWeaponMagazined::switch2_ChangeFireMode()
{
    if (GetState() != eFiremodeNext && GetState() != eFiremodePrev)
        return;

    FireEnd();
    PlayAnimFireMode();
    SetPending(TRUE);
}

void CWeaponMagazined::switch2_AddonSwitch()
{
    if (GetState() != eSwitchAddon)
        return;

    FireEnd();
    PlayAnimAddonSwitch();
    SetPending(TRUE);
}

void CWeaponMagazined::switch2_AmmoCheck()
{
    if (GetState() != eAmmoCheck)
        return;

    FireEnd();
    PlayAnimAmmoCheck();
    SetPending(TRUE);
}

void CWeaponMagazined::PlayAnimFireMode()
{
    if (!isHUDAnimationExist("anm_changefiremode")) // Если нет базовой анимации, не виснем в состоянии
        SwitchState(eIdle);

    if (isHUDAnimationExist("anm_changefiremode"))
    {
        if (!IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_changefiremode", true, this, GetState());
        if (isHUDAnimationExist("anm_changefiremode_empty") && !IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_changefiremode_empty", true, this, GetState());

        if (isHUDAnimationExist("anm_changefiremode_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_changefiremode_w_gl", true, this, GetState());
        if (isHUDAnimationExist("anm_changefiremode_empty_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_changefiremode_empty_w_gl", true, this, GetState());

        if (isHUDAnimationExist("anm_changefiremode_jammed") && !IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_changefiremode_jammed", true, this, GetState());
        if (isHUDAnimationExist("anm_changefiremode_empty_jammed") && !IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_changefiremode_empty_jammed", true, this, GetState());

        if (isHUDAnimationExist("anm_changefiremode_jammed_w_gl") && IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_changefiremode_jammed_w_gl", true, this, GetState());
        if (isHUDAnimationExist("anm_changefiremode_empty_jammed_w_gl") && IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_changefiremode_empty_jammed_w_gl", true, this, GetState());
    }
}

void CWeaponMagazined::PlayAnimAddonSwitch() // Для лазера/фонаря
{
    if (!isHUDAnimationExist("anm_addon_switch")) // Если нет базовой анимации, не виснем в состоянии
        SwitchState(eIdle);

    if (isHUDAnimationExist("anm_addon_switch"))
    {
        if (!IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_addon_switch", true, this, GetState());
        if (isHUDAnimationExist("anm_addon_switch_empty") && !IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_addon_switch_empty", true, this, GetState());

        if (isHUDAnimationExist("anm_addon_switch_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_addon_switch_w_gl", true, this, GetState());
        if (isHUDAnimationExist("anm_addon_switch_empty_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_addon_switch_empty_w_gl", true, this, GetState());

        if (isHUDAnimationExist("anm_addon_switch_jammed") && !IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_addon_switch_jammed", true, this, GetState());
        if (isHUDAnimationExist("anm_addon_switch_empty_jammed") && !IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_addon_switch_empty_jammed", true, this, GetState());

        if (isHUDAnimationExist("anm_addon_switch_jammed_w_gl") && IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_addon_switch_jammed_w_gl", true, this, GetState());
        if (isHUDAnimationExist("anm_addon_switch_empty_jammed_w_gl") && IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_addon_switch_empty_jammed_w_gl", true, this, GetState());
    }
}

void CWeaponMagazined::PlayAnimAmmoCheck()
{
    if (!isHUDAnimationExist("anm_ammo_check"))
        SwitchState(eIdle);

    if (isHUDAnimationExist("anm_ammo_check"))
    {
        if (!IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_ammo_check", true, this, GetState());
        if (isHUDAnimationExist("anm_ammo_check_empty") && !IsMisfire() && !IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_ammo_check_empty", true, this, GetState());

        if (isHUDAnimationExist("anm_ammo_check_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_ammo_check_w_gl", true, this, GetState());
        if (isHUDAnimationExist("anm_ammo_check_empty_w_gl") && !IsMisfire() && IsGrenadeLauncherAttached() && m_ammoElapsed.type1 == 0)
            PlayHUDMotion("anm_ammo_check_empty_w_gl", true, this, GetState());

        if (isHUDAnimationExist("anm_ammo_check_jammed") && !IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_ammo_check_jammed", true, this, GetState());
        if (isHUDAnimationExist("anm_ammo_check_jammed_w_gl") && IsGrenadeLauncherAttached() && IsMisfire() && m_ammoElapsed.type1 > 0)
            PlayHUDMotion("anm_ammo_check_jammed_w_gl", true, this, GetState());
    }
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire()
{
    CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
    CInventoryItem* ii = smart_cast<CInventoryItem*>(this);
#ifdef DEBUG
    if (!io)
        return;
    // VERIFY2					(io,make_string("no inventory owner, item %s",*cName()));

    if (ii != io->inventory().ActiveItem())
        Msg("! not an active item, item %s, owner %s, active item %s", *cName(), *H_Parent()->cName(),
            io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

    if (!(io && (ii == io->inventory().ActiveItem())))
    {
        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(H_Parent());
        if (stalker)
        {
            stalker->planner().show();
            stalker->planner().show_current_world_state();
            stalker->planner().show_target_world_state();
        }
    }
#else
    if (!io)
        return;
#endif // DEBUG

    m_bStopedAfterQueueFired = false;
    m_bFireSingleShot = true;
    m_iShotNum = 0;

    if ((OnClient()) && !IsWorking())
        FireStart();
}

void CWeaponMagazined::switch2_Empty()
{
    if (GameConstants::GetAutoreload())
    {
        if (!TryReload())
        {
            OnEmptyClick();
        }
        else
        {
            inherited::FireEnd();
        }
    }
    else
    {
        OnZoomOut();
        OnEmptyClick();
    }
}
void CWeaponMagazined::PlayReloadSound()
{
    if (m_sounds_enabled)
    {
#ifdef NEW_SOUNDS //AVO: use custom sounds
        if (bMisfire)
        {
            //TODO: make sure correct sound is loaded in CWeaponMagazined::Load(LPCSTR section)
            if (m_sounds.FindSoundItem("sndReloadMisfire", false))
                PlaySound("sndReloadMisfire", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }
        else
        {
            if (m_ammoElapsed.type1 == 0)
                if (m_sounds.FindSoundItem("sndReloadEmpty", false))
                    PlaySound("sndReloadEmpty", get_LastFP());
                else
                    PlaySound("sndReload", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }
#else
        PlaySound("sndReload", get_LastFP());
#endif //-AVO
    }
}

void CWeaponMagazined::switch2_Reload()
{
    CWeapon::FireEnd();

    PlayReloadSound();
    PlayAnimReload();
    SetPending(true);
}

void CWeaponMagazined::switch2_Unmis()
{
    VERIFY(GetState() == eUnMisfire);

    if (m_sounds_enabled)
    {
        if (m_sounds.FindSoundItem("sndReloadMisfire", false) && isHUDAnimationExist("anm_reload_misfire"))
            PlaySound("sndReloadMisfire", get_LastFP());
        else if (m_sounds.FindSoundItem("sndReloadEmpty", false) && isHUDAnimationExist("anm_reload_empty"))
            PlaySound("sndReloadEmpty", get_LastFP());
        else
            PlaySound("sndReload", get_LastFP());
    }

    if (isHUDAnimationExist("anm_reload_misfire"))
        PlayHUDMotion("anm_reload_misfire", TRUE, this, GetState());
    else if (isHUDAnimationExist("anm_reload_empty"))
        PlayHUDMotion("anm_reload_empty", TRUE, this, GetState());
    else
        PlayHUDMotion("anm_reload", TRUE, this, GetState());
}

void CWeaponMagazined::switch2_Hiding()
{
    OnZoomOut();
    CWeapon::FireEnd();

	if (m_sounds_enabled)
    {
        if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_hide_empty") && WeaponSoundExist(m_section_id.c_str(), "snd_holster_empty"))
            PlaySound("sndHideEmpty", get_LastFP());
        else
            PlaySound("sndHide", get_LastFP());
    }

    PlayAnimHide();
    SetPending(true);
}

void CWeaponMagazined::switch2_Hidden()
{
    CWeapon::FireEnd();

    StopCurrentAnimWithoutCallback();

    signal_HideComplete();
    RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
    if (m_sounds_enabled)
    {
        if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_show_empty") && WeaponSoundExist(m_section_id.c_str(), "snd_draw_empty"))
            PlaySound("sndShowEmpty", get_LastFP());
        else
            PlaySound("sndShow", get_LastFP());
    }

    SetPending(true);
    PlayAnimShow();
}

bool CWeaponMagazined::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags)) return true;

    //если оружие чем-то занято, то ничего не делать
    if (IsPending()) return false;

    switch (cmd)
    {
    case kWPN_RELOAD:
    {
        if (flags & CMD_START)
            if (m_ammoElapsed.type1 < iMagazineSize || IsMisfire())
            {
                if (GetState() == eUnMisfire) // Rietmon: Запрещаем перезарядку, если играет анима передергивания затвора
                    return false;

                Reload();
            }
    }
        return true;
    case kWPN_FIREMODE_PREV:
    {
        if (flags & CMD_START)
        {
            OnPrevFireMode();
            return true;
        };
    }
    break;
    case kWPN_FIREMODE_NEXT:
    {
        if (flags & CMD_START)
        {
            OnNextFireMode();
            return true;
        };
    }
    break;
    case kWPN_ADDON_LASER: 
    {
        if (flags & CMD_START)
        {
            if (!IsLaserAttached())
                return false;

            OnWeaponAddonLaserSwitch();
            return true;
        };
    }
    break;
    case kWPN_ADDON_FLASHLIGHT: 
    {
        if (flags & CMD_START)
        {
            if (!IsLaserAttached() || (IsLaserAttached() && !bLaserSupportFlashlight))
                return false;

            OnWeaponAddonFlashlightSwitch();
            return true;
        };
    }
    break;
    case kWPN_AMMO_CHECK: {
        if (flags & CMD_START)
        {
            if (iMagazineSize == 0 || (IsGrenadeLauncherAttached() && m_bGrenadeMode))
                return false;

            OnWeaponAmmoCheck();
            return true;
        };
    }
    break;
    }
    return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
    CScope* pScope = smart_cast<CScope*>(pIItem);
    CSilencer* pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);
    CLaser* pLaser = smart_cast<CLaser*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (bUseAltScope)
            {
                if (*it == pIItem->object().cNameSect())
                    return true;
            }
            else
            {
                if (pSettings->r_string((*it), "scope_name") == pIItem->object().cNameSect())
                    return true;
            }
        }
        return false;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
    {
		// Если есть опция на запрет глушителя и ПГ одновременно
		if (!bGrenadeLauncherNSilencer || bGrenadeLauncherNSilencer && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
		{
            auto it = m_silencers.begin();
            for (; it != m_silencers.end(); it++)
            {
                if (pSettings->r_string((*it), "silencer_name") == pIItem->object().cNameSect())
                    return true;
            }
        }
        return false;
    }
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
    {
		// Если есть опция на запрет глушителя и ПГ одновременно
		if (!bGrenadeLauncherNSilencer || bGrenadeLauncherNSilencer && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
		{
            auto it = m_launchers.begin();
            for (; it != m_launchers.end(); it++)
            {
                if (pSettings->r_string((*it), "grenade_launcher_name") == pIItem->object().cNameSect())
                    return true;
            }
        }
        return false;
    }
    else if (pLaser && m_eLaserStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
    {
        auto it = m_lasers.begin();
        for (; it != m_lasers.end(); it++)
        {
            if (pSettings->r_string((*it), "laser_name") == pIItem->object().cNameSect())
                return true;
        }
        return false;
    }
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
    if (m_eScopeStatus == ALife::eAddonAttachable &&
        0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope))
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (bUseAltScope)
            {
                if (*it == item_section_name)
                    return true;
            }
            else
            {
                if (pSettings->r_string((*it), "scope_name") == item_section_name)
                    return true;
            }
        }
        return false;
    }
    //	   return true;
    else if (m_eSilencerStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer))
    {
        auto it = m_silencers.begin();
        for (; it != m_silencers.end(); it++)
        {
            if (pSettings->r_string((*it), "silencer_name") == item_section_name)
                return true;
        }
        return false;
    }
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher))
    {
        auto it = m_launchers.begin();
        for (; it != m_launchers.end(); it++)
        {
            if (pSettings->r_string((*it), "grenade_launcher_name") == item_section_name)
                return true;
        }
        return false;
    }
    else if (m_eLaserStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser))
    {
        auto it = m_lasers.begin();
        for (; it != m_lasers.end(); it++)
        {
            if (pSettings->r_string((*it), "laser_name") == item_section_name)
                return true;
        }
        return false;
    }
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
    bool result = false;

    CScope* pScope = smart_cast<CScope*>(pIItem);
    CSilencer* pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);
    CLaser* pLaser = smart_cast<CLaser*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable &&
        (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
        auto it = m_scopes.begin();
        for (; it != m_scopes.end(); it++)
        {
            if (bUseAltScope)
            {
                if (*it == pIItem->object().cNameSect())
                    m_cur_addon.scope = u16(it - m_scopes.begin());
            }
            else
            {
                if (pSettings->r_string((*it), "scope_name") == pIItem->object().cNameSect())
                    m_cur_addon.scope = u16(it - m_scopes.begin());
            }
        }
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
        result = true;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
    {
        auto it = m_silencers.begin();
        for (; it != m_silencers.end(); it++)
        {
            if (pSettings->r_string((*it), "silencer_name") == pIItem->object().cNameSect())
            {
                m_cur_addon.silencer = u16(it - m_silencers.begin());
                m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
                result = true;
            }
        }
    }
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
    {
        auto it = m_launchers.begin();
        for (; it != m_launchers.end(); it++)
        {
            if (pSettings->r_string((*it), "grenade_launcher_name") == pIItem->object().cNameSect())
            {
                m_cur_addon.launcher = u16(it - m_launchers.begin());
                m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
                result = true;
            }
        }
    }
    else if (pLaser && m_eLaserStatus == ALife::eAddonAttachable && (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
    {
        auto it = m_lasers.begin();
        for (; it != m_lasers.end(); it++)
        {
            if (pSettings->r_string((*it), "laser_name") == pIItem->object().cNameSect())
            {
                m_cur_addon.laser = u16(it - m_lasers.begin());
                m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonLaser;
                result = true;
            }
        }
    }

    if (result)
    {
        SyncronizeWeaponToServer();
        if (b_send_event && OnServer())
        {
            //уничтожить подсоединенную вещь из инвентаря
            //.			pIItem->Drop					();
            pIItem->object().DestroyObject();
        };

        if (pScope && bUseAltScope)
        {
            bNVsecondVPstatus = !!pSettings->line_exist(pIItem->object().cNameSect(), "scope_nightvision");
        }

		UpdateAltScope();
        UpdateAddonsVisibility();
        InitAddons();

        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::DetachScope(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
    shared_str iter_scope_name = "none";

    auto it = m_scopes.begin();

    for (; it != m_scopes.end(); it++)
    {
        if (bUseAltScope)
        {
            iter_scope_name = (*it);
        }
        else
        {
            iter_scope_name = pSettings->r_string((*it), "scope_name");
        }
        if (!xr_strcmp(iter_scope_name, item_section_name))
        {
            m_cur_addon.scope = 0;
            m_cur_scope_bone = NULL;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::DetachSilencer(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
    auto it = m_silencers.begin();
    for (; it != m_silencers.end(); it++)
    {
        LPCSTR iter_scope_name = pSettings->r_string((*it), "silencer_name");
        if (!xr_strcmp(iter_scope_name, item_section_name))
        {
            m_cur_addon.silencer = 0;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::DetachGrenadeLauncher(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
    auto it = m_launchers.begin();
    for (; it != m_launchers.end(); it++)
    {
        LPCSTR iter_scope_name = pSettings->r_string((*it), "grenade_launcher_name");
        if (!xr_strcmp(iter_scope_name, item_section_name))
        {
            m_cur_addon.launcher = 0;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::DetachLaser(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
    auto it = m_lasers.begin();
    for (; it != m_lasers.end(); it++)
    {
        LPCSTR iter_scope_name = pSettings->r_string((*it), "laser_name");
        if (!xr_strcmp(iter_scope_name, item_section_name))
        {
            m_cur_addon.laser = 0;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item)
{
    if (m_eScopeStatus == ALife::eAddonAttachable && DetachScope(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
        {
            Msg("ERROR: scope addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;

		UpdateAltScope();
        UpdateAddonsVisibility();
        InitAddons();
        SyncronizeWeaponToServer();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eSilencerStatus == ALife::eAddonAttachable && DetachSilencer(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
        {
            Msg("ERROR: silencer addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

        UpdateAddonsVisibility();
        InitAddons();
        SyncronizeWeaponToServer();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && DetachGrenadeLauncher(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
        {
            Msg("ERROR: grenade launcher addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

        UpdateAddonsVisibility();
        InitAddons();
        SyncronizeWeaponToServer();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eLaserStatus == ALife::eAddonAttachable && DetachLaser(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
        {
            Msg("ERROR: laser addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonLaser;

        UpdateAddonsVisibility();
        InitAddons();
        SyncronizeWeaponToServer();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item);;
}

void CWeaponMagazined::InitAddons()
{
    if (IsScopeAttached())
    {
        if (m_eScopeStatus == ALife::eAddonAttachable)
        {
            LoadCurrentScopeParams(GetScopeName().c_str());

			if (pSettings->line_exist(m_scopes[m_cur_addon.scope], "bones"))
            {
                pcstr ScopeBone = pSettings->r_string(m_scopes[m_cur_addon.scope], "bones");
                m_cur_scope_bone = ScopeBone;
            }
        }
    }
    else
    {
        if (m_UIScope)
        {
            xr_delete(m_UIScope);
        }

        // 3д прицелы
	    if (bIsSecondVPZoomPresent())
            m_zoom_params.m_fSecondVPFovFactor = 0.0f;

		m_fSecondRTZoomFactor = -1.0f;

        // Убираем дин. зум при убирании прицела
		if (IsZoomEnabled())
        {
            m_zoom_params.m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "scope_dynamic_zoom", FALSE);
        }
    }

    if (IsLaserAttached())
    {
        if (m_eLaserStatus == ALife::eAddonAttachable)
        {
            LoadCurrentLaserParams(GetLaserName().c_str());

            if (bLaserSupportFlashlight)
                LoadCurrentFlashlightParams(GetLaserName().c_str());
        }
    }
    if (m_eLaserStatus == ALife::eAddonAttachable && !IsLaserAttached())
    {
        if (HasFlashlight() && bLaserSupportFlashlight && m_bLaserShaderOn)
            g_pGamePersistent->laser_shader_data.laser_factor = 0.f;
            //m_bLaserShaderOn = false;
    }

    if (IsSilencerAttached() /* && SilencerAttachable() */)
    {
        m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
        m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
        m_sSndShotCurrent = "sndSilencerShot";

        //подсветка от выстрела
		LPCSTR light_section = m_eSilencerStatus == ALife::eAddonAttachable ? m_silencers[m_cur_addon.silencer].c_str() : cNameSect().c_str();
		LoadLights(light_section, "silencer_");
        ApplySilencerKoeffs();
    }
    else
    {
        m_sFlameParticlesCurrent = m_sFlameParticles;
        m_sSmokeParticlesCurrent = m_sSmokeParticles;
        m_sSndShotCurrent = "sndShot";

        //подсветка от выстрела
        LoadLights(cNameSect().c_str(), "");
        ResetSilencerKoeffs();
    }

    inherited::InitAddons();
}

void CWeaponMagazined::LoadSilencerKoeffs()
{
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        LPCSTR sect = GetSilencerName().c_str();
        m_silencer_koef.hit_power = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_power_k", 1.0f);
        m_silencer_koef.hit_impulse = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_impulse_k", 1.0f);
        m_silencer_koef.bullet_speed = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_speed_k", 1.0f);
        m_silencer_koef.fire_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "fire_dispersion_base_k", 1.0f);
        m_silencer_koef.cam_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
        m_silencer_koef.cam_disper_inc = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
    }

    clamp(m_silencer_koef.hit_power, 0.0f, 1.0f);
    clamp(m_silencer_koef.hit_impulse, 0.0f, 1.0f);
    clamp(m_silencer_koef.bullet_speed, 0.0f, 1.0f);
    clamp(m_silencer_koef.fire_dispersion, 0.0f, 3.0f);
    clamp(m_silencer_koef.cam_dispersion, 0.0f, 1.0f);
    clamp(m_silencer_koef.cam_disper_inc, 0.0f, 1.0f);
}

void CWeaponMagazined::ApplySilencerKoeffs() { cur_silencer_koef = m_silencer_koef; }
void CWeaponMagazined::ResetSilencerKoeffs() { cur_silencer_koef.Reset(); }

void CWeaponMagazined::PlayAnimShow()
{
    VERIFY(GetState() == eShowing);
    HUD_VisualBulletUpdate();

	if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_show_empty"))
        PlayHUDMotion("anm_show_empty", false, this, GetState());
    else if (IsMisfire() && isHUDAnimationExist("anm_show_jammed"))
        PlayHUDMotion("anm_show_jammed", false, this, GetState());
    else
        PlayHUDMotion("anm_show", false, this, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
    VERIFY(GetState() == eHiding);

	if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_hide_empty"))
        PlayHUDMotion("anm_hide_empty", true, this, GetState());
    else if (IsMisfire() && isHUDAnimationExist("anm_hide_jammed"))
        PlayHUDMotion("anm_hide_jammed", true, this, GetState());
    else
        PlayHUDMotion("anm_hide", true, this, GetState());
}

void CWeaponMagazined::PlayAnimBore()
{
    if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_bore_empty"))
        PlayHUDMotion("anm_bore_empty", true, this, GetState());
    else if (IsMisfire() && isHUDAnimationExist("anm_bore_jammed"))
        PlayHUDMotion("anm_bore_jammed", true, nullptr, GetState());
    else
        inherited::PlayAnimBore();
}

void CWeaponMagazined::PlayAnimIdleSprint()
{
    if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_sprint_empty"))
        PlayHUDMotion("anm_idle_sprint_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdleSprint();
}

void CWeaponMagazined::PlayAnimIdleMoving()
{
    if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_empty"))
        PlayHUDMotion("anm_idle_moving_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdleMoving();
}

void CWeaponMagazined::PlayAnimIdleMovingCrouch()
{
    if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_crouch_empty"))
        PlayHUDMotion("anm_idle_moving_crouch_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdleMovingCrouch();
}

void CWeaponMagazined::PlayAnimReload()
{
    auto state = GetState();
    VERIFY(state == eReload);

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
            PlayHUDMotion("anm_reload", true, this, state);
    }
    else
    {
        if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_reload_empty"))
            PlayHUDMotion("anm_reload_empty", true, this, state);
        else
            PlayHUDMotion("anm_reload", true, this, state);
    }
}

void CWeaponMagazined::PlayAnimAim() 
{ 
	if (IsRotatingToZoom())
    {
        if (SprintType) // Сразу принудительно их отключаем, чтобы не было багов, когда выходишь из аима, а она только проигрывается
        {
            SwitchState(eSprintEnd);
            return;
        }

        if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_start_empty"))
        {
            PlayHUDMotionNew("anm_idle_aim_start_empty", true, GetState());
            return;
        }
        else if (IsMisfire() && isHUDAnimationExist("anm_idle_aim_start_jammed"))
        {
            PlayHUDMotionNew("anm_idle_aim_start_jammed", true, GetState());
            return;
        }
        else if (isHUDAnimationExist("anm_idle_aim_start"))
        {
            PlayHUDMotionNew("anm_idle_aim_start", true, GetState());
            return;
        }
    }

	if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_empty"))
        PlayHUDMotion("anm_idle_aim_empty", true, nullptr, GetState());
    else if (IsMisfire() && isHUDAnimationExist("anm_idle_aim_jammed"))
        PlayHUDMotion("anm_idle_aim_jammed", true, nullptr, GetState());
    else
        PlayHUDMotion("anm_idle_aim", true, nullptr, GetState());

}

void CWeaponMagazined::PlayAnimIdle()
{
    if (GetState() != eIdle) return;
    if (TryPlayAnimIdle()) return;

	if (IsZoomed())
        PlayAnimAim();
    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_empty") && !TryPlayAnimIdle())
        PlayHUDMotion("anm_idle_empty", true, nullptr, GetState());
    else if (IsMisfire() && isHUDAnimationExist("anm_idle_jammed") && !TryPlayAnimIdle())
        PlayHUDMotion("anm_idle_jammed", true, nullptr, GetState());
    else
    {
        if (IsRotatingFromZoom())
        {
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_end_empty"))
            {
                PlayHUDMotionNew("anm_idle_aim_end_empty", true, GetState());
                return;
            }
            else if (IsMisfire() && isHUDAnimationExist("anm_idle_aim_end_jammed"))
            {
                PlayHUDMotionNew("anm_idle_aim_end_jammed", true, GetState());
                return;
            }
            else if (isHUDAnimationExist("anm_idle_aim_end"))
            {
                PlayHUDMotionNew("anm_idle_aim_end", true, GetState());
                return;
            }
        }
        inherited::PlayAnimIdle();
    }
}

void CWeaponMagazined::PlayAnimShoot()
{
    VERIFY(GetState() == eFire);

    // Если IsDiffShotModes = false или true, но в одиночном режиме.
	if (!IsDiffShotModes() || IsDiffShotModes() && GetCurrentFireMode() != -1)
    {
        // В зуме
        if (IsZoomed())
            if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_when_aim_l"))
                PlayHUDMotion("anm_shot_when_aim_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_aim_l"))
                PlayHUDMotion("anm_shots_aim_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_l"))
                PlayHUDMotion("anm_shot_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_l"))
                PlayHUDMotion("anm_shots_l", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shots_when_aim"))
                PlayHUDMotion("anm_shots_when_aim", false, nullptr, GetState());
            else if (IsZoomed() && isHUDAnimationExist("anm_shots_aim"))
                PlayHUDMotion("anm_shots_aim", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shoot"))
                PlayHUDMotion("anm_shoot", false, nullptr, GetState());
            else
                PlayHUDMotion("anm_shots", false, nullptr, GetState());
        // От бедра
        else if (!IsZoomed())
            if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_l"))
                PlayHUDMotion("anm_shot_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_l"))
                PlayHUDMotion("anm_shots_l", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shoot"))
                PlayHUDMotion("anm_shoot", false, nullptr, GetState());
            else
                PlayHUDMotion("anm_shots", false, nullptr, GetState());
    }
    // Если IsDiffShotModes и авторежим стрельбы (для SPAS-12).
    if (IsDiffShotModes() && GetCurrentFireMode() == -1)
    {
        // В зуме
        if (IsZoomed())
            if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_auto_when_aim_l"))
                PlayHUDMotion("anm_shot_auto_when_aim_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_auto_aim_l"))
                PlayHUDMotion("anm_shots_auto_aim_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_auto_l"))
                PlayHUDMotion("anm_shot_auto_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_auto_l"))
                PlayHUDMotion("anm_shots_auto_l", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shots_auto_when_aim"))
                PlayHUDMotion("anm_shots_auto_when_aim", false, nullptr, GetState());
            else if (IsZoomed() && isHUDAnimationExist("anm_shots_auto_aim"))
                PlayHUDMotion("anm_shots_auto_aim", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shoot"))
                PlayHUDMotion("anm_shoot", false, nullptr, GetState());
            else
                PlayHUDMotion("anm_shots", false, nullptr, GetState());
        // От бедра
        else if (!IsZoomed())
            if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shot_auto_l"))
                PlayHUDMotion("anm_shot_auto_l", false, nullptr, GetState());
            else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_auto_l"))
                PlayHUDMotion("anm_shots_auto_l", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shoot_auto"))
                PlayHUDMotion("anm_shoot_auto", false, nullptr, GetState());
            else if (isHUDAnimationExist("anm_shots_auto"))
                PlayHUDMotion("anm_shots_auto", false, nullptr, GetState());
            else
                PlayHUDMotion("anm_shots", false, nullptr, GetState());
    }
}

void CWeaponMagazined::OnZoomIn()
{
    inherited::OnZoomIn();

    if (GetState() == eIdle)
        PlayAnimIdle();

    //Alundaio: callback not sure why vs2013 gives error, it's fine
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject	*object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomIn)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (pActor)
    {
        CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
        if (!S)
        {
            S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(new CEffectorZoomInertion());
            S->Init(this);
        };
        S->SetRndSeed(pActor->GetZoomRndSeed());
        R_ASSERT(S);
    }
}
void CWeaponMagazined::OnZoomOut()
{
    if (!IsZoomed())
        return;

    inherited::OnZoomOut();

    if (GetState() == eIdle)
        PlayAnimIdle();

    //Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject *object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomOut)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    CActor* pActor = smart_cast<CActor*>(H_Parent());

    if (pActor)
        pActor->Cameras().RemoveCamEffector(eCEZoom);
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
    if (eIdle != GetState() || IsPending()) return false;

    if (SingleShotMode())
        m_iQueueSize = WEAPON_ININITE_QUEUE;
    else
        m_iQueueSize = 1;

    PlaySound("sndEmptyClick", get_LastFP());

    return true;
}

void CWeaponMagazined::OnNextFireMode()
{
    if (!m_bHasDifferentFireModes) 
        return;

    SwitchState(eFiremodeNext);

    if (GetState() != eIdle) 
        return;

    m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

void CWeaponMagazined::OnPrevFireMode()
{
    if (!m_bHasDifferentFireModes) 
        return;

    SwitchState(eFiremodePrev);

    if (GetState() != eIdle) 
        return;

    m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

void CWeaponMagazined::OnWeaponAddonLaserSwitch()
{
    SwitchState(eSwitchAddon);

    if (GetState() != eIdle)
        return;

    if (IsLaserAttached() && !m_bLaserShaderOn)
    {
        g_pGamePersistent->laser_shader_data.laser_factor = 1.f;
        m_bLaserShaderOn = true;
    }
    else if (IsLaserAttached() && m_bLaserShaderOn)
    {
        g_pGamePersistent->laser_shader_data.laser_factor = 0.f;
        m_bLaserShaderOn = false;
    }
};

void CWeaponMagazined::OnWeaponAddonFlashlightSwitch()
{
    SwitchState(eSwitchAddon);

    if (GetState() != eIdle)
        return;

    if (HasFlashlight())
        SwitchFlashlight(!IsFlashlightOn());
};

void CWeaponMagazined::OnWeaponAmmoCheck()
{
    SwitchState(eAmmoCheck);

    if (GetState() != eIdle)
        return;

    if (isHUDAnimationExist("anm_ammo_check"))
    {
        if (!IsMisfire() && m_ammoElapsed.type1 > 0)
            PlaySound("sndAmmoCheck", get_LastFP());

        if (isHUDAnimationExist("anm_ammo_check_empty") && m_ammoElapsed.type1 == 0)
        {
           if (m_sounds.FindSoundItem("sndAmmoCheckEmpty", false))
                PlaySound("sndAmmoCheckEmpty", get_LastFP());
           else
               PlaySound("sndAmmoCheck", get_LastFP());
        }

        if (isHUDAnimationExist("anm_ammo_check_jammed") && IsMisfire())
        {
            if (m_sounds.FindSoundItem("sndAmmoCheckJammed", false))
                PlaySound("sndAmmoCheckJammed", get_LastFP());
            else
                PlaySound("sndAmmoCheck", get_LastFP());
        }
    }
};

void CWeaponMagazined::OnH_A_Chield()
{
    if (m_bHasDifferentFireModes)
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());
        if (!actor) 
            SetQueueSize(-1);
        else 
            SetQueueSize(GetCurrentFireMode());
    };
    inherited::OnH_A_Chield();
};

void CWeaponMagazined::SetQueueSize(int size) 
{ 
    m_iQueueSize = size; 
};

float CWeaponMagazined::GetWeaponDeterioration()
{
    // modified by Peacemaker [17.10.08]
    //	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
    //		return inherited::GetWeaponDeterioration();
    //	return m_iShotNum*conditionDecreasePerShot;
    return (m_iShotNum == 1) ? conditionDecreasePerShot : conditionDecreasePerShot * 1.5;
};

void CWeaponMagazined::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_iQueueSize, output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_iCurFireMode, output_packet);
}

void CWeaponMagazined::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_iQueueSize, input_packet); 
    SetQueueSize(m_iQueueSize);
    load_data(m_iShotNum, input_packet);
    load_data(m_iCurFireMode, input_packet);
}

void CWeaponMagazined::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    P.w_u8(u8(m_iCurFireMode & 0x00ff));
}

void CWeaponMagazined::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    m_iCurFireMode = P.r_u8();
    SetQueueSize(GetCurrentFireMode());
}

#include "string_table.h"
bool CWeaponMagazined::GetBriefInfo(II_BriefInfo& info)
{
    VERIFY(m_pInventory);
    string32 int_str;

    int ae = GetAmmoElapsed();
    xr_sprintf(int_str, "%d", ae);
    info.cur_ammo = int_str;

	if (bHasBulletsToHide)
    {
        last_hide_bullet = ae >= bullet_cnt ? bullet_cnt : bullet_cnt - ae - 1;

        if (ae == 0)
            last_hide_bullet = -1;
    }

    if (HasFireModes())
    {
        if (m_iQueueSize == WEAPON_ININITE_QUEUE)
        {
            info.fire_mode = "A";
        }
        else
        {
            xr_sprintf(int_str, "%d", m_iQueueSize);
            info.fire_mode = int_str;
        }
    }
    else
        info.fire_mode = "";

    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
    {
        return false;
    }
    GetSuitableAmmoTotal(); // update m_BriefInfo_CalcFrame
    info.grenade = "";

    const u32 at_size = m_ammoTypes.size();
    if (unlimited_ammo() || at_size == 0)
    {
        info.fmj_ammo._set("--");
        info.ap_ammo._set("--");
        info.third_ammo._set("--"); //Alundaio
    }
    else
    {
        // GetSuitableAmmoTotal(); //mp = all type
        //Alundaio: Added third ammo type and cleanup
        info.fmj_ammo._set("");
        info.ap_ammo._set("");
        info.third_ammo._set("");

        if (at_size >= 1)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(0));
            info.fmj_ammo._set(int_str);
        }
        if (at_size >= 2)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(1));
            info.ap_ammo._set(int_str);
        }
        if (at_size >= 3)
        {
            xr_sprintf(int_str, "%d", GetAmmoCount(2));
            info.third_ammo._set(int_str);
        }
        //-Alundaio
    }

    if (ae != 0 && m_magazine.size() != 0)
    {
        LPCSTR ammo_type = m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();
        info.name = StringTable().translate(pSettings->r_string(ammo_type, "inv_name"));
        info.icon = ammo_type;
    }
    else
    {
        LPCSTR ammo_type = m_ammoTypes[m_ammoType.type1].c_str();
        info.name = StringTable().translate(pSettings->r_string(ammo_type, "inv_name"));
        info.icon = ammo_type;
    }
    return true;
}

bool CWeaponMagazined::install_upgrade_impl(LPCSTR section, bool test)
{
    bool result = inherited::install_upgrade_impl(section, test);

    LPCSTR str;
    // fire_modes = 1, 2, -1
    bool result2 = process_if_exists_set(section, "fire_modes", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        int ModesCount = _GetItemCount(str);
        m_aFireModes.clear();
        for (int i = 0; i < ModesCount; ++i)
        {
            string16 sItem;
            _GetItem(str, i, sItem);
            m_aFireModes.push_back((s8)atoi(sItem));
        }
        m_iCurFireMode = ModesCount - 1;
    }
    result |= result2;

    result |= process_if_exists_set(section, "base_dispersioned_bullets_count", &CInifile::r_s32, m_iBaseDispersionedBulletsCount, test);
    result |= process_if_exists_set(section, "base_dispersioned_bullets_speed", &CInifile::r_float, m_fBaseDispersionedBulletsSpeed, test);

    // sounds (name of the sound, volume (0.0 - 1.0), delay (sec))
    result2 = process_if_exists_set(section, "snd_draw", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_draw", "sndShow", false, m_eSoundShow);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_holster", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_holster", "sndHide", false, m_eSoundHide);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_shoot", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
#ifdef LAYERED_SND_SHOOT
        m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#else
        m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#endif
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_empty", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", false, m_eSoundEmptyClick);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_reload", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);
    }
    result |= result2;

	result2 = process_if_exists_set(section, "snd_reflect", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reflect", "sndReflect", false, m_eSoundReflect);
    }
    result |= result2;

	result2 = process_if_exists_set(section, "snd_shoot_distant", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_shoot_distant", "sndShotDist", false, m_eSoundShot);
        m_bHasDistantShotSound = true;
    }
    result |= result2;

	result2 = process_if_exists_set(section, "snd_shoot_distant_far", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_shoot_distant_far", "sndShotDistFar", false, m_eSoundShot);
        m_bHasDistantShotSound = true;
    }
    result |= result2;

	result2 = process_if_exists_set(section, "snd_draw_empty", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_draw_empty", "sndShowEmpty", false, m_eSoundShowEmpty);
	}
	result |= result2;

	result2 = process_if_exists_set(section, "snd_holster_empty", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_holster_empty", "sndHideEmpty", false, m_eSoundHideEmpty);
	}
	result |= result2;

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    result2 = process_if_exists_set(section, "snd_reload_empty", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    }
    result |= result2;
#endif //-NEW_SOUNDS

    // snd_shoot1     = weapons\ak74u_shot_1 ??
    // snd_shoot2     = weapons\ak74u_shot_2 ??
    // snd_shoot3     = weapons\ak74u_shot_3 ??

    if (m_eSilencerStatus == ALife::eAddonAttachable || m_eSilencerStatus == ALife::eAddonPermanent)
    {
		result |= process_if_exists_set(section, "silencer_flame_particles", &CInifile::r_string, m_sSilencerFlameParticles, test);
		result |= process_if_exists_set(section, "silencer_smoke_particles", &CInifile::r_string, m_sSilencerSmokeParticles, test);

		result2 = process_if_exists_set(section, "snd_silncer_shot", &CInifile::r_string, str, test);
        if (result2 && !test)
        {
#ifdef LAYERED_SND_SHOOT
            m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#else
            m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#endif
        }
        result |= result2;
    }

    result |= process_if_exists(section, "scope_zoom_factor", &CInifile::r_float, m_zoom_params.m_fBaseZoomFactor, test);

    // Альт. прицеливание
	UpdateUIScope();

    return result;
}
//текущая дисперсия (в радианах) оружия с учетом используемого патрона и недисперсионных пуль
float CWeaponMagazined::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
    float fire_disp = GetBaseDispersion(cartridge_k);
    if (for_crosshair || !m_iBaseDispersionedBulletsCount || !m_iShotNum || m_iShotNum > m_iBaseDispersionedBulletsCount)
    {
        fire_disp = inherited::GetFireDispersion(cartridge_k);
    }
    return fire_disp;
}
void CWeaponMagazined::FireBullet(const Fvector& pos, const Fvector& shot_dir, float fire_disp,
    const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit)
{
    if (m_iBaseDispersionedBulletsCount)
    {
        if (m_iShotNum <= 1)
        {
            m_fOldBulletSpeed = GetBulletSpeed();
            SetBulletSpeed(m_fBaseDispersionedBulletsSpeed);
        }
        else if (m_iShotNum > m_iBaseDispersionedBulletsCount)
        {
            SetBulletSpeed(m_fOldBulletSpeed);
        }
    }
    inherited::FireBullet(pos, shot_dir, fire_disp, cartridge, parent_id, weapon_id, send_hit, GetAmmoElapsed());
}

void CWeaponMagazined::CheckMagazine() // Остаётся ли патрон в патроннике?
{
    if (!ParentIsActor())
    {
        m_bNeedBulletInGun = false;
        return;
    }

    if (m_bCartridgeInTheChamber == true && isHUDAnimationExist("anm_reload_empty") && m_ammoElapsed.type1 >= 1 && m_bNeedBulletInGun == false)
    {
        m_bNeedBulletInGun = true;
    }
    else if (isHUDAnimationExist("anm_reload_empty") && m_ammoElapsed.type1 == 0 && m_bNeedBulletInGun == true)
    {
        m_bNeedBulletInGun = false;
    }
}

void CWeaponMagazined::OnMotionMark(u32 state, const motion_marks& M)
{
    inherited::OnMotionMark(state, M);
    if (state == eReload)
    {
        u8 ammo_type = m_ammoType.type1;
        int ae = CheckAmmoBeforeReload(ammo_type);

        if (ammo_type == m_ammoType.type1)
        {
            //Msg("Ammo elapsed: %d", m_ammoElapsed.type1);
            ae += m_ammoElapsed.type1;
        }

        last_hide_bullet = ae >= bullet_cnt ? bullet_cnt : bullet_cnt - ae - 1;

        //Msg("Next reload: count %d with type %d", ae, ammo_type);

        HUD_VisualBulletUpdate();
    }
    //-- Дропаем гильзу, если есть MotionMark в анимации расклина во время расклина.
    if (IsMisfireOneCartRemove() && GetState() == eUnMisfire)
    {
        Fvector vel;
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }
    //-- Дропаем гильзу во время стрельбы (для дробовиков и болтовок).
    if (IsMotionMarkShell() && GetState() == eFire)
    {
        Fvector vel;
        PHGetLinearVell(vel);
        OnShellDrop(get_LastSP(), vel);
    }
}
