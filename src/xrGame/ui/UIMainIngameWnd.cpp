#include "stdafx.h"
#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "UIZoneMap.h"
#include <dinput.h>
#include "Actor.h"
#include "ActorCondition.h"
#include "EntityCondition.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "PDA.h"
#include "xrServerEntities/character_info.h"
#include "Inventory.h"
#include "UIGameSP.h"
#include "WeaponMagazined.h"
#include "Missile.h"
#include "Grenade.h"
#include "xrServerEntities/xrServer_objects_ALife.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "game_cl_base.h"
#include "Level.h"
#include "seniority_hierarchy_holder.h"
#include "date_time.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "xrEngine/LightAnimLibrary.h"
#include "UIInventoryUtilities.h"
#include "UIHelper.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "UIPdaWnd.h"
#include "alife_registry_wrappers.h"
#include "string_table.h"
#ifdef DEBUG
#include "attachable_item.h"
#include "xrEngine/xr_input.h"
#endif
#include "xrUICore/ScrollView/UIScrollView.h"
#include "map_hint.h"
#include "game_news.h"
#include "static_cast_checked.hpp"
#include "UIHudStatesWnd.h"
#include "UIActorMenu.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "WeaponKnife.h"

void test_draw();
void test_key(int dik);

#include "Include/xrRender/Kinematics.h"

using namespace InventoryUtilities;
// BOOL		g_old_style_ui_hud			= FALSE;
const u32 g_clWhite = 0xffffffff;

#define DEFAULT_MAP_SCALE 1.f

#define C_SIZE 0.025f
#define NEAR_LIM 0.5f

#define SHOW_INFO_SPEED 0.5f
#define HIDE_INFO_SPEED 10.f
#define C_ON_ENEMY color_xrgb(0xff, 0, 0)
#define C_DEFAULT color_xrgb(0xff, 0xff, 0xff)

#define MAININGAME_XML "maingame.xml"

CUIMainIngameWnd::CUIMainIngameWnd(): /*m_pGrenade(NULL),m_pItem(NULL),*/ m_pPickUpItem(NULL), m_pMPChatWnd(NULL), UIArtefactIcon(NULL), m_pMPLogWnd(NULL)
{
    UIZoneMap = new CUIZoneMap();
}

extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
    DestroyFlashingIcons();
    xr_delete(UIZoneMap);
    HUD_SOUND_ITEM::DestroySound(m_contactSnd);
    xr_delete(g_MissileForceShape);
    xr_delete(UIWeaponJammedIcon);
    xr_delete(UIInvincibleIcon);
    xr_delete(UIArtefactIcon);
}

