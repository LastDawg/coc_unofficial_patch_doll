#include "stdafx.h"
#include "WeaponMagazinedWGrenade.h"
#include "Entity.h"
#include "ParticlesObject.h"
#include "GrenadeLauncher.h"
#include "xrServer_Objects_ALife_Items.h"
#include "ExplosiveRocket.h"
#include "Actor.h"
#include "xr_level_controller.h"
#include "Level.h"
#include "Common/object_broker.h"
#include "game_base_space.h"
#include "xrPhysics/MathUtils.h"
#include "player_hud.h"

#ifdef DEBUG
#include "PHDebug.h"
#endif

CWeaponMagazinedWGrenade::CWeaponMagazinedWGrenade(ESoundTypes eSoundType) : CWeaponMagazined(eSoundType) {}

CWeaponMagazinedWGrenade::~CWeaponMagazinedWGrenade() {}
void CWeaponMagazinedWGrenade::Load(LPCSTR section)
{
    inherited::Load(section);
    CRocketLauncher::Load(section);

    //// Sounds
    m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", false, m_eSoundShot);
    m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
    m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);

    m_sFlameParticles2 = pSettings->r_string(section, "grenade_flame_particles");

	m_ammoTypes2.clear();
    if (m_eGrenadeLauncherStatus != ALife::eAddonAttachable)
    {
        LPCSTR S = pSettings->r_string(section, "grenade_class");
        if (S && S[0])
        {
            string128 _ammoItem;
            int count = _GetItemCount(S);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _ammoItem);
                m_ammoTypes2.push_back(_ammoItem);
            }
        }
    }

	if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
    {
        CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(section, "grenade_vel");
    }
}

void CWeaponMagazinedWGrenade::net_Destroy() { inherited::net_Destroy(); }

BOOL CWeaponMagazinedWGrenade::net_Spawn(CSE_Abstract* DC)
{
    CSE_ALifeItemWeapon* const weapon = smart_cast<CSE_ALifeItemWeapon*>(DC);
    R_ASSERT(weapon);
    if (IsGameTypeSingle()){}

    BOOL l_res = inherited::net_Spawn(DC);

	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
    {
        m_ammoTypes2.clear();
        LPCSTR S = pSettings->r_string(GetGrenadeLauncherName(), "grenade_class");
        if (S && S[0])
        {
            string128 _ammoItem;
            int count = _GetItemCount(S);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _ammoItem);
                m_ammoTypes2.push_back(_ammoItem);
            }
        }
    }

	if (m_ammoType.type2 >= m_ammoTypes2.size())
        m_ammoType.type2 = 0;

	m_DefaultCartridge2.Load(m_ammoTypes2[m_ammoType.type2].c_str(), m_ammoType.type2);
    if (m_ammoElapsed.type2)
    {
        for (int i = 0; i < m_ammoElapsed.type2; ++i)
            m_magazine2.push_back(m_DefaultCartridge2);

		if (!getRocketCount())
        {
            shared_str fake_grenade_name = pSettings->r_string(m_magazine2.back().m_ammoSect, "fake_grenade_name");
            CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
        }
    }
    UpdateGrenadeVisibility(m_bGrenadeMode && !!m_ammoElapsed.type2 ? true : false);
    SetPending(FALSE);

    if (m_bGrenadeMode && IsGrenadeLauncherAttached())
    {
		std::swap(iMagazineSize, iMagazineSize2);
		u8 old = m_ammoType.type1;
		m_ammoType.type1 = m_ammoType.type2;
		m_ammoType.type2 = old;
		m_ammoTypes.swap(m_ammoTypes2);
		std::swap(m_DefaultCartridge, m_DefaultCartridge2);
		m_magazine.swap(m_magazine2);
		m_ammoElapsed.type1 = (u16)m_magazine.size();
		m_ammoElapsed.type2 = (u16)m_magazine2.size();
    }
	else //In the case of config change or upgrade that removes GL
    {
	    m_bGrenadeMode = false;
    }
    return l_res;
}

void CWeaponMagazinedWGrenade::switch2_Reload()
{
    VERIFY(GetState() == eReload);
    if (m_bGrenadeMode)
    {
        PlaySound("sndReloadG", get_LastFP2());

        if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_reload_empty_g"))
            PlayHUDMotion("anm_reload_empty_g", FALSE, this, GetState());
        else if (IsMisfire() && isHUDAnimationExist("anm_reload_jammed_g"))
            PlayHUDMotion("anm_reload_jammed_g", FALSE, this, GetState());
        else
            PlayHUDMotion("anm_reload_g", FALSE, this, GetState());

        SetPending(TRUE);
    }
    else
        inherited::switch2_Reload();
}

void CWeaponMagazinedWGrenade::OnShot()
{
    if (m_bGrenadeMode)
    {
        PlayAnimShoot();
        PlaySound("sndShotG", get_LastFP2());
        AddShotEffector();
        StartFlameParticles2();
    }
    else
        inherited::OnShot();
}

