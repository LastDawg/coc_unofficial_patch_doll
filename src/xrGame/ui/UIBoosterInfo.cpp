#include "stdafx.h"
#include "UIBoosterInfo.h"
#include "xrUICore/Static/UIStatic.h"
#include "Common/object_broker.h"
#include "EntityCondition.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "string_table.h"

u32 const red_clr = color_argb(255, 210, 50, 50);
u32 const green_clr = color_argb(255, 170, 170, 170);

CUIBoosterInfo::CUIBoosterInfo()
{
    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        m_booster_items[i] = NULL;
    }
    m_booster_satiety = NULL;
    m_booster_thirst = NULL;
    m_booster_health = NULL;
    m_booster_power = NULL;
    m_booster_radiation = NULL;
    m_booster_alcohol = NULL;
    m_booster_intoxication = NULL;
    m_booster_sleepeness = NULL;
    m_booster_anabiotic = NULL;
    m_booster_time = NULL;
}

CUIBoosterInfo::~CUIBoosterInfo()
{
    delete_data(m_booster_items);
    xr_delete(m_booster_satiety);
    xr_delete(m_booster_thirst);
    xr_delete(m_booster_health);
    xr_delete(m_booster_power);
    xr_delete(m_booster_radiation);
    xr_delete(m_booster_alcohol);
    xr_delete(m_booster_intoxication);
    xr_delete(m_booster_sleepeness);
    xr_delete(m_booster_anabiotic);
    xr_delete(m_booster_time);
    xr_delete(m_Prop_line);
}

LPCSTR boost_influence_caption[] = 
{
    "ui_inv_health", 
    "ui_inv_power", 
    "ui_inv_radiation", 
    "ui_inv_bleeding",
    "ui_inv_outfit_additional_weight", 
    "ui_inv_outfit_radiation_protection", 
    "ui_inv_outfit_telepatic_protection",
    "ui_inv_outfit_chemical_burn_protection", 
    "ui_inv_outfit_burn_immunity", 
    "ui_inv_outfit_shock_immunity",
    "ui_inv_outfit_radiation_immunity", 
    "ui_inv_outfit_telepatic_immunity", 
    "ui_inv_outfit_chemical_burn_immunity"
};