void CUIMainIngameWnd::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, MAININGAME_XML);

    CUIXmlInit xml_init;
    xml_init.InitWindow(uiXml, "main", 0, this);

    Enable(false);

    //	AttachChild					(&UIStaticHealth);	xml_init.InitStatic			(uiXml, "static_health", 0,
    //&UIStaticHealth);
    //	AttachChild					(&UIStaticArmor);	xml_init.InitStatic			(uiXml, "static_armor", 0,
    //&UIStaticArmor);
    //	AttachChild					(&UIWeaponBack);
    //	xml_init.InitStatic			(uiXml, "static_weapon", 0, &UIWeaponBack);

    /*	UIWeaponBack.AttachChild	(&UIWeaponSignAmmo);
	xml_init.InitStatic			(uiXml, "static_ammo", 0, &UIWeaponSignAmmo);
	UIWeaponSignAmmo.SetEllipsis	(CUIStatic::eepEnd, 2);

	UIWeaponBack.AttachChild	(&UIWeaponIcon);
	xml_init.InitStatic			(uiXml, "static_wpn_icon", 0, &UIWeaponIcon);
	UIWeaponIcon.SetShader		(GetEquipmentIconsShader());
	UIWeaponIcon_rect			= UIWeaponIcon.GetWndRect();
*/ //---------------------------------------------------------
    UIPickUpItemIcon = UIHelper::CreateStatic(uiXml, "pick_up_item", this);
    UIPickUpItemIcon->SetShader(GetEquipmentIconsShader());

    m_iPickUpItemIconWidth = UIPickUpItemIcon->GetWidth();
    m_iPickUpItemIconHeight = UIPickUpItemIcon->GetHeight();
    m_iPickUpItemIconX = UIPickUpItemIcon->GetWndRect().left;
    m_iPickUpItemIconY = UIPickUpItemIcon->GetWndRect().top;
    //---------------------------------------------------------

    //индикаторы
    UIZoneMap->Init();

    // Подсказки, которые возникают при наведении прицела на объект
    UIStaticQuickHelp = UIHelper::CreateTextWnd(uiXml, "quick_info", this);

    uiXml.SetLocalRoot(uiXml.GetRoot());

    m_UIIcons = new CUIScrollView();
    m_UIIcons->SetAutoDelete(true);
    xml_init.InitScrollView(uiXml, "icons_scroll_view", 0, m_UIIcons);
    AttachChild(m_UIIcons);

    m_ind_bleeding = UIHelper::CreateStatic(uiXml, "indicator_bleeding", this);
    m_ind_radiation = UIHelper::CreateStatic(uiXml, "indicator_radiation", this);
    m_ind_psy_health = UIHelper::CreateStatic(uiXml, "indicator_psy_health", this);
    m_ind_starvation = UIHelper::CreateStatic(uiXml, "indicator_starvation", this);
    m_ind_thirst = UIHelper::CreateStatic(uiXml, "indicator_thirst", this);
    m_ind_intoxication = UIHelper::CreateStatic(uiXml, "indicator_intoxication", this);
    m_ind_sleepeness = UIHelper::CreateStatic(uiXml, "indicator_sleepeness", this);
    m_ind_weapon_broken = UIHelper::CreateStatic(uiXml, "indicator_weapon_broken", this);
    m_ind_helmet_broken = UIHelper::CreateStatic(uiXml, "indicator_helmet_broken", this);
    m_ind_outfit_broken = UIHelper::CreateStatic(uiXml, "indicator_outfit_broken", this);
    m_ind_overweight = UIHelper::CreateStatic(uiXml, "indicator_overweight", this);

    m_ind_boost_psy = UIHelper::CreateStatic(uiXml, "indicator_booster_psy", this);
    m_ind_boost_radia = UIHelper::CreateStatic(uiXml, "indicator_booster_radia", this);
    m_ind_boost_chem = UIHelper::CreateStatic(uiXml, "indicator_booster_chem", this);
    m_ind_boost_wound = UIHelper::CreateStatic(uiXml, "indicator_booster_wound", this);
    m_ind_boost_weight = UIHelper::CreateStatic(uiXml, "indicator_booster_weight", this);
    m_ind_boost_health = UIHelper::CreateStatic(uiXml, "indicator_booster_health", this);
    m_ind_boost_power = UIHelper::CreateStatic(uiXml, "indicator_booster_power", this);
    m_ind_boost_rad = UIHelper::CreateStatic(uiXml, "indicator_booster_rad", this);
    m_ind_boost_psy->Show(false);
    m_ind_boost_radia->Show(false);
    m_ind_boost_chem->Show(false);
    m_ind_boost_wound->Show(false);
    m_ind_boost_weight->Show(false);
    m_ind_boost_health->Show(false);
    m_ind_boost_power->Show(false);
    m_ind_boost_rad->Show(false);

    // Загружаем иконки
    /*	if ( IsGameTypeSingle() )
        {
            xml_init.InitStatic		(uiXml, "starvation_static", 0, &UIStarvationIcon);
            UIStarvationIcon.Show	(false);

    //		xml_init.InitStatic		(uiXml, "psy_health_static", 0, &UIPsyHealthIcon);
    //		UIPsyHealthIcon.Show	(false);
        }
    */
    UIWeaponJammedIcon = UIHelper::CreateStatic(uiXml, "weapon_jammed_static", NULL);
    UIWeaponJammedIcon->Show(false);

    //	xml_init.InitStatic			(uiXml, "radiation_static", 0, &UIRadiaitionIcon);
    //	UIRadiaitionIcon.Show		(false);

    //	xml_init.InitStatic			(uiXml, "wound_static", 0, &UIWoundIcon);
    //	UIWoundIcon.Show			(false);

    UIInvincibleIcon = UIHelper::CreateStatic(uiXml, "invincible_static", NULL);
    UIInvincibleIcon->Show(false);

    if ((GameID() == eGameIDArtefactHunt) || (GameID() == eGameIDCaptureTheArtefact))
    {
        UIArtefactIcon = UIHelper::CreateStatic(uiXml, "artefact_static", NULL);
        UIArtefactIcon->Show(false);
    }

    shared_str warningStrings[7] = {"jammed", "radiation", "wounds", "starvation", "fatigue",
        "invincible"
        "artefact"};

    // Загружаем пороговые значения для индикаторов
    EWarningIcons j = ewiWeaponJammed;
    while (j < ewiInvincible)
    {
        // Читаем данные порогов для каждого индикатора
        shared_str cfgRecord =
            pSettings->r_string("main_ingame_indicators_thresholds", *warningStrings[static_cast<int>(j) - 1]);
        u32 count = _GetItemCount(*cfgRecord);

        char singleThreshold[8];
        float f = 0;
        for (u32 k = 0; k < count; ++k)
        {
            _GetItem(*cfgRecord, k, singleThreshold);
            sscanf(singleThreshold, "%f", &f);

            m_Thresholds[j].push_back(f);
        }

        j = static_cast<EWarningIcons>(j + 1);
    }

    // Flashing icons initialize
    uiXml.SetLocalRoot(uiXml.NavigateToNode("flashing_icons"));
    InitFlashingIcons(&uiXml);

    uiXml.SetLocalRoot(uiXml.GetRoot());

    UIMotionIcon = new CUIMotionIcon();
    UIMotionIcon->SetAutoDelete(true);
    UIZoneMap->MapFrame().AttachChild(UIMotionIcon);
    UIMotionIcon->Init(UIZoneMap->MapFrame().GetWndRect());

    UIStaticDiskIO = UIHelper::CreateStatic(uiXml, "disk_io", this);

    m_ui_hud_states = new CUIHudStatesWnd();
    m_ui_hud_states->SetAutoDelete(true);
    AttachChild(m_ui_hud_states);
    m_ui_hud_states->InitFromXml(uiXml, "hud_states");

    for (int i = 0; i < 4; i++)
    {
        m_quick_slots_icons.push_back(new CUIStatic());
        m_quick_slots_icons.back()->SetAutoDelete(true);
        AttachChild(m_quick_slots_icons.back());
        string32 path;
        xr_sprintf(path, "quick_slot%d", i);
        CUIXmlInit::InitStatic(uiXml, path, 0, m_quick_slots_icons.back());
        xr_sprintf(path, "%s:counter", path);
        UIHelper::CreateStatic(uiXml, path, m_quick_slots_icons.back());
    }
    m_QuickSlotText1 = UIHelper::CreateTextWnd(uiXml, "quick_slot0_text", this);
    m_QuickSlotText2 = UIHelper::CreateTextWnd(uiXml, "quick_slot1_text", this);
    m_QuickSlotText3 = UIHelper::CreateTextWnd(uiXml, "quick_slot2_text", this);
    m_QuickSlotText4 = UIHelper::CreateTextWnd(uiXml, "quick_slot3_text", this);

    HUD_SOUND_ITEM::LoadSound("maingame_ui", "snd_new_contact", m_contactSnd, SOUND_TYPE_IDLE);
}

