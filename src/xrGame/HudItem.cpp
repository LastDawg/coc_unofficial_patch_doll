#include "stdafx.h"
#include "HudItem.h"
#include "physic_item.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Missile.h"
#include "xrMessages.h"
#include "Level.h"
#include "Inventory.h"
#include "xrEngine/CameraBase.h"
#include "player_hud.h"
#include "xrCore/Animation/SkeletonMotions.hpp"

#include "xrUICore/ui_base.h"

#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "HUDManager.h"

extern const float PITCH_OFFSET_R = 0.00f; // barrel movement sideways (to the left) with vertical camera turns
extern const float PITCH_OFFSET_N = 0.00f; // barrel rise / fall with vertical camera turns
extern const float PITCH_OFFSET_D = 0.00f; // barrel toward / away when the camera rotates vertically
extern const float PITCH_LOW_LIMIT = 0.14f; // minimum pitch value when used in conjunction with PITCH_OFFSET_N
extern const float ORIGIN_OFFSET = -0.05f; // inertia factor influence on position of torso (the smaller, the larger the inertia)
extern const float ORIGIN_OFFSET_AIM = -0.03f; // (zoomed inertia factor)
extern const float TENDTO_SPEED = 5.f; // barrel return speed
extern const float TENDTO_SPEED_AIM = 8.f; // (zoomed return speed)

ENGINE_API extern float psHUD_FOV_def; //--#SM+#--

CHudItem::CHudItem()
{
    RenderHud(TRUE);
    EnableHudInertion(TRUE);
    AllowHudInertion(TRUE);
    m_bStopAtEndAnimIsRunning = false;
    m_current_motion_def = NULL;
    m_started_rnd_anim_idx = u8(-1);
	m_nearwall_last_hud_fov = psHUD_FOV_def;
    m_fLR_MovingFactor = 0.f;
    m_fLR_CameraFactor = 0.f;
    m_fLR_InertiaFactor = 0.f;
    m_fUD_InertiaFactor = 0.f;

	// inertion
    m_inertion_params.m_origin_offset = ORIGIN_OFFSET;
    m_inertion_params.m_origin_offset_aim = ORIGIN_OFFSET_AIM;
    m_inertion_params.m_pitch_low_limit = PITCH_LOW_LIMIT;
    m_inertion_params.m_pitch_offset_d = PITCH_OFFSET_D;
    m_inertion_params.m_pitch_offset_n = PITCH_OFFSET_N;
    m_inertion_params.m_pitch_offset_r = PITCH_OFFSET_R;
    m_inertion_params.m_tendto_speed = TENDTO_SPEED;
    m_inertion_params.m_tendto_speed_aim = TENDTO_SPEED_AIM;
}

IFactoryObject* CHudItem::_construct()
{
    m_object = smart_cast<CPhysicItem*>(this);
    VERIFY(m_object);

    m_item = smart_cast<CInventoryItem*>(this);
    VERIFY(m_item);

    return (m_object);
}

CHudItem::~CHudItem() {}

