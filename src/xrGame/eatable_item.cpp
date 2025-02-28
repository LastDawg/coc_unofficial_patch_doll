////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "xrmessages.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "UIGameCustom.h"
#include "ui/UIActorMenu.h"

#include "Actor.h"
#include "Inventory.h"
#include "game_object_space.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Weapon.h"
#include "actorEffector.h"
#include "HudManager.h"
#include "player_hud.h"
#include "../xrPhysics/ElevatorState.h"
#include "CustomDetector.h"
#include "Flashlight.h"
#include "../xrEngine/x_ray.h"
#include "XrayGameConstants.h"
#include "CustomOutfit.h"

extern bool g_block_all_except_movement;

CEatableItem::CEatableItem() : m_fWeightFull(0), m_fWeightEmpty(0)
{
    m_iMaxUses = 1;
    use_cam_effector = nullptr;
    anim_sect = nullptr;
    m_bHasAnimation = false;
    m_bRemoveAfterUse = true;
    m_physic_item = 0;

	m_fEffectorIntensity = 1.0f;
    m_iAnimHandsCnt = 1;
    m_iAnimLength = 0;
    m_bActivated = false;
    m_bItmStartAnim = false;
}

CEatableItem::~CEatableItem() {}

IFactoryObject* CEatableItem::_construct()
{
    m_physic_item = smart_cast<CPhysicItem*>(this);
    return (inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
    inherited::Load(section);

    m_iMaxUses = READ_IF_EXISTS(pSettings, r_u8, section, "max_uses", 1);
    if (m_iMaxUses < 1)
        m_iMaxUses = 1;

    m_bRemoveAfterUse = READ_IF_EXISTS(pSettings, r_bool, section, "remove_after_use", true);
    m_fWeightFull = m_weight;
    m_fWeightEmpty = READ_IF_EXISTS(pSettings, r_float, section, "empty_weight", 0.0f);

	m_bHasAnimation = READ_IF_EXISTS(pSettings, r_bool, section, "has_anim", false);
    anim_sect = READ_IF_EXISTS(pSettings, r_string, section, "hud_section", nullptr);
    m_fEffectorIntensity = READ_IF_EXISTS(pSettings, r_float, section, "cam_effector_intensity", 1.0f);
    use_cam_effector = READ_IF_EXISTS(pSettings, r_string, section, "use_cam_effector", nullptr);
}

void CEatableItem::load(IReader& packet)
{
    inherited::load(packet);

    if (g_block_all_except_movement)
		g_block_all_except_movement = false;

	if (!g_actor_allow_ladder)
		g_actor_allow_ladder = true;
}

void CEatableItem::save(NET_Packet& packet)
{
    inherited::save(packet);
}

BOOL CEatableItem::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return FALSE;

    return TRUE;
};

void CEatableItem::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    //P.w_float_q8(GetCondition(), 0.0f, 1.0f);
}

void CEatableItem::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
    /*float _cond;
    P.r_float_q8(_cond, 0.0f, 1.0f);
    SetCondition(_cond);*/
}

bool CEatableItem::Useful() const
{
    if (!inherited::Useful())
        return false;

    //проверить не все ли еще съедено
    if (GetRemainingUses() == 0 && CanDelete())
        return false;

    return true;
}

void CEatableItem::OnH_A_Independent()
{
    inherited::OnH_A_Independent();
    if (!Useful())
    {
        if (object().Local() && OnServer())
            object().DestroyObject();
    }
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
    if (!Useful())
    {
        object().setVisible(FALSE);
        object().setEnabled(FALSE);
        if (m_physic_item)
            m_physic_item->m_ready_to_destroy = true;
    }
    inherited::OnH_B_Independent(just_before_destroy);
}

void CEatableItem::UpdateInRuck(void) 
{ 
    UpdateUseAnim(); 
}

void CEatableItem::HideWeapon()
{
    CEffectorCam* effector = Actor()->Cameras().GetCamEffector((ECamEffectorType)effUseItem);
    CCustomDetector* pDet = smart_cast<CCustomDetector*>(Actor()->inventory().ItemFromSlot(DETECTOR_SLOT));
    CFlashlight* pFlight = smart_cast<CFlashlight*>(Actor()->inventory().ItemFromSlot(DETECTOR_SLOT));

    Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);

	if (pDet)
        pDet->HideDetector(true);
    if (pFlight)
        pFlight->HideDevice(true);

    m_bItmStartAnim = true;

    if (!Actor()->inventory_disabled())
        CurrentGameUI()->HideActorMenu();
}

