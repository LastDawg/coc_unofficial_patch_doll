#include "stdafx.h"
#include "ui_af_params.h"
#include "xrUICore/Static/UIStatic.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "Common/object_broker.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "string_table.h"
#include "Inventory_Item.h"
#include "Artefact.h"

#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "ActorBackpack.h"
#include "ActorUnvest.h"
#include "CustomDetector.h"

u32 const red_clr = color_argb(255, 210, 50, 50);
u32 const green_clr = color_argb(255, 170, 170, 170);

CUIArtefactParams::CUIArtefactParams()
{
    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        m_immunity_item[i] = nullptr;
    }
    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        m_restore_item[i] = nullptr;
    }
    m_additional_weight = nullptr;
	m_disp_condition = nullptr;
	m_fJumpSpeed = nullptr;
	m_fWalkAccel = nullptr;
    m_fThirstRestoreSpeed = nullptr;
    m_fIntoxicationRestoreSpeed = nullptr;
    m_fPsyHealthRestoreSpeed = nullptr;
    m_fSleepenessRestoreSpeed = nullptr;
    m_artefact_count = nullptr;
    m_fAfVisRadius = nullptr;
    m_fRepairRestoreSpeed = nullptr;
    m_fDevChargeRestoreSpeed = nullptr;
	m_Prop_line = nullptr;
}

CUIArtefactParams::~CUIArtefactParams()
{
	delete_data	(m_immunity_item);
	delete_data	(m_restore_item);
	xr_delete	(m_additional_weight );
	xr_delete   (m_disp_condition);
	xr_delete   (m_fJumpSpeed);
	xr_delete   (m_fWalkAccel);
    xr_delete   (m_fThirstRestoreSpeed);
    xr_delete   (m_fIntoxicationRestoreSpeed);
    xr_delete   (m_fPsyHealthRestoreSpeed);
    xr_delete   (m_fSleepenessRestoreSpeed);
    xr_delete   (m_artefact_count);
    xr_delete   (m_fAfVisRadius);
    xr_delete   (m_fRepairRestoreSpeed);
    xr_delete   (m_fDevChargeRestoreSpeed);
	xr_delete	(m_Prop_line);
}

constexpr pcstr af_immunity_section_names[] = // ALife::EInfluenceType
{
    "radiation_immunity", // infl_rad=0
    "burn_immunity", // infl_fire=1
    "chemical_burn_immunity", // infl_acid=2
    "telepatic_immunity", // infl_psi=3
    "shock_immunity", // infl_electra=4
    "wound_immunity",
    "fire_wound_immunity",
    "explosion_immunity",
    "strike_immunity",
};

constexpr pcstr af_restore_section_names[] = // ALife::EConditionRestoreType
{
        "health_restore_speed", // eHealthRestoreSpeed=0
        "satiety_restore_speed", // eSatietyRestoreSpeed=1
        "power_restore_speed", // ePowerRestoreSpeed=2
        "bleeding_restore_speed", // eBleedingRestoreSpeed=3
        "radiation_restore_speed", // eRadiationRestoreSpeed=4
        "thirst_restore_speed", // eThirstRestoreSpeed=5
        "intoxication_restore_speed", // eIntoxicationRestoreSpeed=6
        "sleepeness_restore_speed", // eSleepenessRestoreSpeed=7
        "psy_health_restore_speed", // eHealthRestoreSpeed=8
};

constexpr pcstr af_immunity_caption[] = // ALife::EInfluenceType
{
    "ui_inv_outfit_radiation_protection", // "(radiation_imm)",
    "ui_inv_outfit_burn_protection", // "(burn_imm)",
    "ui_inv_outfit_chemical_burn_protection", // "(chemical_burn_imm)",
    "ui_inv_outfit_telepatic_protection", // "(telepatic_imm)",
    "ui_inv_outfit_shock_protection", // "(shock_imm)",
    "ui_inv_outfit_wound_protection", // "(wound_imm)",
    "ui_inv_outfit_fire_wound_protection", // "(fire_wound_imm)",
    "ui_inv_outfit_explosion_protection", // "(explosion_imm)",
    "ui_inv_outfit_strike_protection",	 // "(strike_imm)",
};