void CHudItem::Load(LPCSTR section)
{
    hud_sect = pSettings->r_string(section, "hud");
    m_animation_slot = pSettings->r_u32(section, "animation_slot");

    m_sounds.LoadSound(section, "snd_bore", "sndBore", true);
    m_sounds.LoadSound(section, "snd_bore_jammed", "sndBoreJammed", true);

	m_strafe_offset[0] = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "strafe_hud_offset_pos", Fvector().set(0.015f, 0.0f, 0.0f));
    m_strafe_offset[1] = READ_IF_EXISTS(pSettings, r_fvector3, hud_sect, "strafe_hud_offset_rot", Fvector().set(0.0f, 0.0f, 4.5f));

    float fFullStrafeTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "strafe_transition_time", 0.25f);
    bool bStrafeEnabled = !!READ_IF_EXISTS(pSettings, r_bool, hud_sect, "strafe_enabled", TRUE);

    m_strafe_offset[2].set(bStrafeEnabled ? 1.0f : 0.0f, fFullStrafeTime, 0.f); // normal

    m_nearwall_enabled = READ_IF_EXISTS(pSettings, r_bool, section, "nearwall_on", true);
	m_hud_fov_add_mod = READ_IF_EXISTS(pSettings, r_float, hud_sect, "hud_fov_addition_modifier", 0.f);
    m_nearwall_dist_min = READ_IF_EXISTS(pSettings, r_float, hud_sect, "nearwall_dist_min", 0.5f);
    m_nearwall_dist_max = READ_IF_EXISTS(pSettings, r_float, hud_sect, "nearwall_dist_max", 1.f);
    m_nearwall_target_hud_fov = READ_IF_EXISTS(pSettings, r_float, hud_sect, "nearwall_target_hud_fov", 0.45f);
    m_nearwall_speed_mod = READ_IF_EXISTS(pSettings, r_float, hud_sect, "nearwall_speed_mod", 10.f);

	m_inertion_params.m_pitch_offset_r = READ_IF_EXISTS(pSettings, r_float, section, "pitch_offset_right", PITCH_OFFSET_R);
	m_inertion_params.m_pitch_offset_n = READ_IF_EXISTS(pSettings, r_float, section, "pitch_offset_up", PITCH_OFFSET_N);
	m_inertion_params.m_pitch_offset_d = READ_IF_EXISTS(pSettings, r_float, section, "pitch_offset_forward", PITCH_OFFSET_D);
	m_inertion_params.m_pitch_low_limit = READ_IF_EXISTS(pSettings, r_float, section, "pitch_offset_up_low_limit", PITCH_LOW_LIMIT);

	m_inertion_params.m_origin_offset = READ_IF_EXISTS(pSettings, r_float, section, "inertion_origin_offset", ORIGIN_OFFSET);
	m_inertion_params.m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, section, "inertion_origin_aim_offset", ORIGIN_OFFSET_AIM);
	m_inertion_params.m_tendto_speed = READ_IF_EXISTS(pSettings, r_float, section, "inertion_tendto_speed", TENDTO_SPEED);
	m_inertion_params.m_tendto_speed_aim = READ_IF_EXISTS(pSettings, r_float, section, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);
}

void CHudItem::PlaySound(LPCSTR alias, const Fvector& position)
{
    m_sounds.PlaySound(alias, position, object().H_Root(), !!GetHUDmode());
}

//Alundaio: Play at index
void CHudItem::PlaySound(pcstr alias, const Fvector& position, u8 index)
{
    m_sounds.PlaySound(alias, position, object().H_Root(), !!GetHUDmode(), false, index);
}
//-Alundaio

void CHudItem::renderable_Render()
{
    UpdateXForm();
    BOOL _hud_render = GEnv.Render->get_HUD() && GetHUDmode();

    if (_hud_render && !IsHidden())
    {
    }
    else
    {
        if (!object().H_Parent() || (!_hud_render && !IsHidden()))
        {
            on_renderable_Render();
            debug_draw_firedeps();
        }
        else if (object().H_Parent())
        {
            CInventoryOwner* owner = smart_cast<CInventoryOwner*>(object().H_Parent());
            VERIFY(owner);
            CInventoryItem* self = smart_cast<CInventoryItem*>(this);
            if (owner->attached(self))
                on_renderable_Render();
        }
    }
}

void CHudItem::SwitchState(u32 S)
{
    if (OnClient())
        return;

    SetNextState(S);

    if (object().Local() && !object().getDestroy())
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        object().u_EventGen(P, GE_WPN_STATE_CHANGE, object().ID());
        P.w_u8(u8(S));
        object().u_EventSend(P);
    }
}

void CHudItem::OnEvent(NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_WPN_STATE_CHANGE:
    {
        u8 S;
        P.r_u8(S);
        OnStateSwitch(u32(S), GetState());
    }
    break;
    }
}

void CHudItem::OnStateSwitch(u32 S, u32 oldState)
{
    SetState(S);

    if (object().Remote())
        SetNextState(S);

    switch (S)
    {
    case eSprintStart: 
        SetPending(FALSE);

        PlayAnimSprintStart();
    break;

    case eSprintEnd: 
        SetPending(FALSE);

        PlayAnimSprintEnd();
    break;

    case eBore:
        SetPending(FALSE);

        PlayAnimBore();
        if (HudItemData())
        {
            Fvector P = HudItemData()->m_item_transform.c;
            if (m_sounds.FindSoundItem("sndBore", false) && !IsMisfireNow())
                m_sounds.PlaySound("sndBore", P, object().H_Root(), !!GetHUDmode(), false, m_started_rnd_anim_idx);
            if (m_sounds.FindSoundItem("sndBoreJammed", false) && IsMisfireNow())
                m_sounds.PlaySound("sndBoreJammed", P, object().H_Root(), !!GetHUDmode(), false, m_started_rnd_anim_idx);
        }
        break;
    }
    g_player_hud->updateMovementLayerState();
}