float UIStaticDiskIO_start_time = 0.0f;
void CUIMainIngameWnd::Draw()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

    // show IO icon
    bool IOActive = (FS.dwOpenCounter > 0);
    if (IOActive)
        UIStaticDiskIO_start_time = Device.fTimeGlobal;

    if ((UIStaticDiskIO_start_time + 1.0f) < Device.fTimeGlobal)
        UIStaticDiskIO->Show(false);
    else
    {
        u32 alpha = clampr(iFloor(255.f * (1.f - (Device.fTimeGlobal - UIStaticDiskIO_start_time) / 1.f)), 0, 255);
        UIStaticDiskIO->Show(true);
        UIStaticDiskIO->SetTextureColor(color_rgba(255, 255, 255, alpha));
    }
    FS.dwOpenCounter = 0;

    if (!IsGameTypeSingle())
    {
        float luminocity = smart_cast<CGameObject*>(Level().CurrentEntity())->ROS()->get_luminocity();
        float power = log(luminocity > .001f ? luminocity : .001f) * (1.f /*luminocity_factor*/);
        luminocity = exp(power);

        static float cur_lum = luminocity;
        cur_lum = luminocity * 0.01f + cur_lum * 0.99f;
        UIMotionIcon->SetLuminosity((s16)iFloor(cur_lum * 100.0f));
    }
    if (!pActor || !pActor->g_Alive())
        return;

    UIMotionIcon->SetNoise((s16)(0xffff & iFloor(pActor->m_snd_noise * 100)));

	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
    CActor* pActorOwner = smart_cast<CActor*>(pInvOwner);
    CPda* pda = pActorOwner->GetPDA();

    if (g_SingleGameDifficulty != egdMaster && pInvOwner && pActorOwner && pda && pda->GetCondition() > 0.0)
    {
        UIZoneMap->visible = true;
        UIZoneMap->Render();

        UIMotionIcon->Draw();
        bool tmp = UIMotionIcon->IsShown();
        UIMotionIcon->Show(false);
        UIMotionIcon->Show(tmp);
    }

    CUIWindow::Draw();

    RenderQuickInfos();
}

void CUIMainIngameWnd::SetMPChatLog(CUIWindow* pChat, CUIWindow* pLog)
{
    m_pMPChatWnd = pChat;
    m_pMPLogWnd = pLog;
}

void CUIMainIngameWnd::Update()
{
    CUIWindow::Update();
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

    if (m_pMPChatWnd)
        m_pMPChatWnd->Update();

    if (m_pMPLogWnd)
        m_pMPLogWnd->Update();

    if (!pActor)
        return;

    UIZoneMap->Update();

    //	UIHealthBar.SetProgressPos	(m_pActor->GetfHealth()*100.0f);
    //	UIMotionIcon->SetPower		(m_pActor->conditions().GetPower()*100.0f);

    UpdatePickUpItem();

    if (Device.dwFrame % 10)
        return;

    bool b_God = GodMode()? true : false;
    if (b_God)
    {
        SetWarningIconColor(ewiInvincible, 0xffffffff);
    }
    else
    {
        SetWarningIconColor(ewiInvincible, 0x00ffffff);
    }

    UpdateMainIndicators();
}