void CWeaponMagazinedWGrenade::OnMotionMark(u32 state, const motion_marks& M)
{
	inherited::OnMotionMark(state, M);
}

bool CWeaponMagazinedWGrenade::SwitchMode()
{
    bool bUsefulStateToSwitch = ((eIdle == GetState()) || (eHidden == GetState()) || (eMisfire == GetState()) || (eMagEmpty == GetState())) && (!IsPending());

    if (!bUsefulStateToSwitch)
        return false;

    if (!IsGrenadeLauncherAttached())
        return false;

    OnZoomOut();

    SetPending(TRUE);

    PerformSwitchGL();

    PlaySound("sndSwitch", get_LastFP());

    PlayAnimModeSwitch();

    m_BriefInfo_CalcFrame = 0;

    return true;
}

void CWeaponMagazinedWGrenade::PerformSwitchGL()
{
    m_bGrenadeMode = !m_bGrenadeMode;

	// Альт. прицеливание
    m_zoomtype = m_bGrenadeMode ? 2 : 0;
    UpdateUIScope();

    std::swap(iMagazineSize, iMagazineSize2);

    m_ammoTypes.swap(m_ammoTypes2);

	u8 old = m_ammoType.type1;
    m_ammoType.type1 = m_ammoType.type2;
    m_ammoType.type2 = old;

    std::swap(m_DefaultCartridge, m_DefaultCartridge2);

    m_magazine.swap(m_magazine2);
    m_ammoElapsed.type1 = (u16)m_magazine.size();
    m_ammoElapsed.type2 = (u16)m_magazine2.size();

    m_BriefInfo_CalcFrame = 0;
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags)
{
    if (m_bGrenadeMode && cmd == kWPN_FIRE)
    {
        if (IsPending())
            return false;

        if (flags & CMD_START)
        {
            if (m_ammoElapsed.type1)
                LaunchGrenade();
            else
                Reload();

            if (GetState() == eIdle)
                OnEmptyClick();
        }
        return true;
    }
    if (inherited::Action(cmd, flags))
        return true;

    switch (cmd)
    {
    case kWPN_FUNC: 
    {
        if (flags & CMD_START && !IsPending())
            SwitchState(eSwitchGL);
        return true;
    }
    }

    return false;
}

#include "inventory.h"
#include "inventoryOwner.h"
void CWeaponMagazinedWGrenade::state_Fire(float dt)
{
    VERIFY(fOneShotTime > 0.f);

    //режим стрельбы подствольника
    if (m_bGrenadeMode)
    {
        /*
        fTime					-=dt;
        while (fTime<=0 && (iAmmoElapsed>0) && (IsWorking() || m_bFireSingleShot))
        {
            ++m_iShotNum;
            OnShot			();

            // Ammo
            if(Local())
            {
                VERIFY				(m_magazine.size());
                m_magazine.pop_back	();
                --iAmmoElapsed;

                VERIFY((u32)iAmmoElapsed == m_magazine.size());
            }
        }
        UpdateSounds				();
        if(m_iShotNum == m_iQueueSize)
            FireEnd();
        */
    }
    //режим стрельбы очередями
    else
        inherited::state_Fire(dt);
}

void CWeaponMagazinedWGrenade::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    u16 id;
    switch (type)
    {
    case GE_OWNERSHIP_TAKE:
    {
        P.r_u16(id);
        CRocketLauncher::AttachRocket(id, this);
    }
    break;
    case GE_OWNERSHIP_REJECT:
    case GE_LAUNCH_ROCKET:
    {
        bool bLaunch = (type == GE_LAUNCH_ROCKET);
        P.r_u16(id);
        CRocketLauncher::DetachRocket(id, bLaunch);
        if (bLaunch)
        {
            PlayAnimShoot();
            PlaySound("sndShotG", get_LastFP2());
            AddShotEffector();
            StartFlameParticles2();
        }
        break;
    }
    }
}