LPCSTR af_restore_caption[] = // ALife::EConditionRestoreType
{
        "ui_inv_health",
        "ui_inv_satiety",
        "ui_inv_power",
        "ui_inv_bleeding",
        "ui_inv_radiation",
        "ui_inv_thirst",
        "ui_inv_intoxication",
        "ui_inv_sleepeness",
};

/*
LPCSTR af_actor_param_names[]=
{
    "satiety_health_v",
    "radiation_v",
    "satiety_v",
    "satiety_power_v",
    "wound_incarnation_v",
};
*/

void CUIArtefactParams::InitFromXml(CUIXml& xml)
{
    LPCSTR base = "af_params";

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(base, 0);
    if (!base_node) { return; }
    CUIXmlInit::InitWindow(xml, base, 0, this);
    xml.SetLocalRoot(base_node);

    m_Prop_line = new CUIStatic();
    AttachChild(m_Prop_line);
    m_Prop_line->SetAutoDelete(false);
    CUIXmlInit::InitStatic(xml, "prop_line", 0, m_Prop_line);

    //Alundaio: Show AF Condition
    m_disp_condition = new UIArtefactParamItem();
    m_disp_condition->Init(xml, "condition");
    m_disp_condition->SetAutoDelete(false);
    pcstr name = StringTable().translate("ui_inv_af_condition").c_str();
    m_disp_condition->SetCaption(name);
    xml.SetLocalRoot(base_node);
    //-Alundaio

    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        m_immunity_item[i] = new UIArtefactParamItem();
        m_immunity_item[i]->Init(xml, af_immunity_section_names[i]);
        m_immunity_item[i]->SetAutoDelete(false);

        LPCSTR name = StringTable().translate(af_immunity_caption[i]).c_str();
        m_immunity_item[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        m_restore_item[i] = new UIArtefactParamItem();
        m_restore_item[i]->Init(xml, af_restore_section_names[i]);
        m_restore_item[i]->SetAutoDelete(false);

        LPCSTR name = StringTable().translate(af_restore_caption[i]).c_str();
        m_restore_item[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    // Alundaio: AF movement modifiers
    {
        m_fJumpSpeed = new UIArtefactParamItem();
        m_fJumpSpeed->Init(xml, "jump_speed");
        m_fJumpSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_af_jump_speed").c_str();
        m_fJumpSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }
    {
        m_fWalkAccel = new UIArtefactParamItem();
        m_fWalkAccel->Init(xml, "walk_accel");
        m_fWalkAccel->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_af_walk_accel").c_str();
        m_fWalkAccel->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fThirstRestoreSpeed = new UIArtefactParamItem();
        m_fThirstRestoreSpeed->Init(xml, "thirst_restore_speed");
        m_fThirstRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_thirst").c_str();
        m_fThirstRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fIntoxicationRestoreSpeed = new UIArtefactParamItem();
        m_fIntoxicationRestoreSpeed->Init(xml, "intoxication_restore_speed");
        m_fIntoxicationRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_intoxication").c_str();
        m_fIntoxicationRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fSleepenessRestoreSpeed = new UIArtefactParamItem();
        m_fSleepenessRestoreSpeed->Init(xml, "sleepeness_restore_speed");
        m_fSleepenessRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_sleepeness").c_str();
        m_fSleepenessRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fPsyHealthRestoreSpeed = new UIArtefactParamItem();
        m_fPsyHealthRestoreSpeed->Init(xml, "psy_health_restore_speed");
        m_fPsyHealthRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_psy_health_restore").c_str();
        m_fPsyHealthRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_artefact_count = new UIArtefactParamItem();
        m_artefact_count->Init(xml, "artefact_count");
        m_artefact_count->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_af_artefact_count").c_str();
        m_artefact_count->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fAfVisRadius = new UIArtefactParamItem();
        m_fAfVisRadius->Init(xml, "det_af_vis_radius");
        m_fAfVisRadius->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_det_af_vis_radius").c_str();
        m_fAfVisRadius->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fRepairRestoreSpeed = new UIArtefactParamItem();
        m_fRepairRestoreSpeed->Init(xml, "repair_restore_speed");
        m_fRepairRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_repair_speed").c_str();
        m_fRepairRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fDevChargeRestoreSpeed = new UIArtefactParamItem();
        m_fDevChargeRestoreSpeed->Init(xml, "device_charge_restore_speed");
        m_fDevChargeRestoreSpeed->SetAutoDelete(false);
        LPCSTR name = StringTable().translate("ui_inv_charge_speed").c_str();
        m_fDevChargeRestoreSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_additional_weight = new UIArtefactParamItem();
        m_additional_weight->Init(xml, "additional_weight");
        m_additional_weight->SetAutoDelete(false);

        LPCSTR name = StringTable().translate("ui_inv_weight").c_str();
        m_additional_weight->SetCaption(name);

        // xml.SetLocalRoot( base_node );
    }

    xml.SetLocalRoot(stored_root);
}

bool CUIArtefactParams::Check(const shared_str& af_section)
{
    return !!pSettings->line_exist(af_section, "af_actor_properties");
}

bool CUIArtefactParams::CheckDescrInfoPortions(const shared_str& af_section)
{
    if (pSettings->line_exist(af_section, "af_description_infoportion"))
    {
        shared_str ArtifactDescrInfo = pSettings->r_string(af_section, "af_description_infoportion");
        if (Actor()->HasInfo(ArtifactDescrInfo))
            return true;
        else
            return false;
    }
    else
        return true;
}

void CUIArtefactParams::SetInfo(const CArtefact* pInvItem)
{
    DetachAll();
    AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
        return;

    const shared_str& af_section = pInvItem->cNameSect();

    float val = 0.0f, max_val = 1.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	m_disp_condition->SetValue(pInvItem->GetCondition());
    pos.set(m_disp_condition->GetWndPos());
    pos.y = h;
    m_disp_condition->SetWndPos(pos);
    h += m_disp_condition->GetWndSize().y;
    AttachChild(m_disp_condition);

    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        shared_str const& sect = pSettings->r_string(af_section, "hit_absorbation_sect");
        val = READ_IF_EXISTS(pSettings, r_float, sect, af_immunity_section_names[i], 0.f);
        if (fis_zero(val))
            continue;

        val *= pInvItem->GetCondition();
        max_val = actor->conditions().GetZoneMaxPower((ALife::EInfluenceType)i);
        val /= max_val;
        m_immunity_item[i]->SetValue(val);

        pos.set(m_immunity_item[i]->GetWndPos());
        pos.y = h;
        m_immunity_item[i]->SetWndPos(pos);

        h += m_immunity_item[i]->GetWndSize().y;
        AttachChild(m_immunity_item[i]);
    }

	{
		float val = pInvItem->m_fJumpSpeed;
        if (!fis_zero(val))
		{
			m_fJumpSpeed->SetValue(val*pInvItem->GetCondition());
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
        if (!fis_zero(val))
		{
			m_fWalkAccel->SetValue(val*pInvItem->GetCondition());
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
	}

    {
        val = pSettings->r_float(af_section, "additional_inventory_weight");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_additional_weight->SetValue(val);

            pos.set(m_additional_weight->GetWndPos());
            pos.y = h;
            m_additional_weight->SetWndPos(pos);

            h += m_additional_weight->GetWndSize().y;
            AttachChild(m_additional_weight);
        }

        val = pSettings->r_float(af_section, "thirst_restore_speed");
        if (!fis_zero(val))
        {
            m_fThirstRestoreSpeed->SetValue(val);

            pos.set(m_fThirstRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fThirstRestoreSpeed->SetWndPos(pos);

            h += m_fThirstRestoreSpeed->GetWndSize().y;
            AttachChild(m_fThirstRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "intoxication_restore_speed");
        if (!fis_zero(val))
        {
            m_fIntoxicationRestoreSpeed->SetValue(val);

            pos.set(m_fIntoxicationRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fIntoxicationRestoreSpeed->SetWndPos(pos);

            h += m_fIntoxicationRestoreSpeed->GetWndSize().y;
            AttachChild(m_fIntoxicationRestoreSpeed);
        }

		val = pSettings->r_float(af_section, "sleepeness_restore_speed");
        if (!fis_zero(val))
        {
            m_fSleepenessRestoreSpeed->SetValue(val);

            pos.set(m_fSleepenessRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fSleepenessRestoreSpeed->SetWndPos(pos);

            h += m_fSleepenessRestoreSpeed->GetWndSize().y;
            AttachChild(m_fSleepenessRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "psy_health_restore_speed");
        if (!fis_zero(val))
        {
            m_fPsyHealthRestoreSpeed->SetValue(val);

            pos.set(m_fPsyHealthRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fPsyHealthRestoreSpeed->SetWndPos(pos);

            h += m_fPsyHealthRestoreSpeed->GetWndSize().y;
            AttachChild(m_fPsyHealthRestoreSpeed);
        }
    }

    val = pSettings->r_float(af_section, "repair_restore_speed");
    if (!fis_zero(val))
    {
        m_fRepairRestoreSpeed->SetValue(val);

        pos.set(m_fRepairRestoreSpeed->GetWndPos());
        pos.y = h;
        m_fRepairRestoreSpeed->SetWndPos(pos);

        h += m_fRepairRestoreSpeed->GetWndSize().y;
        AttachChild(m_fRepairRestoreSpeed);
    }

	val = pSettings->r_float(af_section, "device_charge_restore_speed");
    if (!fis_zero(val))
    {
        m_fDevChargeRestoreSpeed->SetValue(val);

        pos.set(m_fDevChargeRestoreSpeed->GetWndPos());
        pos.y = h;
        m_fDevChargeRestoreSpeed->SetWndPos(pos);

        h += m_fDevChargeRestoreSpeed->GetWndSize().y;
        AttachChild(m_fDevChargeRestoreSpeed);
    }

    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        val = pSettings->r_float(af_section, af_restore_section_names[i]);
        if (fis_zero(val))
            continue;

        val *= pInvItem->GetCondition();
        m_restore_item[i]->SetValue(val);

        pos.set(m_restore_item[i]->GetWndPos());
        pos.y = h;
        m_restore_item[i]->SetWndPos(pos);

        h += m_restore_item[i]->GetWndSize().y;
        AttachChild(m_restore_item[i]);
    }

    SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CCustomOutfit* pInvItem)
{
	DetachAll();
	//AttachChild(m_Prop_line);

	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
	{
		return;
	}

	const shared_str& af_section = pInvItem->cNameSect();

	float val = 0.0f;
	Fvector2 pos;
	float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	{
		float val = pInvItem->m_fJumpSpeed;
        if (!fis_zero(val))
		{
			m_fJumpSpeed->SetValue(val);
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
        if (!fis_zero(val))
		{
			m_fWalkAccel->SetValue(val);
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
	}

	{
		val = pSettings->r_float(af_section, "additional_inventory_weight");
		if (!fis_zero(val))
		{
			//val *= pInvItem->GetCondition();
			m_additional_weight->SetValue(val);

			pos.set(m_additional_weight->GetWndPos());
			pos.y = h;
			m_additional_weight->SetWndPos(pos);

			h += m_additional_weight->GetWndSize().y;
			AttachChild(m_additional_weight);
		}

        val = pSettings->r_float(af_section, "thirst_restore_speed");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_fThirstRestoreSpeed->SetValue(val);

            pos.set(m_fThirstRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fThirstRestoreSpeed->SetWndPos(pos);

            h += m_fThirstRestoreSpeed->GetWndSize().y;
            AttachChild(m_fThirstRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "intoxication_restore_speed");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_fIntoxicationRestoreSpeed->SetValue(val);

            pos.set(m_fIntoxicationRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fIntoxicationRestoreSpeed->SetWndPos(pos);

            h += m_fIntoxicationRestoreSpeed->GetWndSize().y;
            AttachChild(m_fIntoxicationRestoreSpeed);
        }

		val = pSettings->r_float(af_section, "sleepeness_restore_speed");
        if (!fis_zero(val))
        {
            m_fSleepenessRestoreSpeed->SetValue(val);

            pos.set(m_fSleepenessRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fSleepenessRestoreSpeed->SetWndPos(pos);

            h += m_fSleepenessRestoreSpeed->GetWndSize().y;
            AttachChild(m_fSleepenessRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "psy_health_restore_speed");
        if (!fis_zero(val))
        {
            m_fPsyHealthRestoreSpeed->SetValue(val);

            pos.set(m_fPsyHealthRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fPsyHealthRestoreSpeed->SetWndPos(pos);

            h += m_fPsyHealthRestoreSpeed->GetWndSize().y;
            AttachChild(m_fPsyHealthRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "artefact_count");
        if (!fis_zero(val))
        {
            m_artefact_count->SetValue(val);

            pos.set(m_artefact_count->GetWndPos());
            pos.y = h;
            m_artefact_count->SetWndPos(pos);

            h += m_artefact_count->GetWndSize().y;
            AttachChild(m_artefact_count);
        }
    }

	SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CHelmet* pInvItem) 
{ 
    DetachAll(); 

	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    const shared_str& af_section = pInvItem->cNameSect();

    float val = 0.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    {
        val = pSettings->r_float(af_section, "thirst_restore_speed");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_fThirstRestoreSpeed->SetValue(val);

            pos.set(m_fThirstRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fThirstRestoreSpeed->SetWndPos(pos);

            h += m_fThirstRestoreSpeed->GetWndSize().y;
            AttachChild(m_fThirstRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "intoxication_restore_speed");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_fIntoxicationRestoreSpeed->SetValue(val);

            pos.set(m_fIntoxicationRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fIntoxicationRestoreSpeed->SetWndPos(pos);

            h += m_fIntoxicationRestoreSpeed->GetWndSize().y;
            AttachChild(m_fIntoxicationRestoreSpeed);
        }

		val = pSettings->r_float(af_section, "sleepeness_restore_speed");
        if (!fis_zero(val))
        {
            m_fSleepenessRestoreSpeed->SetValue(val);

            pos.set(m_fSleepenessRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fSleepenessRestoreSpeed->SetWndPos(pos);

            h += m_fSleepenessRestoreSpeed->GetWndSize().y;
            AttachChild(m_fSleepenessRestoreSpeed);
        }

        val = pSettings->r_float(af_section, "psy_health_restore_speed");
        if (!fis_zero(val))
        {
            m_fPsyHealthRestoreSpeed->SetValue(val);

            pos.set(m_fPsyHealthRestoreSpeed->GetWndPos());
            pos.y = h;
            m_fPsyHealthRestoreSpeed->SetWndPos(pos);

            h += m_fPsyHealthRestoreSpeed->GetWndSize().y;
            AttachChild(m_fPsyHealthRestoreSpeed);
        }
    }

}

void CUIArtefactParams::SetInfo(const CBackpack* pInvItem)
{
	DetachAll();
	AttachChild(m_Prop_line);

	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
	{
		return;
	}

	const shared_str& af_section = pInvItem->cNameSect();

	float val = 0.0f, max_val = 1.0f;
	Fvector2 pos;
	float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	{
		float val = pInvItem->m_fJumpSpeed;
        if (!fis_zero(val))
		{
			m_fJumpSpeed->SetValue(val);
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
        if (!fis_zero(val))
		{
			m_fWalkAccel->SetValue(val);
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
	}

	{
		val = pSettings->r_float(af_section, "additional_inventory_weight");
		if (!fis_zero(val))
		{
			//val *= pInvItem->GetCondition();
			m_additional_weight->SetValue(val);

			pos.set(m_additional_weight->GetWndPos());
			pos.y = h;
			m_additional_weight->SetWndPos(pos);

			h += m_additional_weight->GetWndSize().y;
			AttachChild(m_additional_weight);
		}
	}

	SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CUnvest* pInvItem)
{
    DetachAll();
    AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    const shared_str& af_section = pInvItem->cNameSect();

    float val = 0.0f, max_val = 1.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    {
        float val = pInvItem->m_fJumpSpeed;
        if (!fis_zero(val))
        {
            m_fJumpSpeed->SetValue(val);
            pos.set(m_fJumpSpeed->GetWndPos());
            pos.y = h;
            m_fJumpSpeed->SetWndPos(pos);
            h += m_fJumpSpeed->GetWndSize().y;
            AttachChild(m_fJumpSpeed);
        }
        val = pInvItem->m_fWalkAccel;
        if (!fis_zero(val))
        {
            m_fWalkAccel->SetValue(val);
            pos.set(m_fWalkAccel->GetWndPos());
            pos.y = h;
            m_fWalkAccel->SetWndPos(pos);
            h += m_fWalkAccel->GetWndSize().y;
            AttachChild(m_fWalkAccel);
        }
    }

    {
        val = pSettings->r_float(af_section, "additional_inventory_weight");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_additional_weight->SetValue(val);

            pos.set(m_additional_weight->GetWndPos());
            pos.y = h;
            m_additional_weight->SetWndPos(pos);

            h += m_additional_weight->GetWndSize().y;
            AttachChild(m_additional_weight);
        }

        val = pSettings->r_float(af_section, "artefact_count");
        if (!fis_zero(val))
        {
            m_artefact_count->SetValue(val);

            pos.set(m_artefact_count->GetWndPos());
            pos.y = h;
            m_artefact_count->SetWndPos(pos);

            h += m_artefact_count->GetWndSize().y;
            AttachChild(m_artefact_count);
        }
    }

    SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CCustomDetector* pInvItem)
{
    DetachAll();
    AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    const shared_str& af_section = pInvItem->cNameSect();

    float val = 0.0f, max_val = 1.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    {
        float val = pInvItem->m_fAfVisRadius;
        if (!fis_zero(val))
        {
            m_fAfVisRadius->SetValue(val);
            pos.set(m_fAfVisRadius->GetWndPos());
            pos.y = h;
            m_fAfVisRadius->SetWndPos(pos);
            h += m_fAfVisRadius->GetWndSize().y;
            AttachChild(m_fAfVisRadius);
        }
    }

    SetHeight(h);
}

/// ----------------------------------------------------------------

UIArtefactParamItem::UIArtefactParamItem()
{
    m_caption = nullptr;
    m_value = nullptr;
    m_magnitude = 1.0f;
    m_sign_inverse = false;
    m_show_sign = false;
    m_accuracy = 0;

    m_unit_str._set("");
    m_texture_minus._set("");
    m_texture_plus._set("");
}

UIArtefactParamItem::~UIArtefactParamItem() {}
void UIArtefactParamItem::Init(CUIXml& xml, LPCSTR section)
{
    CUIXmlInit::InitWindow(xml, section, 0, this);
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this);
    m_value = UIHelper::CreateTextWnd(xml, "value", this);
    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
    m_sign_inverse = (xml.ReadAttribInt("value", 0, "sign_inverse", 0) == 1);
    m_show_sign = (xml.ReadAttribInt("value", 0, "show_sign", 1) == 1);
    m_accuracy = xml.ReadAttribInt("value", 0, "accuracy", 0);

    LPCSTR unit_str = xml.ReadAttrib("value", 0, "unit_str", "");
    m_unit_str._set(StringTable().translate(unit_str));

    LPCSTR texture_minus = xml.Read("texture_minus", 0, "");
    if (texture_minus && xr_strlen(texture_minus))
    {
        m_texture_minus._set(texture_minus);

        LPCSTR texture_plus = xml.Read("caption:texture", 0, "");
        m_texture_plus._set(texture_plus);
        VERIFY(m_texture_plus.size());
    }
}

void UIArtefactParamItem::SetCaption(LPCSTR name) { m_caption->TextItemControl()->SetText(name); }
void UIArtefactParamItem::SetValue(float value)
{
    value *= m_magnitude;
    string32 buf;

    if (m_show_sign)
	    if (m_accuracy == 0)
            xr_sprintf(buf, "%+.0f", value);
        else if (m_accuracy == 1)
            xr_sprintf(buf, "%+.1f", value);
        else if (m_accuracy == 2)
            xr_sprintf(buf, "%+.2f", value);
        else if (m_accuracy >= 3)
            xr_sprintf(buf, "%+.3f", value);
    else 
	    if (m_accuracy == 0)
            xr_sprintf(buf, "%.0f", value);
        else if (m_accuracy == 1)
            xr_sprintf(buf, "%.1f", value);
        else if (m_accuracy == 2)
            xr_sprintf(buf, "%.2f", value);
        else if (m_accuracy >= 3)
            xr_sprintf(buf, "%.3f", value);

    LPSTR str;
    if (m_unit_str.size())
    {
        STRCONCAT(str, buf, " ", m_unit_str.c_str());
    }
    else // = ""
    {
        STRCONCAT(str, buf);
    }
    m_value->SetText(str);

    bool positive = (value >= 0.0f);
    positive = (m_sign_inverse) ? !positive : positive;
    u32 color = (positive) ? green_clr : red_clr;
    m_value->SetTextColor(color);

    if (m_texture_minus.size())
    {
        if (positive)
        {
            m_caption->InitTexture(m_texture_plus.c_str());
        }
        else
        {
            m_caption->InitTexture(m_texture_minus.c_str());
        }
    }
}