void CUIMainIngameWnd::RenderQuickInfos()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    static CGameObject* pObject = NULL;
    LPCSTR actor_action = pActor->GetDefaultActionForObject();
    UIStaticQuickHelp->Show(NULL != actor_action);

    if (NULL != actor_action)
    {
        if (xr_stricmp(actor_action, UIStaticQuickHelp->GetText()))
            UIStaticQuickHelp->SetTextST(actor_action);
    }

    if (pObject != pActor->ObjectWeLookingAt())
    {
        UIStaticQuickHelp->SetTextST(actor_action ? actor_action : " ");
        UIStaticQuickHelp->ResetColorAnimation();
        pObject = pActor->ObjectWeLookingAt();
    }
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
    VERIFY(news->texture_name.size());

    CurrentGameUI()->m_pMessagesWnd->AddIconedPdaMessage(news);
    CurrentGameUI()->UpdatePda();
}

void CUIMainIngameWnd::SetWarningIconColorUI(CUIStatic* s, const u32 cl)
{
    int bOn = (cl >> 24);
    bool bIsShown = s->IsShown();

    if (bOn)
    {
        s->SetTextureColor(cl);
    }

    if (bOn && !bIsShown)
    {
        m_UIIcons->AddWindow(s, false);
        s->Show(true);
    }

    if (!bOn && bIsShown)
    {
        m_UIIcons->RemoveWindow(s);
        s->Show(false);
    }
}

void CUIMainIngameWnd::SetWarningIconColor(EWarningIcons icon, const u32 cl)
{
    bool bMagicFlag = true;

    // Задаем цвет требуемой иконки
    switch (icon)
    {
    case ewiAll: bMagicFlag = false;
    case ewiWeaponJammed:
        SetWarningIconColorUI(UIWeaponJammedIcon, cl);
        if (bMagicFlag)
            break;

    /*	case ewiRadiation:
            SetWarningIconColorUI	(&UIRadiaitionIcon, cl);
            if (bMagicFlag) break;
        case ewiWound:
            SetWarningIconColorUI	(&UIWoundIcon, cl);
            if (bMagicFlag) break;

        case ewiStarvation:
            SetWarningIconColorUI	(&UIStarvationIcon, cl);
            if (bMagicFlag) break;
        case ewiPsyHealth:
            SetWarningIconColorUI	(&UIPsyHealthIcon, cl);
            if (bMagicFlag) break;
    */
    case ewiInvincible:
        SetWarningIconColorUI(UIInvincibleIcon, cl);
        if (bMagicFlag)
            break;
        break;
    case ewiArtefact: SetWarningIconColorUI(UIArtefactIcon, cl); break;

    default: R_ASSERT(!"Unknown warning icon type");
    }
}