void CWeaponMagazinedWGrenade::LaunchGrenade()
{
    if (!getRocketCount())
        return;
    R_ASSERT(m_bGrenadeMode);
    {
        Fvector p1, d;
        p1.set(get_LastFP2());
        d.set(get_LastFD());
        CEntity* E = smart_cast<CEntity*>(H_Parent());

        if (E)
        {
            CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
            if (NULL == io->inventory().ActiveItem())
            {
                Log("current_state", GetState());
                Log("next_state", GetNextState());
                Log("item_sect", cNameSect().c_str());
                Log("H_Parent", H_Parent()->cNameSect().c_str());
            }
            E->g_fireParams(this, p1, d);
        }
        if (IsGameTypeSingle())
            p1.set(get_LastFP2());

        Fmatrix launch_matrix;
        launch_matrix.identity();
        launch_matrix.k.set(d);
        Fvector::generate_orthonormal_basis(launch_matrix.k, launch_matrix.j, launch_matrix.i);

        launch_matrix.c.set(p1);

        if (IsGameTypeSingle() && IsZoomed() && smart_cast<CActor*>(H_Parent()))
        {
            H_Parent()->setEnabled(FALSE);
            setEnabled(FALSE);

            collide::rq_result RQ;
            BOOL HasPick = Level().ObjectSpace.RayPick(p1, d, 300.0f, collide::rqtStatic, RQ, this);

            setEnabled(TRUE);
            H_Parent()->setEnabled(TRUE);

            if (HasPick)
            {
                Fvector Transference;
                Transference.mul(d, RQ.range);
                Fvector res[2];
#ifdef DEBUG
//.				DBG_OpenCashedDraw();
//.				DBG_DrawLine(p1,Fvector().add(p1,d),color_xrgb(255,0,0));
#endif
                u8 canfire0 = TransferenceAndThrowVelToThrowDir(
                    Transference, CRocketLauncher::m_fLaunchSpeed, EffectiveGravity(), res);
#ifdef DEBUG
//.				if(canfire0>0)DBG_DrawLine(p1,Fvector().add(p1,res[0]),color_xrgb(0,255,0));
//.				if(canfire0>1)DBG_DrawLine(p1,Fvector().add(p1,res[1]),color_xrgb(0,0,255));
//.				DBG_ClosedCashedDraw(30000);
#endif

                if (canfire0 != 0)
                {
                    d = res[0];
                };
            }
        };

        d.normalize();
        d.mul(CRocketLauncher::m_fLaunchSpeed);
        VERIFY2(_valid(launch_matrix), "CWeaponMagazinedWGrenade::SwitchState. Invalid launch_matrix!");
        CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

        CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
        VERIFY(pGrenade);
        pGrenade->SetInitiator(H_Parent()->ID());

        if (Local() && OnServer())
        {
            VERIFY(m_magazine.size());
            m_magazine.pop_back();
            --m_ammoElapsed.type1;
            VERIFY((u32)m_ammoElapsed.type1 == m_magazine.size());

            NET_Packet P;
            u_EventGen(P, GE_LAUNCH_ROCKET, ID());
            P.w_u16(getCurrentRocket()->ID());
            u_EventSend(P);
        };
    }
}

void CWeaponMagazinedWGrenade::FireEnd()
{
    if (m_bGrenadeMode)
    {
        CWeapon::FireEnd();
    }
    else
        inherited::FireEnd();
}

void CWeaponMagazinedWGrenade::OnMagazineEmpty()
{
    if (GetState() == eIdle)
    {
        OnEmptyClick();
    }
}

void CWeaponMagazinedWGrenade::ReloadMagazine()
{
    inherited::ReloadMagazine();

    //перезарядка подствольного гранатомета
    if (m_ammoElapsed.type1 && !getRocketCount() && m_bGrenadeMode)
    {
        shared_str fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType.type1].c_str(), "fake_grenade_name");

        CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
    }
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S, u32 oldState)
{
    switch (S)
    {
    case eSwitchGL:
    {
        if (!SwitchMode())
        {
            SwitchState(eIdle);
            return;
        }
    }
    break;
    }

    inherited::OnStateSwitch(S, oldState);
    UpdateGrenadeVisibility(!!m_ammoElapsed.type1 || S == eReload);
}

void CWeaponMagazinedWGrenade::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eSwitchGL: 
    { 
        SwitchState(eIdle);
    }
    break;
    case eFire:
    {
        if (m_bGrenadeMode)
            Reload();
    }
    break;
    }
    inherited::OnAnimationEnd(state);
}

void CWeaponMagazinedWGrenade::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    SetPending(FALSE);
    if (m_bGrenadeMode)
    {
        SetState(eIdle);
        SetPending(FALSE);
    }
}

bool CWeaponMagazinedWGrenade::CanAttach(PIItem pIItem) { return inherited::CanAttach(pIItem); }

bool CWeaponMagazinedWGrenade::CanDetach(pcstr item_section_name) 
{ return inherited::CanDetach(item_section_name); }