void CHudItem::OnAnimationEnd(u32 state)
{
    CActor* A = smart_cast<CActor*>(object().H_Parent());
    if (A)
        A->callback(GameObject::eActorHudAnimationEnd)(smart_cast<CGameObject*>(this)->lua_game_object(),
                                                       this->hud_sect.c_str(), this->m_current_motion.c_str(), state,
                                                       this->animation_slot());

    switch (state)
    {
        case eBore: 
        { 
            SwitchState(eIdle);
        }
        break;
        case eSprintStart: 
        {
            SprintType = true;
            SwitchState(eIdle);
        }
        break;
        case eSprintEnd: 
        {
            SprintType = false;
            SwitchState(eIdle);
        }
    break;
    }
}

void CHudItem::PlayAnimBore() 
{ 
    PlayHUDMotion("anm_bore", TRUE, this, GetState()); 
}

bool CHudItem::ActivateItem()
{
    OnActiveItem();
    return true;
}

void CHudItem::DeactivateItem() { OnHiddenItem(); }
void CHudItem::OnMoveToRuck(const SInvItemPlace& prev) { SwitchState(eHidden); }
void CHudItem::SendDeactivateItem() { SendHiddenItem(); }
void CHudItem::SendHiddenItem()
{
    if (!object().getDestroy())
    {
        NET_Packet P;
        object().u_EventGen(P, GE_WPN_STATE_CHANGE, object().ID());
        P.w_u8(u8(eHiding));
        object().u_EventSend(P);
    }
}

void __inertion(float& _val_cur, const float& _val_trgt, const float& _friction)
{
    float friction_i = 1.f - _friction;
    _val_cur = _val_cur * _friction + _val_trgt * friction_i;
}