void CUIBoosterInfo::InitFromXml(CUIXml& xml)
{
    LPCSTR base = "booster_params";
    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(base, 0);
    if (!base_node)
        return;

    CUIXmlInit::InitWindow(xml, base, 0, this);
    xml.SetLocalRoot(base_node);

    m_Prop_line = new CUIStatic();
    AttachChild(m_Prop_line);
    m_Prop_line->SetAutoDelete(false);
    CUIXmlInit::InitStatic(xml, "prop_line", 0, m_Prop_line);

    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        m_booster_items[i] = new UIBoosterInfoItem();
        m_booster_items[i]->Init(xml, ef_boosters_section_names[i]);
        m_booster_items[i]->SetAutoDelete(false);

        LPCSTR name = StringTable().translate(boost_influence_caption[i]).c_str();
        m_booster_items[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    m_booster_satiety = new UIBoosterInfoItem();
    m_booster_satiety->Init(xml, "boost_satiety");
    m_booster_satiety->SetAutoDelete(false);
    LPCSTR name = StringTable().translate("ui_inv_satiety").c_str();
    m_booster_satiety->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_thirst = new UIBoosterInfoItem();
    m_booster_thirst->Init(xml, "boost_thirst");
    m_booster_thirst->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_thirst").c_str();
    m_booster_thirst->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_health = new UIBoosterInfoItem();
    m_booster_health->Init(xml, "boost_health");
    m_booster_health->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_health_boost").c_str();
    m_booster_health->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_power = new UIBoosterInfoItem();
    m_booster_power->Init(xml, "boost_power");
    m_booster_power->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_power_boost").c_str();
    m_booster_power->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_radiation = new UIBoosterInfoItem();
    m_booster_radiation->Init(xml, "boost_radiation");
    m_booster_radiation->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_radiation").c_str();
    m_booster_radiation->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_alcohol = new UIBoosterInfoItem();
    m_booster_alcohol->Init(xml, "boost_alcohol");
    m_booster_alcohol->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_alcohol").c_str();
    m_booster_alcohol->SetCaption(name);
    xml.SetLocalRoot(base_node);

	// M.F.S. Team Intoxication
    m_booster_intoxication = new UIBoosterInfoItem();
    m_booster_intoxication->Init(xml, "boost_intoxication");
    m_booster_intoxication->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_intoxication").c_str();
    m_booster_intoxication->SetCaption(name);
    xml.SetLocalRoot(base_node);

	// M.F.S. Team Sleepeness
    m_booster_sleepeness = new UIBoosterInfoItem();
    m_booster_sleepeness->Init(xml, "boost_sleepeness");
    m_booster_sleepeness->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_sleepeness").c_str();
    m_booster_sleepeness->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_anabiotic = new UIBoosterInfoItem();
    m_booster_anabiotic->Init(xml, "boost_anabiotic");
    m_booster_anabiotic->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_survive_surge").c_str();
    m_booster_anabiotic->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_time = new UIBoosterInfoItem();
    m_booster_time->Init(xml, "boost_time");
    m_booster_time->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_effect_time").c_str();
    m_booster_time->SetCaption(name);

    xml.SetLocalRoot(stored_root);
}

void CUIBoosterInfo::SetInfo(shared_str const& section)
{
    DetachAll();
    AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

	CEntityCondition::BOOSTER_MAP& boosters = actor->conditions().GetCurBoosterInfluences();

    float val = 0.0f, max_val = 1.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        if (pSettings->line_exist(section.c_str(), ef_boosters_section_names[i]))
        {
            val = pSettings->r_float(section, ef_boosters_section_names[i]);
            if (fis_zero(val))
                continue;

            EBoostParams type = (EBoostParams)i;
            switch (type)
            {
            case eBoostHpRestore:
            case eBoostPowerRestore:
            case eBoostBleedingRestore:
            case eBoostMaxWeight: max_val = 1.0f; break;
            case eBoostRadiationRestore: max_val = -1.0f; break;
            case eBoostBurnImmunity: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_fire); break;
            case eBoostShockImmunity: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_electra); break;
            case eBoostRadiationImmunity:
            case eBoostRadiationProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_rad); break;
            case eBoostTelepaticImmunity:
            case eBoostTelepaticProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_psi); break;
            case eBoostChemicalBurnImmunity:
            case eBoostChemicalBurnProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_acid); break;
            }
            val /= max_val;
            m_booster_items[i]->SetValue(val);

            pos.set(m_booster_items[i]->GetWndPos());
            pos.y = h;
            m_booster_items[i]->SetWndPos(pos);

            h += m_booster_items[i]->GetWndSize().y;
            AttachChild(m_booster_items[i]);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_satiety"))
    {
        val = pSettings->r_float(section, "eat_satiety");
        if (!fis_zero(val))
        {
            m_booster_satiety->SetValue(val);
            pos.set(m_booster_satiety->GetWndPos());
            pos.y = h;
            m_booster_satiety->SetWndPos(pos);

            h += m_booster_satiety->GetWndSize().y;
            AttachChild(m_booster_satiety);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_thirst"))
    {
        val = pSettings->r_float(section, "eat_thirst");
        if (!fis_zero(val))
        {
            m_booster_thirst->SetValue(val);
            pos.set(m_booster_thirst->GetWndPos());
            pos.y = h;
            m_booster_thirst->SetWndPos(pos);

            h += m_booster_thirst->GetWndSize().y;
            AttachChild(m_booster_thirst);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_health"))
    {
        val = pSettings->r_float(section, "eat_health");
        if (!fis_zero(val))
        {
            m_booster_health->SetValue(val);
            pos.set(m_booster_health->GetWndPos());
            pos.y = h;
            m_booster_health->SetWndPos(pos);

            h += m_booster_health->GetWndSize().y;
            AttachChild(m_booster_health);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_power"))
    {
        val = pSettings->r_float(section, "eat_power");
        if (!fis_zero(val))
        {
            m_booster_power->SetValue(val);
            pos.set(m_booster_power->GetWndPos());
            pos.y = h;
            m_booster_power->SetWndPos(pos);

            h += m_booster_power->GetWndSize().y;
            AttachChild(m_booster_power);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_radiation"))
    {
        val = pSettings->r_float(section, "eat_radiation");
        if (!fis_zero(val))
        {
            m_booster_radiation->SetValue(val);
            pos.set(m_booster_radiation->GetWndPos());
            pos.y = h;
            m_booster_radiation->SetWndPos(pos);

            h += m_booster_radiation->GetWndSize().y;
            AttachChild(m_booster_radiation);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_alcohol"))
    {
        val = pSettings->r_float(section, "eat_alcohol");
        if (!fis_zero(val))
        {
            m_booster_alcohol->SetValue(val);
            pos.set(m_booster_alcohol->GetWndPos());
            pos.y = h;
            m_booster_alcohol->SetWndPos(pos);

            h += m_booster_alcohol->GetWndSize().y;
            AttachChild(m_booster_alcohol);
        }
    }

	// M.F.S. Team Intoxication
    if (pSettings->line_exist(section.c_str(), "eat_intoxication"))
    {
        val = pSettings->r_float(section, "eat_intoxication");
        if (!fis_zero(val))
        {
            m_booster_intoxication->SetValue(val);
            pos.set(m_booster_intoxication->GetWndPos());
            pos.y = h;
            m_booster_intoxication->SetWndPos(pos);

            h += m_booster_intoxication->GetWndSize().y;
            AttachChild(m_booster_intoxication);
        }
    }

	// M.F.S. Team Sleepeness
    if (pSettings->line_exist(section.c_str(), "eat_sleepeness"))
    {
        val = pSettings->r_float(section, "eat_sleepeness");
        if (!fis_zero(val))
        {
            m_booster_sleepeness->SetValue(val);
            pos.set(m_booster_sleepeness->GetWndPos());
            pos.y = h;
            m_booster_sleepeness->SetWndPos(pos);

            h += m_booster_sleepeness->GetWndSize().y;
            AttachChild(m_booster_sleepeness);
        }
    }

    if (!xr_strcmp(section.c_str(), "drug_anabiotic"))
    {
        pos.set(m_booster_anabiotic->GetWndPos());
        pos.y = h;
        m_booster_anabiotic->SetWndPos(pos);

        h += m_booster_anabiotic->GetWndSize().y;
        AttachChild(m_booster_anabiotic);
    }

    if (pSettings->line_exist(section.c_str(), "boost_time"))
    {
        val = pSettings->r_float(section, "boost_time");
        if (!fis_zero(val))
        {
            m_booster_time->SetValue(val);
            pos.set(m_booster_time->GetWndPos());
            pos.y = h;
            m_booster_time->SetWndPos(pos);

            h += m_booster_time->GetWndSize().y;
            AttachChild(m_booster_time);
        }
    }
    SetHeight(h);
}

/// ----------------------------------------------------------------

UIBoosterInfoItem::UIBoosterInfoItem()
{
    m_caption = NULL;
    m_value = NULL;
    m_magnitude = 1.0f;
    m_show_sign = false;
    m_sign_inverse = false;
    m_accuracy = 0;

    m_unit_str._set("");
    m_texture_minus._set("");
    m_texture_plus._set("");
}

UIBoosterInfoItem::~UIBoosterInfoItem() {}
void UIBoosterInfoItem::Init(CUIXml& xml, LPCSTR section)
{
    CUIXmlInit::InitWindow(xml, section, 0, this);
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this);
    m_value = UIHelper::CreateTextWnd(xml, "value", this);
    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
    m_show_sign = (xml.ReadAttribInt("value", 0, "show_sign", 1) == 1);
    m_sign_inverse = (xml.ReadAttribInt("value", 0, "sign_inverse", 0) == 1);
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

void UIBoosterInfoItem::SetCaption(LPCSTR name) { m_caption->TextItemControl()->SetText(name); }
void UIBoosterInfoItem::SetValue(float value)
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
        STRCONCAT(str, buf, " ", m_unit_str.c_str());
    else
        STRCONCAT(str, buf);

    m_value->SetText(str);

    bool positive = (value >= 0.0f);
    positive = (m_sign_inverse) ? !positive : positive;
    u32 color = (positive) ? green_clr : red_clr;
    m_value->SetTextColor(color);

    if (m_texture_minus.size())
    {
        if (positive)
            m_caption->InitTexture(m_texture_plus.c_str());
        else
            m_caption->InitTexture(m_texture_minus.c_str());
    }
}