void CUIMainIngameWnd::TurnOffWarningIcon(EWarningIcons icon) { SetWarningIconColor(icon, 0x00ffffff); }
void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
    // Включаем анимацию требуемой иконки
    auto  icon = m_FlashingIcons.find(type);
    R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
    icon->second->Show(enable);
}

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
    const char* const flashingIconNodeName = "flashing_icon";
    int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

    CUIXmlInit xml_init;
    CUIStatic* pIcon = NULL;
    // Пробегаемся по всем нодам и инициализируем из них статики
    for (int i = 0; i < staticsCount; ++i)
    {
        pIcon = new CUIStatic();
        xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
        shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

        // Теперь запоминаем иконку и ее тип
        EFlashingIcons type = efiPdaTask;

        if (iconType == "pda")
            type = efiPdaTask;
        else if (iconType == "mail")
            type = efiMail;
        else
            R_ASSERT(!"Unknown type of mainingame flashing icon");

        R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

        CUIStatic*& val = m_FlashingIcons[type];
        val = pIcon;

        AttachChild(pIcon);
        pIcon->Show(false);
    }
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
    for (auto  it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
    {
        DetachChild(it->second);
        xr_delete(it->second);
    }

    m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
    for (auto  it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
        it->second->Update();
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
    UIZoneMap->Counter_ResetClrAnimation();

    if (b_snd && !UIZoneMap->disabled)
        HUD_SOUND_ITEM::PlaySound(m_contactSnd, Fvector().set(0, 0, 0), 0, true);
}

void CUIMainIngameWnd::SetPickUpItem(CInventoryItem* PickUpItem) { m_pPickUpItem = PickUpItem; };
void CUIMainIngameWnd::UpdatePickUpItem()
{
    if (!m_pPickUpItem || !Level().CurrentViewEntity() || !smart_cast<CActor*>(Level().CurrentViewEntity()))
    {
        UIPickUpItemIcon->Show(false);
        return;
    };

    shared_str sect_name = m_pPickUpItem->object().cNameSect();

    // properties used by inventory menu
    int m_iGridWidth = pSettings->r_u32(sect_name, "inv_grid_width");
    int m_iGridHeight = pSettings->r_u32(sect_name, "inv_grid_height");

    int m_iXPos = pSettings->r_u32(sect_name, "inv_grid_x");
    int m_iYPos = pSettings->r_u32(sect_name, "inv_grid_y");

    float scale_x = m_iPickUpItemIconWidth / float(m_iGridWidth * INV_GRID_WIDTH);

    float scale_y = m_iPickUpItemIconHeight / float(m_iGridHeight * INV_GRID_HEIGHT);

    scale_x = (scale_x > 1) ? 1.0f : scale_x;
    scale_y = (scale_y > 1) ? 1.0f : scale_y;

    float scale = scale_x < scale_y ? scale_x : scale_y;

    Frect texture_rect;
    texture_rect.lt.set(m_iXPos * INV_GRID_WIDTH, m_iYPos * INV_GRID_HEIGHT);
    texture_rect.rb.set(m_iGridWidth * INV_GRID_WIDTH, m_iGridHeight * INV_GRID_HEIGHT);
    texture_rect.rb.add(texture_rect.lt);
    UIPickUpItemIcon->GetStaticItem()->SetTextureRect(texture_rect);
    UIPickUpItemIcon->SetStretchTexture(true);

    UIPickUpItemIcon->SetWidth(m_iGridWidth * INV_GRID_WIDTH * scale * UI().get_current_kx());
    UIPickUpItemIcon->SetHeight(m_iGridHeight * INV_GRID_HEIGHT * scale);

    UIPickUpItemIcon->SetWndPos(
        Fvector2().set(m_iPickUpItemIconX + (m_iPickUpItemIconWidth - UIPickUpItemIcon->GetWidth()) / 2.0f,
            m_iPickUpItemIconY + (m_iPickUpItemIconHeight - UIPickUpItemIcon->GetHeight()) / 2.0f));

    UIPickUpItemIcon->SetTextureColor(color_rgba(255, 255, 255, 192));
    UIPickUpItemIcon->Show(true);
};

void CUIMainIngameWnd::OnConnected()
{
    UIZoneMap->SetupCurrentMap();
    if (m_ui_hud_states)
    {
        m_ui_hud_states->on_connected();
    }
}

void CUIMainIngameWnd::OnSectorChanged(int sector) { UIZoneMap->OnSectorChanged(sector); }
void CUIMainIngameWnd::reset_ui()
{
    m_pPickUpItem = NULL;
    UIMotionIcon->ResetVisibility();
    if (m_ui_hud_states)
    {
        m_ui_hud_states->reset_ui();
    }
}

void CUIMainIngameWnd::ShowZoneMap(bool status) { UIZoneMap->visible = status; }
void CUIMainIngameWnd::DrawZoneMap() { UIZoneMap->Render(); }
void CUIMainIngameWnd::UpdateZoneMap() { UIZoneMap->Update(); }
void CUIMainIngameWnd::UpdateMainIndicators()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    UpdateQuickSlots();
    if (IsGameTypeSingle())
        CurrentGameUI()->GetPdaMenu().UpdateRankingWnd();

    u8 flags = 0;
    flags |= LA_CYCLIC;
    flags |= LA_ONLYALPHA;
    flags |= LA_TEXTURECOLOR;
    // Bleeding icon
    float bleeding = pActor->conditions().BleedingSpeed();
    if (fis_zero(bleeding, EPS))
    {
        m_ind_bleeding->Show(false);
        m_ind_bleeding->ResetColorAnimation();
    }
    else
    {
        m_ind_bleeding->Show(true);
        if (bleeding < 0.35f)
        {
            m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_green");
            m_ind_bleeding->SetColorAnimation("ui_slow_blinking_alpha", flags);
        }
        else if (bleeding < 0.7f)
        {
            m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_yellow");
            m_ind_bleeding->SetColorAnimation("ui_medium_blinking_alpha", flags);
        }
        else
        {
            m_ind_bleeding->InitTexture("ui_inGame2_circle_bloodloose_red");
            m_ind_bleeding->SetColorAnimation("ui_fast_blinking_alpha", flags);
        }
    }
    // Radiation icon
    float radiation = pActor->conditions().GetRadiation();
    if (fis_zero(radiation, EPS))
    {
        m_ind_radiation->Show(false);
        m_ind_radiation->ResetColorAnimation();
    }
    else
    {
        m_ind_radiation->Show(true);
        if (radiation < 0.35f)
        {
            m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_green");
            m_ind_radiation->SetColorAnimation("ui_slow_blinking_alpha", flags);
        }
        else if (radiation < 0.7f)
        {
            m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_yellow");
            m_ind_radiation->SetColorAnimation("ui_medium_blinking_alpha", flags);
        }
        else
        {
            m_ind_radiation->InitTexture("ui_inGame2_circle_radiation_red");
            m_ind_radiation->SetColorAnimation("ui_fast_blinking_alpha", flags);
        }
    }
    // Psy Health icon
    float psy_health = pActor->conditions().GetPsyHealth();
    if (psy_health > 0.7)
    {
        m_ind_psy_health->Show(false);
    }
    else
    {
        m_ind_psy_health->Show(true);
        if (psy_health <= 0.7f && psy_health >= 0.5f)
        {
            m_ind_psy_health->InitTexture("ui_inGame2_circle_psy_health_green");
        }
        else if (psy_health <= 0.5f && psy_health >= 0.3f)
        {
            m_ind_psy_health->InitTexture("ui_inGame2_circle_psy_health_yellow");
        }
        else if (psy_health <= 0.3f)
        {
            m_ind_psy_health->InitTexture("ui_inGame2_circle_psy_health_red");
        }
    }

    // Satiety icon
    float satiety = pActor->conditions().GetSatiety();
    float satiety_critical = pActor->conditions().SatietyCritical();
    float satiety_koef = (satiety - satiety_critical) / (satiety >= satiety_critical ? 1 - satiety_critical : satiety_critical);
    if (satiety_koef > 0.5)
        m_ind_starvation->Show(false);
    else
    {
        m_ind_starvation->Show(true);
        if (satiety_koef > 0.0f)
            m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_green");
        else if (satiety_koef > -0.5f)
            m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_yellow");
        else
            m_ind_starvation->InitTexture("ui_inGame2_circle_hunger_red");
    }

	// Thirst icon
    float thirst = pActor->conditions().GetThirst();
    float thirst_critical = pActor->conditions().ThirstCritical();
    float thirst_koef = (thirst - thirst_critical) / (thirst >= thirst_critical ? 1 - thirst_critical : thirst_critical);
    if (thirst_koef > 0.5)
        m_ind_thirst->Show(false);
    else
    {
        m_ind_thirst->Show(true);
        if (thirst_koef > 0.0f)
            m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_green");
        else if (thirst_koef > -0.5f)
            m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_yellow");
        else
            m_ind_thirst->InitTexture("ui_inGame2_circle_thirst_red");
    }

	// M.F.S. Team Intoxication icon
    float intoxication = pActor->conditions().GetIntoxication();
    if (fis_zero(intoxication, EPS))
    {
        m_ind_intoxication->Show(false);
    }
    else
    {
        m_ind_intoxication->Show(true);
        if (intoxication < 0.35f)
        {
            m_ind_intoxication->InitTexture("ui_inGame2_circle_intoxication_green");
        }
        else if (intoxication < 0.7f)
        {
            m_ind_intoxication->InitTexture("ui_inGame2_circle_intoxication_yellow");
        }
        else
        {
            m_ind_intoxication->InitTexture("ui_inGame2_circle_intoxication_red");
        }
    }

	// M.F.S. Team Sleepeness icon
	float sleepeness = pActor->conditions().GetSleepeness();
	if (sleepeness < 0.5)
	{
		m_ind_sleepeness->Show(false);
	}
	else
	{
		m_ind_sleepeness->Show(true);
		if (sleepeness >= 0.5f && sleepeness <= 0.75f)
		{
			m_ind_sleepeness->InitTexture("ui_inGame2_circle_sleepeness_green");
		}
		else if (sleepeness >= 0.75f && sleepeness <= 0.85f)
		{
			m_ind_sleepeness->InitTexture("ui_inGame2_circle_sleepeness_yellow");
		}
		else if (sleepeness >= 0.85f)
		{
			m_ind_sleepeness->InitTexture("ui_inGame2_circle_sleepeness_red");
		}
	}

    // Armor broken icon
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(pActor->inventory().ItemFromSlot(OUTFIT_SLOT));
    m_ind_outfit_broken->Show(false);
    if (outfit)
    {
        float condition = outfit->GetCondition();
        if (condition < 0.75f)
        {
            m_ind_outfit_broken->Show(true);
            if (condition > 0.5f)
                m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_green");
            else if (condition > 0.25f)
                m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_yellow");
            else
                m_ind_outfit_broken->InitTexture("ui_inGame2_circle_Armorbroken_red");
        }
    }
    // Helmet broken icon
    CHelmet* helmet = smart_cast<CHelmet*>(pActor->inventory().ItemFromSlot(HELMET_SLOT));
    m_ind_helmet_broken->Show(false);
    if (helmet)
    {
        float condition = helmet->GetCondition();
        if (condition < 0.75f)
        {
            m_ind_helmet_broken->Show(true);
            if (condition > 0.5f)
                m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_green");
            else if (condition > 0.25f)
                m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_yellow");
            else
                m_ind_helmet_broken->InitTexture("ui_inGame2_circle_Helmetbroken_red");
        }
    }
    // Weapon broken icon
    u16 slot = pActor->inventory().GetActiveSlot();
    m_ind_weapon_broken->Show(false);
    if (slot == INV_SLOT_2 || slot == INV_SLOT_3)
    {
        CWeapon* weapon = smart_cast<CWeapon*>(pActor->inventory().ItemFromSlot(slot));
        if (weapon)
        {
            float condition = weapon->GetCondition();
            float start_misf_cond = weapon->GetMisfireStartCondition();
            float end_misf_cond = weapon->GetMisfireSecondCondition();
            if (condition < 0.8f)
            {
                m_ind_weapon_broken->Show(true);
                if (condition < start_misf_cond && condition > end_misf_cond)
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_yellow");
                else if (condition < end_misf_cond)
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_red");
                else
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_green");
            }
        }
    }
    if (slot == KNIFE_SLOT)
    {
        CWeaponKnife* knife = smart_cast<CWeaponKnife*>(pActor->inventory().ItemFromSlot(slot));
        if (knife)
        {
            float condition = knife->GetCondition();
            if (condition < 0.75f)
            {
                    m_ind_weapon_broken->Show(true);
                if (condition > 0.5f)
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_green");
                else if (condition > 0.25f)
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_yellow");
                else
                    m_ind_weapon_broken->InitTexture("ui_inGame2_circle_Gunbroken_red");
            }
        }
    }
    // Overweight icon
    float cur_weight = pActor->inventory().TotalWeight();
    float max_weight = pActor->MaxWalkWeight();
	float max_carry_weight = pActor->MaxCarryWeight();

    m_ind_overweight->Show(false);
	if (cur_weight >= max_carry_weight)
    {
        m_ind_overweight->Show(true);
		if (cur_weight >= max_weight)
            m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_red");
        // else if(cur_weight>max_weight-10.0f)
        //	m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_yellow");
        else
		{
			if (max_carry_weight / max_weight >= 0.5f)
				m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_yellow");
			else
				m_ind_overweight->InitTexture("ui_inGame2_circle_Overweight_green");
		}
    }
}