void CHudItem::UpdateHudAdditional(Fmatrix& hud_trans)
{
    attachable_hud_item* hi = HudItemData();
    R_ASSERT(hi);

    CActor* pActor = smart_cast<CActor*>(object().H_Parent());
    if (!pActor)
        return;

	float fInertiaPower = GetInertionPowerFactor();

    u32 iMovingState = pActor->MovingState();

    float fYMag = pActor->fFPCamYawMagnitude;
    float fPMag = pActor->fFPCamPitchMagnitude;

    static float fAvgTimeDelta = Device.fTimeDelta;
    __inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

    //============= Aieiaie no?aeo n i?o?eai =============//
    float fStrafeMaxTime =
        m_strafe_offset[2].y; // Iaen. a?aiy a naeoiaao, ca eioi?ia iu iaeeiieiny ec oaio?aeuiiai iiei?aiey
    if (fStrafeMaxTime <= EPS)
        fStrafeMaxTime = 0.01f;

    float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // Aaee?eia eciaiaiea oaeoi?a iiai?ioa

    // Aiaaaeyai aieiaie iaeeii io aae?aiey eaia?u
    float fCamReturnSpeedMod = 1.5f;

    float fCamLimit = 0.8f;

    // N?eoaai no?aeo io iiai?ioa eaia?u
    if (fYMag != 0.0f)
    { //--> Eaia?a e?ooeony ii ine Y
        m_fLR_CameraFactor -= (fYMag * 0.005f);

        float fCamLimitBlend = 1.0f - ((1.0f - fCamLimit) * 1.0f);
        clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
    }
    else
    { //--> Eaia?a ia iiai?a?eaaaony - oae?aai iaeeii
        if (m_fLR_CameraFactor < 0.0f)
        {
            m_fLR_CameraFactor += fStepPerUpd * (fCamReturnSpeedMod);
            clamp(m_fLR_CameraFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_CameraFactor -= fStepPerUpd * (fCamReturnSpeedMod);
            clamp(m_fLR_CameraFactor, 0.0f, 1.0f);
        }
    }
    // Aiaaaeyai aieiaie iaeeii io oiauau aaie
    float fChangeDirSpeedMod =
        3; // Aineieuei auno?i iaiyai iai?aaeaiea iai?aaeaiea iaeeiia, anee iii a a?oao? noi?iio io oaeouaai

    if ((iMovingState & mcLStrafe) != 0)
    { // Aae?ainy aeaai
        float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
        m_fLR_MovingFactor -= fVal;
    }
    else if ((iMovingState & mcRStrafe) != 0)
    { // Aae?ainy ai?aai
        float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
        m_fLR_MovingFactor += fVal;
    }
    else
    { // Aaeaaainy a e?aii a?oaii iai?aaeaiee - ieaaii oae?aai iaeeii
        if (m_fLR_MovingFactor < 0.0f)
        {
            m_fLR_MovingFactor += fStepPerUpd;
            clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_MovingFactor -= fStepPerUpd;
            clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
        }
    }

    clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Oaeoi? aieiaie oiauau ia aie?ai i?aauoaou yoe eeieou

    // Au?eneyai e ii?iaeece?oai eoiaiaue oaeoi? iaeeiia
    float fLR_Factor = m_fLR_MovingFactor + m_fLR_CameraFactor;
    clamp(fLR_Factor, -1.0f, 1.0f); // Oaeoi? aieiaie oiauau ia aie?ai i?aauoaou yoe eeieou

    float fTrgStrafe = 1.f + fLR_Factor * (-1.f - 1.f);
    float mStrafeFactor = fLR_Factor * (1 - fStepPerUpd) + fTrgStrafe * fStepPerUpd;

    // I?iecaiaei iaeeii noaiea aey ii?iaeuiiai ?a?eia e aeia
    if (m_strafe_offset[2].x != 0.0f) //<-- Aey ieaaiiai ia?aoiaa
    {
        Fvector curr_offs, curr_rot;

        // Niauaiea iiceoee ooaa a no?aeoa
        curr_offs = m_strafe_offset[0]; // pos
        curr_offs.mul(mStrafeFactor); // Oiii?aai ia oaeoi? no?aeoa

        // Iiai?io ooaa a no?aeoa
        curr_rot = m_strafe_offset[1]; // rot
        curr_rot.mul(-PI / 180.f); // I?aia?acoai oaeu a ?aaeaiu
        curr_rot.mul(mStrafeFactor); // Oiii?aai ia oaeoi? no?aeoa

        Fmatrix hud_rotation;
        Fmatrix hud_rotation_y;

        hud_rotation.identity();
        hud_rotation.rotateX(curr_rot.x);

        hud_rotation_y.identity();
        hud_rotation_y.rotateY(curr_rot.y);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation_y.identity();
        hud_rotation_y.rotateZ(curr_rot.z);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation.translate_over(curr_offs);
        hud_trans.mulB_43(hud_rotation);
    }

    //============= Eia?oey i?o?ey =============//
    // Ia?aiao?u eia?oee
    float fInertiaSpeedMod = hi->m_measures.m_inertion_params.m_tendto_speed;

    float fInertiaReturnSpeedMod = hi->m_measures.m_inertion_params.m_tendto_ret_speed;

    float fInertiaMinAngle = hi->m_measures.m_inertion_params.m_min_angle;

    Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
    vIOffsets.x = hi->m_measures.m_inertion_params.m_offset_LRUD.x * fInertiaPower;
    vIOffsets.y = hi->m_measures.m_inertion_params.m_offset_LRUD.y * fInertiaPower;
    vIOffsets.z = hi->m_measures.m_inertion_params.m_offset_LRUD.z * fInertiaPower;
    vIOffsets.w = hi->m_measures.m_inertion_params.m_offset_LRUD.w * fInertiaPower;

    // Aun?eouaaai eia?oe? ec iiai?ioia eaia?u
    bool bIsInertionPresent = m_fLR_InertiaFactor != 0.0f || m_fUD_InertiaFactor != 0.0f;
    if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
    {
        float fSpeed = fInertiaSpeedMod;
        if (fYMag > 0.0f && m_fLR_InertiaFactor > 0.0f || fYMag < 0.0f && m_fLR_InertiaFactor < 0.0f)
        {
            fSpeed *= 2.f; //--> Onei?yai eia?oe? i?e aae?aiee a i?ioeaiiiei?io? noi?iio
        }

        m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Ai?eciioaeu (i.a. > |1.0|)
    }

    if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
    {
        float fSpeed = fInertiaSpeedMod;
        if (fPMag > 0.0f && m_fUD_InertiaFactor > 0.0f || fPMag < 0.0f && m_fUD_InertiaFactor < 0.0f)
        {
            fSpeed *= 2.f; //--> Onei?yai eia?oe? i?e aae?aiee a i?ioeaiiiei?io? noi?iio
        }

        m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Aa?oeeaeu (i.a. > |1.0|)
    }

    clamp(m_fLR_InertiaFactor, -1.0f, 1.0f);
    clamp(m_fUD_InertiaFactor, -1.0f, 1.0f);

    // Ieaaiia caoooaiea eia?oee (iniiaiia, ii aac eeiaeiie ieeiaaa ia iionoeo eia?oe? ai iieiiai 0.0f)
    m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
    m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

    // Ieieiaeuiia eeiaeiia caoooaiea eia?oee i?e iieia (ai?eciioaeu)
    if (fYMag == 0.0f)
    {
        float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
        if (m_fLR_InertiaFactor < 0.0f)
        {
            m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_InertiaFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_InertiaFactor, 0.0f, 1.0f);
        }
    }

    // Ieieiaeuiia eeiaeiia caoooaiea eia?oee i?e iieia (aa?oeeaeu)
    if (fPMag == 0.0f)
    {
        float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
        if (m_fUD_InertiaFactor < 0.0f)
        {
            m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_InertiaFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_InertiaFactor, 0.0f, 1.0f);
        }
    }

    // I?eiaiyai eia?oe? e ooao
    float fLR_lim = (m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
    float fUD_lim = (m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

    Fvector curr_offs;
    curr_offs = {fLR_lim * -1.f * m_fLR_InertiaFactor, fUD_lim * m_fUD_InertiaFactor, 0.0f};

    Fmatrix hud_rotation;
    hud_rotation.identity();
    hud_rotation.translate_over(curr_offs);
    hud_trans.mulB_43(hud_rotation);
}
void CHudItem::UpdateCL()
{
    if (m_current_motion_def)
    {
        if (m_bStopAtEndAnimIsRunning)
        {
            const xr_vector<motion_marks>& marks = m_current_motion_def->marks;
            if (!marks.empty())
            {
                float motion_prev_time = ((float)m_dwMotionCurrTm - (float)m_dwMotionStartTm) / 1000.0f;
                float motion_curr_time = ((float)Device.dwTimeGlobal - (float)m_dwMotionStartTm) / 1000.0f;

                xr_vector<motion_marks>::const_iterator it = marks.begin();
                xr_vector<motion_marks>::const_iterator it_e = marks.end();
                for (; it != it_e; ++it)
                {
                    const motion_marks& M = (*it);
                    if (M.is_empty())
                        continue;

                    const motion_marks::interval* Iprev = M.pick_mark(motion_prev_time);
                    const motion_marks::interval* Icurr = M.pick_mark(motion_curr_time);
                    if (Iprev == NULL && Icurr != NULL /* || M.is_mark_between(motion_prev_time, motion_curr_time)*/)
                    {
                        OnMotionMark(m_startedMotionState, M);
                    }
                }
            }

            m_dwMotionCurrTm = Device.dwTimeGlobal;
            if (m_dwMotionCurrTm > m_dwMotionEndTm)
            {
                m_current_motion_def = NULL;
                m_dwMotionStartTm = 0;
                m_dwMotionEndTm = 0;
                m_dwMotionCurrTm = 0;
                m_bStopAtEndAnimIsRunning = false;
                OnAnimationEnd(m_startedMotionState);
            }
        }
    }
}

void CHudItem::OnH_A_Chield() {}

void CHudItem::OnH_B_Chield() 
{ 
    StopCurrentAnimWithoutCallback();
    m_nearwall_last_hud_fov = psHUD_FOV_def;
}

void CHudItem::OnH_B_Independent(bool just_before_destroy)
{
    m_sounds.StopAllSounds();
    UpdateXForm();
    m_nearwall_last_hud_fov = psHUD_FOV_def;

    // next code was commented
    /*
    if(HudItemData() && !just_before_destroy)
    {
        object().XFORM().set( HudItemData()->m_item_transform );
    }

    if (HudItemData())
    {
        g_player_hud->detach_item(this);
        Msg("---Detaching hud item [%s][%d]", this->HudSection().c_str(), this->object().ID());
    }*/
    // SetHudItemData			(NULL);
}

void CHudItem::OnH_A_Independent()
{
    if (HudItemData())
        g_player_hud->detach_item(this);
    StopCurrentAnimWithoutCallback();
}

void CHudItem::on_b_hud_detach() { m_sounds.StopAllSounds(); }
void CHudItem::on_a_hud_attach()
{
    if (m_current_motion_def)
    {
        PlayHUDMotion_noCB(m_current_motion, FALSE);
#ifdef DEBUG
//		Msg("continue playing [%s][%d]",m_current_motion.c_str(), Device.dwFrame);
#endif // #ifdef DEBUG
    }
    else
    {
#ifdef DEBUG
//		Msg("no active motion");
#endif // #ifdef DEBUG
    }
}

u32 CHudItem::PlayHUDMotion(const shared_str& M, bool bMixIn, CHudItem* W, u32 state)
{
    u32 anim_time = PlayHUDMotion_noCB(M, bMixIn);
    if (anim_time > 0)
    {
        m_bStopAtEndAnimIsRunning = true;
        m_dwMotionStartTm = Device.dwTimeGlobal;
        m_dwMotionCurrTm = m_dwMotionStartTm;
        m_dwMotionEndTm = m_dwMotionStartTm + anim_time;
        m_startedMotionState = state;
    }
    else
        m_bStopAtEndAnimIsRunning = false;

    return anim_time;
}

u32 CHudItem::PlayHUDMotionNew(const shared_str& M, const bool bMixIn, const u32 state, const bool randomAnim)
{
    // Msg("~~[%s] Playing motion [%s] for [%s]", __FUNCTION__, M.c_str(), HudSection().c_str());
    u32 anim_time = PlayHUDMotion_noCB(M, bMixIn);
    if (anim_time > 0)
    {
        m_bStopAtEndAnimIsRunning = true;
        m_dwMotionStartTm = Device.dwTimeGlobal;
        m_dwMotionCurrTm = m_dwMotionStartTm;
        m_dwMotionEndTm = m_dwMotionStartTm + anim_time;
        m_startedMotionState = state;
    }
    else
        m_bStopAtEndAnimIsRunning = false;

    return anim_time;
}

u32 CHudItem::PlayHUDMotionIfExists(std::initializer_list<const char*> Ms, const bool bMixIn, const u32 state, const bool randomAnim)
{
    for (const auto* M : Ms)
        if (isHUDAnimationExist(M))
            return PlayHUDMotionNew(M, bMixIn, state, randomAnim);

    std::string dbg_anim_name;
    for (const auto* M : Ms)
    {
        dbg_anim_name += M;
        dbg_anim_name += ", ";
    }
    Msg("~~[%s] Motions [%s] not found for [%s]", __FUNCTION__, dbg_anim_name.c_str(), HudSection().c_str());

    return 0;
}

u32 CHudItem::PlayHUDMotion_noCB(const shared_str& motion_name, const bool bMixIn, const bool randomAnim)
{
    m_current_motion = motion_name;

    if (bDebug && item().m_pInventory)
    {
        Msg("-[%s] as[%d] [%d]anim_play [%s][%d]", HudItemData() ? "HUD" : "Simulating",
            item().m_pInventory->GetActiveSlot(), item().object_id(), motion_name.c_str(), Device.dwFrame);
    }
    if (HudItemData())
    {
        return HudItemData()->anim_play(motion_name, bMixIn, m_current_motion_def, m_started_rnd_anim_idx);
    }
    else
    {
        m_started_rnd_anim_idx = 0;
        return g_player_hud->motion_length(motion_name, HudSection(), m_current_motion_def);
    }
}

void CHudItem::StopCurrentAnimWithoutCallback()
{
    m_dwMotionStartTm = 0;
    m_dwMotionEndTm = 0;
    m_dwMotionCurrTm = 0;
    m_bStopAtEndAnimIsRunning = false;
    m_current_motion_def = NULL;
}

BOOL CHudItem::GetHUDmode()
{
    if (object().H_Parent())
    {
        CActor* A = smart_cast<CActor*>(object().H_Parent());
        return (A && A->HUDview() && HudItemData());
    }
    else
        return FALSE;
}

void CHudItem::PlayBlendAnm(LPCSTR name, float speed, float power, bool stop_old)
{
    u8 part = (object().cast_weapon()->IsZoomed() ? 2 : (g_player_hud->attached_item(1) ? 0 : 2));

    if (stop_old)
        g_player_hud->StopBlendAnm(name, true);
    g_player_hud->PlayBlendAnm(name, part, speed, power, false);
}

void CHudItem::PlayAnimIdle()
{
    if (TryPlayAnimIdle())
        return;

    PlayHUDMotion("anm_idle", TRUE, NULL, GetState());
}

void CHudItem::PlayAnimSprintStart()
{
	auto wpn = smart_cast<CWeapon*>(this);
	string128 guns_sprint_start_anm;
	xr_strconcat(guns_sprint_start_anm, "anm_idle_sprint_start", (wpn && wpn->IsMisfire()) ? "_jammed" : ((wpn && ((wpn->GetAmmoElapsed() == 0 && !wpn->m_bGrenadeMode) || (wpn->GetAmmoElapsed() == 0 && wpn->m_bGrenadeMode))) ? "_empty" : ""), (wpn && wpn->IsGrenadeLauncherAttached()) ? (wpn && wpn->m_bGrenadeMode ? "_g" : "_w_gl") : "");
    if (isHUDAnimationExist(guns_sprint_start_anm))
        PlayHUDMotion(guns_sprint_start_anm, true, nullptr, GetState());
	else 
    {
		SprintType = true;
		SwitchState(eIdle);
	}
}

void CHudItem::PlayAnimSprintEnd()
{
	auto wpn = smart_cast<CWeapon*>(this);
	string128 guns_sprint_end_anm;
	xr_strconcat(guns_sprint_end_anm, "anm_idle_sprint_end", (wpn && wpn->IsMisfire()) ? "_jammed" : ((wpn && ((wpn->GetAmmoElapsed() == 0 && !wpn->m_bGrenadeMode) || (wpn->GetAmmoElapsed() == 0 && wpn->m_bGrenadeMode))) ? "_empty" : ""), (wpn && wpn->IsGrenadeLauncherAttached()) ? (wpn && wpn->m_bGrenadeMode ? "_g" : "_w_gl") : "");
    if (isHUDAnimationExist(guns_sprint_end_anm))
        PlayHUDMotion(guns_sprint_end_anm, true, nullptr, GetState());
	else 
    {
		SprintType = false;
		SwitchState(eIdle);
	}
}

bool CHudItem::TryPlayAnimIdle()
{
    if (MovingAnimAllowedNow())
    {
        CActor* pActor = smart_cast<CActor*>(object().H_Parent());
        if (pActor)
        {
            CEntity::SEntityState st;
            pActor->g_State(st);
            if (st.bSprint)
            {
                if (!SprintType)
                {
                    SwitchState(eSprintStart);
                    return true;
                }
                PlayAnimIdleSprint();
                return true;
            }
            else 
                if (SprintType)
                {
                    SwitchState(eSprintEnd);
                    return true;
                }

            if (pActor->AnyMove())
            {
                if (!st.bCrouch)
                {
                    PlayAnimIdleMoving();
                    return true;
                }
                if (st.bCrouch)
                {
                    PlayAnimIdleMovingCrouch();
                    return true;
                }
            }
        }
    }
    return false;
}

//AVO: check if animation exists
bool CHudItem::isHUDAnimationExist(pcstr anim_name) const
{
    if (HudItemData()) // First person
    {
        string256 anim_name_r;
        bool is_16x9 = UI().is_widescreen();
        u16 attach_place_idx = pSettings->r_u16(HudItemData()->m_sect_name, "attach_place_idx");
        xr_sprintf(anim_name_r, "%s%s", anim_name, (attach_place_idx == 1 && is_16x9) ? "_16x9" : "");
        player_hud_motion* anm = HudItemData()->m_hand_motions.find_motion(anim_name_r);
        if (anm)
            return true;
    }
    else // Third person
    {
        const CMotionDef* temp_motion_def;
        if (g_player_hud->motion_length(anim_name, HudSection(), temp_motion_def) > 100)
            return true;
    }
#ifdef DEBUG
    Msg("~ [WARNING] ------ Animation [%s] does not exist in [%s]", anim_name, HudSection().c_str());
#endif
    return false;
}

bool CHudItem::NeedBlendAnm()
{
    u32 state = GetState();
    return (state != eIdle && state != eHidden);
}

void CHudItem::PlayAnimIdleMoving() 
{ 
    if (IsMisfireNow() && isHUDAnimationExist("anm_idle_moving_jammed"))
        PlayHUDMotion("anm_idle_moving_jammed", true, nullptr, GetState());
    else
        PlayHUDMotion("anm_idle_moving", true, nullptr, GetState());
}

void CHudItem::PlayAnimIdleMovingCrouch()
{
    if (IsMisfireNow() && isHUDAnimationExist("anm_idle_moving_crouch_jammed"))
        PlayHUDMotion("anm_idle_moving_crouch_jammed", true, nullptr, GetState());
    else if (!IsMisfireNow() && isHUDAnimationExist("anm_idle_moving_crouch"))
        PlayHUDMotion("anm_idle_moving_crouch", true, nullptr, GetState());
}

void CHudItem::PlayAnimIdleSprint() 
{ 
    if (IsMisfireNow() && isHUDAnimationExist("anm_idle_sprint_jammed"))
        PlayHUDMotion("anm_idle_sprint_jammed", true, nullptr, GetState());
    else
        PlayHUDMotion("anm_idle_sprint", true, nullptr, GetState());
}

void CHudItem::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
    if (GetState() == eIdle && !m_bStopAtEndAnimIsRunning)
    {
        if ((cmd == ACTOR_DEFS::mcSprint) || (cmd == ACTOR_DEFS::mcAnyMove))
        {
            PlayAnimIdle();
            ResetSubStateTime();
        }
    }
}