void CEatableItem::StartAnimation()
{
    m_bActivated = true;

    CEffectorCam* effector = Actor()->Cameras().GetCamEffector((ECamEffectorType)effUseItem);

    if (pSettings->line_exist(anim_sect, "single_handed_anim"))
        m_iAnimHandsCnt = pSettings->r_u32(anim_sect, "single_handed_anim");

    m_bItmStartAnim = false;
    g_actor_allow_ladder = false;
    Actor()->m_bActionAnimInProcess = true;

    if (!strstr(Core.Params, "-dev") || !strstr(Core.Params, "-dbg"))
        g_block_all_except_movement = true;

    CCustomOutfit* cur_outfit = Actor()->GetOutfit();

    if (pSettings->line_exist(anim_sect, "anm_use"))
    {
		string128 anim_name{};
		strconcat(sizeof(anim_name), anim_name, "anm_use", (cur_outfit && cur_outfit->m_bHasLSS) ? "_exo" : (m_iMaxUses == 1) ? "_last" : "");

		if (pSettings->line_exist(anim_sect, anim_name))
		{
			g_player_hud->script_anim_play(m_iAnimHandsCnt, anim_sect, anim_name, false, 1.0f);
			m_iAnimLength = Device.dwTimeGlobal + g_player_hud->motion_length_script(anim_sect, anim_name, 1.0f);
		}
		else
		{
			g_player_hud->script_anim_play(m_iAnimHandsCnt, anim_sect, "anm_use", false, 1.0f);
			m_iAnimLength = Device.dwTimeGlobal + g_player_hud->motion_length_script(anim_sect, "anm_use", 1.0f);
		}

        if (psActorFlags.test(AF_SSFX_DOF))
        {
            ps_ssfx_wpn_dof_1 = GameConstants::GetSSFX_FocusDoF();
            ps_ssfx_wpn_dof_2 = GameConstants::GetSSFX_FocusDoF().z;
        }
    }

    if (!effector && use_cam_effector != nullptr)
        AddEffector(Actor(), effUseItem, use_cam_effector, m_fEffectorIntensity);

    if (pSettings->line_exist(anim_sect, "snd_using"))
    {
        if (m_using_sound._feedback())
            m_using_sound.stop();

		string128 snd_var_name{};
		shared_str snd_name{};

		strconcat(sizeof(snd_var_name), snd_var_name, "snd_using", (cur_outfit && cur_outfit->m_bHasLSS) ? "_exo" : (m_iMaxUses == 1) ? "_last" : "");

		if (pSettings->line_exist(anim_sect, snd_var_name))
			snd_name = pSettings->r_string(anim_sect, snd_var_name);
		else
			snd_name = pSettings->r_string(anim_sect, "snd_using");

        m_using_sound.create(snd_name.c_str(), st_Effect, sg_SourceType);
        m_using_sound.play(NULL, sm_2D);
    }
}

void CEatableItem::UpdateUseAnim()
{
    if (!psActorFlags.test(AF_ITEM_ANIMATIONS_ENABLE) || !m_bHasAnimation)
        return;

    CCustomDetector* pDet = smart_cast<CCustomDetector*>(Actor()->inventory().ItemFromSlot(DETECTOR_SLOT));
    CFlashlight* pFlight = smart_cast<CFlashlight*>(Actor()->inventory().ItemFromSlot(DETECTOR_SLOT));
    CEffectorCam* effector = Actor()->Cameras().GetCamEffector((ECamEffectorType)effUseItem);

    bool IsActorAlive = g_pGamePersistent->GetActorAliveStatus();

    if (m_bItmStartAnim && Actor()->inventory().GetActiveSlot() == NO_ACTIVE_SLOT) // Тут вылетало
        if (!pDet && !pFlight || pDet && pDet->IsHidden() || pFlight && pFlight->IsHidden())
            StartAnimation();

	if (!IsActorAlive)
        m_using_sound.stop();

    if (m_bActivated)
    {
        if (m_iAnimLength <= Device.dwTimeGlobal || !IsActorAlive)
        {
            Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, false);

            m_iAnimLength = Device.dwTimeGlobal;
            m_bActivated = false;
            g_block_all_except_movement = false;
            g_actor_allow_ladder = true;
            Actor()->m_bActionAnimInProcess = false;

            if (effector)
                RemoveEffector(Actor(), effUseItem);

            if (psActorFlags.test(AF_SSFX_DOF))
            {
                ps_ssfx_wpn_dof_1 = GameConstants::GetSSFX_FocusDoF();
                ps_ssfx_wpn_dof_2 = GameConstants::GetSSFX_FocusDoF().z;
            }

			if (IsActorAlive)
                Actor()->inventory().Eat(this);
        }
    }
}

bool CEatableItem::UseBy(CEntityAlive* entity_alive)
{
    SMedicineInfluenceValues V;
    V.Load(m_physic_item->cNameSect());

    CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
    R_ASSERT(IO);
    R_ASSERT(m_pInventory == IO->m_inventory);
    R_ASSERT(object().H_Parent()->ID() == entity_alive->ID());

    entity_alive->conditions().ApplyInfluence(V, m_physic_item->cNameSect());

    for (u8 i = 0; i < (u8)eBoostMaxCount; i++)
    {
        if (pSettings->line_exist(m_physic_item->cNameSect().c_str(), ef_boosters_section_names[i]))
        {
            SBooster B;
            B.Load(m_physic_item->cNameSect(), (EBoostParams)i);
            entity_alive->conditions().ApplyBooster(B, m_physic_item->cNameSect());
        }
    }

    if (!IsGameTypeSingle() && OnServer())
    {
        NET_Packet tmp_packet;
        CGameObject::u_EventGen(tmp_packet, GEG_PLAYER_USE_BOOSTER, entity_alive->ID());
        tmp_packet.w_u16(object_id());
        Level().Send(tmp_packet);
    }

    return true;
}

float CEatableItem::Weight() const
{
    float res = inherited::Weight();

    if (IsUsingCondition())
    {
        const float net_weight = m_fWeightFull - m_fWeightEmpty;
        const float use_weight = m_iMaxUses > 0 ? net_weight / m_iMaxUses : 0.f;

        res = m_fWeightEmpty + GetRemainingUses() * use_weight;
    }

    return res;
}