void CUIMainIngameWnd::UpdateQuickSlots()
{
    string32 tmp;
    LPCSTR str = StringTable().translate("quick_use_str_1").c_str();
    strncpy_s(tmp, sizeof(tmp), str, 3);
    if (tmp[2] == ',')
        tmp[1] = '\0';
    m_QuickSlotText1->SetTextST(tmp);

    str = StringTable().translate("quick_use_str_2").c_str();
    strncpy_s(tmp, sizeof(tmp), str, 3);
    if (tmp[2] == ',')
        tmp[1] = '\0';
    m_QuickSlotText2->SetTextST(tmp);

    str = StringTable().translate("quick_use_str_3").c_str();
    strncpy_s(tmp, sizeof(tmp), str, 3);
    if (tmp[2] == ',')
        tmp[1] = '\0';
    m_QuickSlotText3->SetTextST(tmp);

    str = StringTable().translate("quick_use_str_4").c_str();
    strncpy_s(tmp, sizeof(tmp), str, 3);
    if (tmp[2] == ',')
        tmp[1] = '\0';
    m_QuickSlotText4->SetTextST(tmp);

    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    for (u8 i = 0; i < 4; i++)
    {
        CUIStatic* wnd = smart_cast<CUIStatic*>(m_quick_slots_icons[i]->FindChild("counter"));
        if (wnd)
        {
            shared_str item_name = g_quick_use_slots[i];
            if (item_name.size())
            {
                u32 count = pActor->inventory().dwfGetSameItemCount(item_name.c_str(), true);
                string32 str;
                xr_sprintf(str, "x%d", count);
                wnd->TextItemControl()->SetText(str);
                wnd->Show(true);

                CUIStatic* main_slot = m_quick_slots_icons[i];
                main_slot->SetShader(InventoryUtilities::GetEquipmentIconsShader());
                Frect texture_rect;
                texture_rect.x1 = pSettings->r_float(item_name, "inv_grid_x") * INV_GRID_WIDTH;
                texture_rect.y1 = pSettings->r_float(item_name, "inv_grid_y") * INV_GRID_HEIGHT;
                texture_rect.x2 = pSettings->r_float(item_name, "inv_grid_width") * INV_GRID_WIDTH;
                texture_rect.y2 = pSettings->r_float(item_name, "inv_grid_height") * INV_GRID_HEIGHT;
                texture_rect.rb.add(texture_rect.lt);
                main_slot->SetTextureRect(texture_rect);
                main_slot->TextureOn();
                main_slot->SetStretchTexture(true);
                if (!count)
                {
                    wnd->SetTextureColor(color_rgba(255, 255, 255, 0));
                    wnd->TextItemControl()->SetTextColor(color_rgba(255, 255, 255, 0));
                    m_quick_slots_icons[i]->SetTextureColor(color_rgba(255, 255, 255, 100));
                }
                else
                {
                    wnd->SetTextureColor(color_rgba(255, 255, 255, 255));
                    wnd->TextItemControl()->SetTextColor(color_rgba(255, 255, 255, 255));
                    m_quick_slots_icons[i]->SetTextureColor(color_rgba(255, 255, 255, 255));
                }
            }
            else
            {
                wnd->Show(false);
                m_quick_slots_icons[i]->SetTextureColor(color_rgba(255, 255, 255, 0));
                //				m_quick_slots_icons[i]->Show(false);
            }
        }
    }
}