attachable_hud_item* CHudItem::HudItemData() const
{
    attachable_hud_item* hi = NULL;
    if (!g_player_hud)
        return hi;

    hi = g_player_hud->attached_item(0);
    if (hi && hi->m_parent_hud_item == this)
        return hi;

    hi = g_player_hud->attached_item(1);
    if (hi && hi->m_parent_hud_item == this)
        return hi;

    return NULL;
}

BOOL CHudItem::ParentIsActor()
{
    IGameObject* O = object().H_Parent();
    if (!O)
        return false;

    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    if (!EA)
        return false;

    return !!EA->cast_actor();
}

void CHudItem::ReplaceHudSection(LPCSTR hud_section)
{
    if (hud_section != hud_sect)
        hud_sect = hud_section;
}

float CHudItem::GetHudFov()
{
    if (m_nearwall_enabled && ParentIsActor() && (Level().CurrentViewEntity() == object().H_Parent()))
    {
        collide::rq_result& RQ = HUD().GetCurrentRayQuery();
        float dist = RQ.range;

        clamp(dist, m_nearwall_dist_min, m_nearwall_dist_max);
        float fDistanceMod =
            ((dist - m_nearwall_dist_min) / (m_nearwall_dist_max - m_nearwall_dist_min)); // 0.f ... 1.f

        float fBaseFov = psHUD_FOV_def + m_hud_fov_add_mod;
        clamp(fBaseFov, 0.0f, FLT_MAX);

        float src = m_nearwall_speed_mod * Device.fTimeDelta;
        clamp(src, 0.f, 1.f);

        float fTrgFov = m_nearwall_target_hud_fov + fDistanceMod * (fBaseFov - m_nearwall_target_hud_fov);
        m_nearwall_last_hud_fov = m_nearwall_last_hud_fov * (1 - src) + fTrgFov * src;
    }
    return m_nearwall_last_hud_fov;
}