bool CWeaponMagazinedWGrenade::Attach(PIItem pIItem, bool b_send_event)
{
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

	if (pGrenadeLauncher &&
		m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 /*&&
																					(m_sGrenadeLauncherName == pIItem->object().cNameSect())*/)
    {
        auto it = m_launchers.begin();
        for (; it != m_launchers.end(); it++)
        {
            if (pSettings->r_string((*it), "grenade_launcher_name") == pIItem->object().cNameSect())
            {
                m_cur_addon.launcher = u16(it - m_launchers.begin());
                m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
                CRocketLauncher::m_fLaunchSpeed = pGrenadeLauncher->GetGrenadeVel();
                m_ammoTypes2.clear();
                LPCSTR S = pSettings->r_string(pIItem->object().cNameSect(), "grenade_class");
                if (S && S[0])
                {
                    string128 _ammoItem;
                    int count = _GetItemCount(S);
                    for (int it = 0; it < count; ++it)
                    {
                        _GetItem(S, it, _ammoItem);
                        m_ammoTypes2.push_back(_ammoItem);
                    }
                }
                //óíè÷òîæèòü ïîäñòâîëüíèê èç èíâåíòàðÿ
                if (b_send_event)
                {
                    if (OnServer())
                        pIItem->object().DestroyObject();
                }
                InitAddons();
                UpdateAddonsVisibility();
                if (GetState() == eIdle)
                    PlayAnimIdle();

                SyncronizeWeaponToServer();

                return true;
            }
        }
    }

    return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazinedWGrenade::Detach(pcstr item_section_name, bool b_spawn_item)
{
    if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && DetachGrenadeLauncher(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
        {
            Msg("ERROR: grenade launcher addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		// Now we need to unload GL's magazine
        if (!m_bGrenadeMode)
        {
            PerformSwitchGL();
        }
		UnloadMagazine();
		PerformSwitchGL();        

        UpdateAddonsVisibility();

        if (GetState() == eIdle)
            PlayAnimIdle();

		SyncronizeWeaponToServer();

        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item);
}

void CWeaponMagazinedWGrenade::InitAddons()
{
    inherited::InitAddons();

	if (IsGrenadeLauncherAttached() && GrenadeLauncherAttachable())
    {
        CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(GetGrenadeLauncherName(), "grenade_vel");
        m_ammoTypes2.clear();
        LPCSTR S = pSettings->r_string(GetGrenadeLauncherName(), "grenade_class");
        if (S && S[0])
        {
            string128 _ammoItem;
            int count = _GetItemCount(S);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _ammoItem);
                m_ammoTypes2.push_back(_ammoItem);
            }
        }
    }
}

bool CWeaponMagazinedWGrenade::UseScopeTexture()
{
    if (IsGrenadeLauncherAttached() && m_bGrenadeMode)
        return false;

    return true;
};

float CWeaponMagazinedWGrenade::CurrentZoomFactor()
{
    if (IsGrenadeLauncherAttached() && m_bGrenadeMode)
        return m_zoom_params.m_fScopeZoomFactor;
    return inherited::CurrentZoomFactor();
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazinedWGrenade::PlayAnimShow()
{
    VERIFY(GetState() == eShowing);
    if (IsGrenadeLauncherAttached())
    {
        if (!m_bGrenadeMode)
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_show_empty_w_gl"))
                PlayHUDMotion("anm_show_empty_w_gl", FALSE, this, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_show_jammed_w_gl"))
                PlayHUDMotion("anm_show_jammed_w_gl", FALSE, this, GetState());
            else
                PlayHUDMotion("anm_show_w_gl", FALSE, this, GetState());
        else
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_show_g_empty"))
                PlayHUDMotion("anm_show_g_empty", TRUE, this, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_show_g_jammed"))
                PlayHUDMotion("anm_show_g_jammed", true, this, GetState());
            else
                PlayHUDMotion("anm_show_g", TRUE, this, GetState());
    }
    else
        inherited::PlayAnimShow();
}

void CWeaponMagazinedWGrenade::PlayAnimHide()
{
    VERIFY(GetState() == eHiding);

    if (IsGrenadeLauncherAttached())
    {
        if (!m_bGrenadeMode)
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_hide_empty_w_gl"))
                PlayHUDMotion("anm_hide_empty_w_gl", TRUE, this, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_hide_jammed_w_gl"))
                PlayHUDMotion("anm_hide_jammed_w_gl", true, this, GetState());
            else
                PlayHUDMotion("anm_hide_w_gl", TRUE, this, GetState());
        else
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_hide_g_empty"))
                PlayHUDMotion("anm_hide_g_empty", TRUE, this, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_hide_g_jammed"))
                PlayHUDMotion("anm_hide_g_jammed", true, this, GetState());
            else
                PlayHUDMotion("anm_hide_g", TRUE, this, GetState());
    }
    else
        inherited::PlayAnimHide();
}

void CWeaponMagazinedWGrenade::PlayAnimReload()
{
    auto state = GetState();
    VERIFY(state == eReload);

#ifdef NEW_ANIMS //AVO: use new animations
    if (IsGrenadeLauncherAttached())
    {
        if (bMisfire)
        {
            if (isHUDAnimationExist("anm_reload_misfire_w_gl"))
                PlayHUDMotion("anm_reload_misfire_w_gl", true, this, state);
            else
                PlayHUDMotion("anm_reload_w_gl", true, this, state);
        }
        else
        {
            if (m_ammoElapsed.type1 == 0)
            {
                if (isHUDAnimationExist("anm_reload_empty_w_gl"))
                    PlayHUDMotion("anm_reload_empty_w_gl", true, this, state);
                else
                    PlayHUDMotion("anm_reload_w_gl", true, this, state);
            }
            else
                PlayHUDMotion("anm_reload_w_gl", true, this, state);
        }
    }
    else
        inherited::PlayAnimReload();
#else
    if (IsGrenadeLauncherAttached())
        PlayHUDMotion("anm_reload_w_gl", true, this, state);
    else
        inherited::PlayAnimReload();
#endif //-NEW_ANIMS
}

void CWeaponMagazinedWGrenade::PlayAnimIdle()
{
    // Вход и выход из прицеливания
    if (IsGrenadeLauncherAttached())
    {
        if (IsZoomed())
        {
            if (IsRotatingToZoom() && m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_start_empty_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_start_empty_w_gl", true, GetState());
                return;
            }
            else if (IsRotatingToZoom() && IsMisfire() && isHUDAnimationExist("anm_idle_aim_start_jammed_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_start_jammed_w_gl", true, GetState());
                return;
            }
            else if (IsRotatingToZoom() && isHUDAnimationExist("anm_idle_aim_start_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_start_w_gl", true, GetState());
                return;
            }
            // Анимации в прицеливании
            if (m_bGrenadeMode)
            {
                if (IsMisfire() && isHUDAnimationExist("anm_idle_aim_jammed_g"))
                    PlayHUDMotion("anm_idle_aim_jammed_g", true, nullptr, GetState());
                else if(m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_empty_g"))
                    PlayHUDMotion("anm_idle_aim_empty_g", true, nullptr, GetState());
                else if (isHUDAnimationExist("anm_idle_aim_g"))
                    PlayHUDMotion("anm_idle_aim_g", true, nullptr, GetState());
                else if (isHUDAnimationExist("anm_idle_g_aim"))
                    PlayHUDMotion("anm_idle_g_aim", true, nullptr, GetState());
                else
                    PlayHUDMotion("anm_idle_w_gl", true, nullptr, GetState());
            }
            else if (!m_bGrenadeMode)
            {
                if (IsMisfire() && isHUDAnimationExist("anm_idle_aim_jammed_w_gl"))
                    PlayHUDMotion("anm_idle_aim_jammed_w_gl", true, nullptr, GetState());
                else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_empty_w_gl"))
                    PlayHUDMotion("anm_idle_aim_empty_w_gl", true, nullptr, GetState());
                else if(isHUDAnimationExist("anm_idle_aim_w_gl"))
                    PlayHUDMotion("anm_idle_aim_w_gl", true, nullptr, GetState());
                else if (isHUDAnimationExist("anm_idle_w_gl_aim"))
                    PlayHUDMotion("anm_idle_w_gl_aim", true, nullptr, GetState());
                else
                    PlayHUDMotion("anm_idle_w_gl", true, nullptr, GetState());
            }
        }
        else
        {
            if (IsRotatingFromZoom() && m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_aim_end_empty_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_end_empty_w_gl", true, GetState());
                return;
            }
            else if (IsRotatingFromZoom() && IsMisfire() && isHUDAnimationExist("anm_idle_aim_end_jammed_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_end_jammed_w_gl", true, GetState());
                return;
            }
            else if (IsRotatingFromZoom() && isHUDAnimationExist("anm_idle_aim_end_w_gl"))
            {
                PlayHUDMotionNew("anm_idle_aim_end_w_gl", true, GetState());
                return;
            }
        }
    }

    if (IsGrenadeLauncherAttached())
    {
        // Состояния
        if (!IsZoomed())
        {
            int act_state = 0;
            CActor* pActor = smart_cast<CActor*>(H_Parent());
            if (pActor)
            {
                CEntity::SEntityState st;
                pActor->g_State(st);
                if (st.bSprint)
                {
					if (!SprintType)
                    {
                        SwitchState(eSprintStart);
                        return;
                    }
                    act_state = 1;
                }
                else if (pActor->AnyMove())
                {
                    if (!st.bCrouch)
                        act_state = 2;
                    if (st.bCrouch)
                        act_state = 3;
                }
                else 
                    if (SprintType)
                    {
                        SwitchState(eSprintEnd);
                        return;
                    }
            }
            // В режиме стрельбы ПГ
			if (m_bGrenadeMode)
            {
                switch (act_state)
                {
                case 0:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_jammed_g"))
                        PlayHUDMotion("anm_idle_jammed_g", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_empty_g"))
                        PlayHUDMotion("anm_idle_empty_g", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_g", true, nullptr, GetState());
                    break;
                case 1:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_sprint_jammed_g"))
                        PlayHUDMotion("anm_idle_sprint_jammed_g", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_sprint_empty_g"))
                        PlayHUDMotion("anm_idle_sprint_empty_g", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_sprint_g", true, nullptr, GetState());
                    break;
                case 2:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_moving_jammed_g"))
                        PlayHUDMotion("anm_idle_moving_jammed_g", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_empty_g"))
                        PlayHUDMotion("anm_idle_moving_empty_g", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_moving_g", true, nullptr, GetState());
                    break;
                case 3:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_moving_crouch_jammed_g"))
                        PlayHUDMotion("anm_idle_moving_crouch_jammed_g", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_crouch_empty_g"))
                        PlayHUDMotion("anm_idle_moving_crouch_empty_g", true, nullptr, GetState());
                    else if (isHUDAnimationExist("anm_idle_moving_crouch_g"))
                        PlayHUDMotion("anm_idle_moving_crouch_g", true, nullptr, GetState());
                    else 
                        PlayHUDMotion("anm_idle_moving_g", true, nullptr, GetState());
                    break;
                }
            }
            // В режиме стрельбы автомата
            else if (!m_bGrenadeMode)
            {
                switch (act_state)
                {
                case 0:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_jammed_w_gl"))
                        PlayHUDMotion("anm_idle_jammed_w_gl", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_empty_w_gl"))
                        PlayHUDMotion("anm_idle_empty_w_gl", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_w_gl", true, nullptr, GetState());
                    break;
                case 1:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_sprint_jammed_w_gl"))
                        PlayHUDMotion("anm_idle_sprint_jammed_w_gl", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_sprint_empty_w_gl"))
                        PlayHUDMotion("anm_idle_sprint_empty_w_gl", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_sprint_w_gl", true, nullptr, GetState());
                    break;
                case 2:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_moving_jammed_w_gl"))
                        PlayHUDMotion("anm_idle_moving_jammed_w_gl", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_empty_w_gl"))
                        PlayHUDMotion("anm_idle_moving_empty_w_gl", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_moving_w_gl", true, nullptr, GetState());
                    break;
                case 3:
                    if (IsMisfire() && isHUDAnimationExist("anm_idle_moving_crouch_jammed_w_gl"))
                        PlayHUDMotion("anm_idle_moving_crouch_jammed_w_gl", true, nullptr, GetState());
                    else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_idle_moving_crouch_empty_w_gl"))
                        PlayHUDMotion("anm_idle_moving_crouch_empty_w_gl", true, nullptr, GetState());
                    else
                        PlayHUDMotion("anm_idle_moving_crouch_w_gl", true, nullptr, GetState());
                    break;
                }
            }
        }
    }
    else
        inherited::PlayAnimIdle();
}

void CWeaponMagazinedWGrenade::PlayAnimShoot()
{
    // В режиме стрельбы ПГ
    if (m_bGrenadeMode)
    {
        // В зуме
        if (IsZoomed())
        {
            if (IsMisfire() && isHUDAnimationExist("anm_shots_aim_jammed_g"))
                PlayHUDMotion("anm_shots_aim_jammed_g", false, this, eFire);
            else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_shots_aim_empty_g"))
                PlayHUDMotion("anm_shots_aim_empty_g", false, this, eFire);
            else if(isHUDAnimationExist("anm_shots_aim_g"))
                PlayHUDMotion("anm_shots_aim_g", false, this, eFire);
            else if (isHUDAnimationExist("anm_shots_g_aim"))
                PlayHUDMotion("anm_shots_g_aim", false, this, eFire);
            else
                PlayHUDMotion("anm_shots_g", false, this, eFire);
        }
        // От бедра
        else if (!IsZoomed())
        {
            if (IsMisfire() && isHUDAnimationExist("anm_shots_jammed_g"))
                PlayHUDMotion("anm_shots_jammed_g", false, this, eFire);
            else if(m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_shots_empty_g"))
                PlayHUDMotion("anm_shots_empty_g", false, this, eFire);
            else 
                PlayHUDMotion("anm_shots_g", false, this, eFire);
        }
    }
    // В режиме стрельбы автомата
    else if (!m_bGrenadeMode)
    {
        VERIFY(GetState() == eFire);
        if (IsGrenadeLauncherAttached())
        {
            // В зуме
            if (IsZoomed())
                if (isHUDAnimationExist("anm_shots_when_aim_w_gl"))
                    PlayHUDMotion("anm_shots_when_aim_w_gl", false, this, GetState());
                else if (isHUDAnimationExist("anm_shots_w_gl_aim"))
                    PlayHUDMotion("anm_shots_w_gl_aim", false, this, GetState());
                else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_when_aim_l_w_gl"))
                    PlayHUDMotion("anm_shots_when_aim_l_w_gl", false, this, GetState());
                else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_when_aim_w_gl_l"))
                    PlayHUDMotion("anm_shots_when_aim_w_gl_l", false, this, GetState());
                else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_l_w_gl_aim"))
                    PlayHUDMotion("anm_shots_l_w_gl_aim", false, this, GetState());
                else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_w_gl_aim_l"))
                    PlayHUDMotion("anm_shots_w_gl_aim_l", false, this, GetState());
                else
                    PlayHUDMotion("anm_shots_w_gl", false, this, GetState());
            // От бедра
            else if (!IsZoomed())
                if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_l_w_gl"))
                    PlayHUDMotion("anm_shots_l_w_gl", false, this, GetState());
                else if (m_ammoElapsed.type1 == 1 && isHUDAnimationExist("anm_shots_w_gl_l"))
                    PlayHUDMotion("anm_shots_w_gl_l", false, this, GetState());
                else
                    PlayHUDMotion("anm_shots_w_gl", false, this, GetState());
        }
        else
            inherited::PlayAnimShoot();
    }
}

void CWeaponMagazinedWGrenade::PlayAnimModeSwitch()
{
    if (m_bGrenadeMode)
        if (IsMisfire() && isHUDAnimationExist("anm_switch_jammed_g"))
            PlayHUDMotion("anm_switch_jammed_g", true, this, eSwitchGL);
        else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_switch_empty_g"))
            PlayHUDMotion("anm_switch_empty_g", true, this, eSwitchGL);
        else
            PlayHUDMotion("anm_switch_g", true, this, eSwitchGL); 
    else if (!m_bGrenadeMode)
        if (IsMisfire() && isHUDAnimationExist("anm_switch_jammed"))
            PlayHUDMotion("anm_switch_jammed", true, this, eSwitchGL);
        else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_switch_empty"))
            PlayHUDMotion("anm_switch_empty", true, this, eSwitchGL);
        else
            PlayHUDMotion("anm_switch", true, this, eSwitchGL);

}

void CWeaponMagazinedWGrenade::PlayAnimBore()
{
    if (IsGrenadeLauncherAttached())
    {
        if (!m_bGrenadeMode)
        {
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_bore_empty_w_gl"))
                PlayHUDMotion("anm_bore_empty_w_gl", TRUE, this, GetState());
            else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_bore_w_gl_empty"))
                PlayHUDMotion("anm_bore_w_gl_empty", true, nullptr, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_bore_jammed_w_gl"))
                PlayHUDMotion("anm_bore_jammed_w_gl", true, nullptr, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_bore_w_gl_jammed"))
                PlayHUDMotion("anm_bore_w_gl_jammed", true, nullptr, GetState());
            else 
                PlayHUDMotion("anm_bore_w_gl", true, nullptr, GetState());
        }

        if (m_bGrenadeMode)
        {
            if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_bore_empty_g"))
                PlayHUDMotion("anm_bore_empty_g", TRUE, this, GetState());
            else if (m_ammoElapsed.type1 == 0 && isHUDAnimationExist("anm_bore_g_empty"))
                PlayHUDMotion("anm_bore_g_empty", TRUE, this, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_bore_jammed_g"))
                PlayHUDMotion("anm_bore_jammed_g", true, nullptr, GetState());
            else if (IsMisfire() && isHUDAnimationExist("anm_bore_g_jammed"))
                PlayHUDMotion("anm_bore_g_jammed", true, nullptr, GetState());
            else
                PlayHUDMotion("anm_bore_g", TRUE, this, GetState());
        }
    }
    else
        inherited::PlayAnimBore();
}

void CWeaponMagazinedWGrenade::UpdateSounds()
{
    inherited::UpdateSounds();
    Fvector P = get_LastFP();
    m_sounds.SetPosition("sndShotG", P);
    m_sounds.SetPosition("sndReloadG", P);
    m_sounds.SetPosition("sndSwitch", P);
}

void CWeaponMagazinedWGrenade::UpdateGrenadeVisibility(bool visibility)
{
    if (!GetHUDmode())
        return;
    HudItemData()->set_bone_visible("grenade", visibility, TRUE);
}

void CWeaponMagazinedWGrenade::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_bGrenadeMode, output_packet);
    save_data((u32)0, output_packet);
}

void CWeaponMagazinedWGrenade::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_bGrenadeMode, input_packet);
    u32 dummy;
    load_data(dummy, input_packet);
}

void CWeaponMagazinedWGrenade::net_Export(NET_Packet& P)
{
    P.w_u8(m_bGrenadeMode ? 1 : 0);
    inherited::net_Export(P);
}

void CWeaponMagazinedWGrenade::net_Import(NET_Packet& P)
{
    m_bGrenadeMode = !!P.r_u8();
    inherited::net_Import(P);
}

float CWeaponMagazinedWGrenade::Weight() const
{
    float res = inherited::Weight();
    res += GetMagazineWeight(m_magazine2);

    return res;
}

bool CWeaponMagazinedWGrenade::IsNecessaryItem(const shared_str& item_sect)
{
    return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() ||
        std::find(m_ammoTypes2.begin(), m_ammoTypes2.end(), item_sect) != m_ammoTypes2.end());
}

u8 CWeaponMagazinedWGrenade::GetCurrentHudOffsetIdx()
{
    bool b_aiming = ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) ||
        (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f));

    if (!b_aiming)
        return 0;
    else
        // Альт. прицел
        if (m_bGrenadeMode)
            return 2;
        else if (m_zoomtype == 1)
            return 3;
        else
            return 1;
}

bool CWeaponMagazinedWGrenade::install_upgrade_ammo_class(LPCSTR section, bool test)
{
    LPCSTR str;

	bool result = process_if_exists(section, "ammo_mag_size", &CInifile::r_s32, m_bGrenadeMode ? iMagazineSize2 : iMagazineSize, test);

    //	ammo_class = ammo_5.45x39_fmj, ammo_5.45x39_ap  // name of the ltx-section of used ammo
    bool result2 = process_if_exists_set(section, "ammo_class", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        xr_vector<shared_str>& ammo_types = !m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
        ammo_types.clear();
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 ammo_item;
            _GetItem(str, i, ammo_item);
            ammo_types.push_back(ammo_item);
        }

        m_ammoType.data = 0;
        SyncronizeWeaponToServer();
    }
    result |= result2;

    return result2;
}

bool CWeaponMagazinedWGrenade::install_upgrade_impl(LPCSTR section, bool test)
{
    LPCSTR str;
    bool result = inherited::install_upgrade_impl(section, test);

    //	grenade_class = ammo_vog-25, ammo_vog-25p          // name of the ltx-section of used grenades
    bool result2 = process_if_exists_set(section, "grenade_class", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        xr_vector<shared_str>& ammo_types = !m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
        ammo_types.clear();
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 ammo_item;
            _GetItem(str, i, ammo_item);
            ammo_types.push_back(ammo_item);
        }

        m_ammoType.data = 0;
        SyncronizeWeaponToServer();
    }
    result |= result2;

    result |= process_if_exists(section, "launch_speed", &CInifile::r_float, m_fLaunchSpeed, test);

    result2 = process_if_exists_set(section, "snd_shoot_grenade", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", false, m_eSoundShot);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_reload_grenade", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
    }
    result |= result2;

    result2 = process_if_exists_set(section, "snd_switch", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);
    }
    result |= result2;

    return result;
}

#include "string_table.h"
bool CWeaponMagazinedWGrenade::GetBriefInfo(II_BriefInfo& info)
{
    VERIFY(m_pInventory);
    if (!inherited::GetBriefInfo(info))
        return false;

    string32 int_str;

	int ae = GetAmmoElapsed();

	if (bHasBulletsToHide && !m_bGrenadeMode)
    {
        last_hide_bullet = ae >= bullet_cnt ? bullet_cnt : bullet_cnt - ae - 1;
        if (ae == 0)
            last_hide_bullet = -1;
    }

    if (m_bGrenadeMode || !IsGrenadeLauncherAttached())
    {
        info.grenade = "";
        return false;
    }

    int total2 = GetAmmoCount2(0);
    if (unlimited_ammo())
        xr_sprintf(int_str, "--");
    else
    {
        if (total2)
            xr_sprintf(int_str, "%d", total2);
        else
            xr_sprintf(int_str, "X");
    }
    info.grenade = int_str;

    return true;
}

int CWeaponMagazinedWGrenade::GetAmmoCount2(u8 ammo2_type) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo2_type < m_ammoTypes2.size());

    return GetAmmoCount_forType(m_ammoTypes2[ammo2_type]);
}

void CWeaponMagazinedWGrenade::switch2_Unmis()
{
    if (m_bGrenadeMode || !ParentIsActor())
    {
        m_bNeedBulletInGun = false;
        return;
    }
    VERIFY(GetState() == eUnMisfire);
    if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
    {
        if (m_sounds_enabled)
        {
            if (m_sounds.FindSoundItem("sndReloadMisfire", false) && isHUDAnimationExist("anm_reload_misfire_w_gl"))
                PlaySound("sndReloadMisfire", get_LastFP());
            else if (m_sounds.FindSoundItem("sndReloadEmpty", false) && isHUDAnimationExist("anm_reload_misfire_w_gl"))
                PlaySound("sndReloadEmpty", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }

        if (isHUDAnimationExist("anm_reload_misfire_w_gl"))
            PlayHUDMotion("anm_reload_misfire_w_gl", TRUE, this, GetState());
        else if (isHUDAnimationExist("anm_reload_empty_w_gl"))
            PlayHUDMotion("anm_reload_empty_w_gl", TRUE, this, GetState());
        else
            PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
    }
    else
        inherited::switch2_Unmis();
}

void CWeaponMagazinedWGrenade::CheckMagazine()
{
    if (m_bGrenadeMode)
        return;

    if (m_bCartridgeInTheChamber == true && (isHUDAnimationExist("anm_reload_empty_w_gl") || isHUDAnimationExist("anm_reload_empty")) && m_ammoElapsed.type1 >= 1 && m_bNeedBulletInGun == false)
    {
        m_bNeedBulletInGun = true;
    }
    else if ((isHUDAnimationExist("anm_reload_empty_w_gl") || isHUDAnimationExist("anm_reload_empty")) && m_ammoElapsed.type1 == 0 && m_bNeedBulletInGun == true)
    {
        m_bNeedBulletInGun = false;
    }
}