void CUIMainIngameWnd::DrawMainIndicatorsForInventory()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    UpdateQuickSlots();
    UpdateBoosterIndicators(pActor->conditions().GetCurBoosterInfluences());

    for (int i = 0; i < 4; i++)
        m_quick_slots_icons[i]->Draw();

    m_QuickSlotText1->Draw();
    m_QuickSlotText2->Draw();
    m_QuickSlotText3->Draw();
    m_QuickSlotText4->Draw();

    if (m_ind_boost_psy->IsShown())
    {
        m_ind_boost_psy->Update();
        m_ind_boost_psy->Draw();
    }

    if (m_ind_boost_radia->IsShown())
    {
        m_ind_boost_radia->Update();
        m_ind_boost_radia->Draw();
    }

    if (m_ind_boost_chem->IsShown())
    {
        m_ind_boost_chem->Update();
        m_ind_boost_chem->Draw();
    }

    if (m_ind_boost_wound->IsShown())
    {
        m_ind_boost_wound->Update();
        m_ind_boost_wound->Draw();
    }

    if (m_ind_boost_weight->IsShown())
    {
        m_ind_boost_weight->Update();
        m_ind_boost_weight->Draw();
    }

    if (m_ind_boost_health->IsShown())
    {
        m_ind_boost_health->Update();
        m_ind_boost_health->Draw();
    }

    if (m_ind_boost_power->IsShown())
    {
        m_ind_boost_power->Update();
        m_ind_boost_power->Draw();
    }

    if (m_ind_boost_rad->IsShown())
    {
        m_ind_boost_rad->Update();
        m_ind_boost_rad->Draw();
    }

    m_ui_hud_states->DrawZoneIndicators();
}

void CUIMainIngameWnd::UpdateBoosterIndicators(const xr_map<EBoostParams, SBooster>& influences)
{
    m_ind_boost_psy->Show(false);
    m_ind_boost_radia->Show(false);
    m_ind_boost_chem->Show(false);
    m_ind_boost_wound->Show(false);
    m_ind_boost_weight->Show(false);
    m_ind_boost_health->Show(false);
    m_ind_boost_power->Show(false);
    m_ind_boost_rad->Show(false);

    LPCSTR str_flag = "ui_slow_blinking_alpha";
    u8 flags = 0;
    flags |= LA_CYCLIC;
    flags |= LA_ONLYALPHA;
    flags |= LA_TEXTURECOLOR;

    xr_map<EBoostParams, SBooster>::const_iterator b = influences.begin(), e = influences.end();
    for (; b != e; b++)
    {
        switch (b->second.m_type)
        {
        case eBoostHpRestore:
        {
            m_ind_boost_health->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_health->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_health->ResetColorAnimation();
        }
        break;
        case eBoostPowerRestore:
        {
            m_ind_boost_power->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_power->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_power->ResetColorAnimation();
        }
        break;
        case eBoostRadiationRestore:
        {
            m_ind_boost_rad->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_rad->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_rad->ResetColorAnimation();
        }
        break;
        case eBoostBleedingRestore:
        {
            m_ind_boost_wound->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_wound->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_wound->ResetColorAnimation();
        }
        break;
        case eBoostMaxWeight:
        {
            m_ind_boost_weight->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_weight->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_weight->ResetColorAnimation();
        }
        break;
        case eBoostRadiationImmunity:
        case eBoostRadiationProtection:
        {
            m_ind_boost_radia->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_radia->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_radia->ResetColorAnimation();
        }
        break;
        case eBoostTelepaticImmunity:
        case eBoostTelepaticProtection:
        {
            m_ind_boost_psy->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_psy->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_psy->ResetColorAnimation();
        }
        break;
        case eBoostChemicalBurnImmunity:
        case eBoostChemicalBurnProtection:
        {
            m_ind_boost_chem->Show(true);
            if (b->second.fBoostTime <= 3.0f)
                m_ind_boost_chem->SetColorAnimation(str_flag, flags);
            else
                m_ind_boost_chem->ResetColorAnimation();
        }
        break;
        }
    }
}
