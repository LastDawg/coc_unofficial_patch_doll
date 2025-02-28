////////////////////////////////////////////////////////////////////////////
//	Module 		: level_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Level script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Level.h"
#include "Actor.h"
#include "script_game_object.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "xrServer.h"
#include "client_spawn_manager.h"
#include "xrEngine/IGame_Persistent.h"
#include "game_cl_base.h"
#include "UIGameCustom.h"
#include "UI/UIDialogWnd.h"
#include "date_time.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "PHCommander.h"
#include "PHScriptCall.h"
#include "xrScriptEngine/script_engine.hpp"
#include "game_cl_single.h"
#include "game_sv_single.h"
#include "map_manager.h"
#include "map_spot.h"
#include "map_location.h"
#include "physics_world_scripted.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"
#include "UI/UIGameTutorial.h"
#include "string_table.h"
#include "ui/UIInventoryUtilities.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "HUDManager.h"
#include "raypick.h"
#include "xrCDB/xr_collide_defs.h"
#include "GamePersistent.h"
#include "player_hud.h"
#include "../xrPhysics/ElevatorState.h"

using namespace luabind;
using namespace luabind::policy;
bool g_block_all_except_movement;

LPCSTR command_line() { return Core.Params; }
bool IsDynamicMusic() { return !!psActorFlags.test(AF_DYNAMIC_MUSIC); }
bool IsImportantSave() { return !!psActorFlags.test(AF_IMPORTANT_SAVE); }
#ifdef DEBUG
void check_object(CScriptGameObject* object)
{
    try
    {
        Msg("check_object %s", object->Name());
    }
    catch (...)
    {
        object = object;
    }
}

CScriptGameObject* tpfGetActor()
{
    static bool first_time = true;
    if (first_time)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Do not use level.actor function!");
    first_time = false;

    CActor* l_tpActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (l_tpActor)
        return (smart_cast<CGameObject*>(l_tpActor)->lua_game_object());
    else
        return (0);
}

CScriptGameObject* get_object_by_name(LPCSTR caObjectName)
{
    static bool first_time = true;
    if (first_time)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Do not use level.object function!");
    first_time = false;

    CGameObject* l_tpGameObject = smart_cast<CGameObject*>(Level().Objects.FindObjectByName(caObjectName));
    if (l_tpGameObject)
        return (l_tpGameObject->lua_game_object());
    else
        return (0);
}
#endif

CScriptGameObject* get_object_by_id(u16 id)
{
    CGameObject* pGameObject = smart_cast<CGameObject*>(Level().Objects.net_Find(id));
    if (!pGameObject)
        return NULL;

    return pGameObject->lua_game_object();
}

LPCSTR get_weather() { return *g_pGamePersistent->Environment().GetWeather(); }
void set_weather(pcstr const weather_name, const bool forced)
{
    if (!Device.editor())
        g_pGamePersistent->Environment().SetWeather(weather_name, forced);
}

bool set_weather_fx(pcstr const weather_name)
{
    if (!Device.editor())
        return g_pGamePersistent->Environment().SetWeatherFX(weather_name);

    return false;
}

bool start_weather_fx_from_time(pcstr const weather_name, const float time)
{
    if (!Device.editor())
        return g_pGamePersistent->Environment().StartWeatherFXFromTime(weather_name, time);

    return false;
}

bool is_wfx_playing() { return (g_pGamePersistent->Environment().IsWFXPlaying()); }
float get_wfx_time() { return (g_pGamePersistent->Environment().wfx_time); }
void stop_weather_fx() { g_pGamePersistent->Environment().StopWFX(); }
void set_time_factor(const float time_factor)
{
    if (!OnServer() || Device.editor())
        return;

    Level().Server->GetGameState()->SetGameTimeFactor(time_factor);
	GamePersistent().Environment().SetGameTime( Level().GetEnvironmentGameDayTimeSec(), Level().game->GetEnvironmentGameTimeFactor() );
}

void block_player_action(EGameActions dik)
{
	Actor()->block_action(dik);
}

void unblock_player_action(EGameActions dik)
{
	Actor()->unblock_action(dik);
}

float get_time_factor() 
{ 
    return (Level().GetGameTimeFactor()); 
}

void set_global_time_factor(float tf)
{
    if (!OnServer())
        return;

    Device.time_factor(tf);
}

float get_global_time_factor() { return (Device.time_factor()); }

void set_game_difficulty(ESingleGameDifficulty dif)
{
    g_SingleGameDifficulty = dif;
    game_cl_Single* game = smart_cast<game_cl_Single*>(Level().game);
    VERIFY(game);
    game->OnDifficultyChanged();
}
ESingleGameDifficulty get_game_difficulty() { return g_SingleGameDifficulty; }
u32 get_time_days()
{
    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
    split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year,
        month, day, hours, mins, secs, milisecs);
    return day;
}

u32 get_time_hours()
{
    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
    split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year,
        month, day, hours, mins, secs, milisecs);
    return hours;
}

u32 get_time_minutes()
{
    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
    split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year,
        month, day, hours, mins, secs, milisecs);
    return mins;
}

void change_game_time(u32 days, u32 hours, u32 mins)
{
    game_sv_Single* tpGame = smart_cast<game_sv_Single*>(Level().Server->GetGameState());
    if (tpGame && ai().get_alife())
    {
        u32 value = days * 86400 + hours * 3600 + mins * 60;
        float fValue = static_cast<float>(value);
        value *= 1000; // msec
        g_pGamePersistent->Environment().ChangeGameTime(fValue);
        tpGame->alife().time_manager().change_game_time(value);
    }
}

float high_cover_in_direction(u32 level_vertex_id, const Fvector& direction)
{
    float y, p;
    direction.getHP(y, p);
    return (ai().level_graph().high_cover_in_direction(y, level_vertex_id));
}

float low_cover_in_direction(u32 level_vertex_id, const Fvector& direction)
{
    float y, p;
    direction.getHP(y, p);
    return (ai().level_graph().low_cover_in_direction(y, level_vertex_id));
}

float rain_factor() { return (g_pGamePersistent->Environment().CurrentEnv->rain_density); }
u32 vertex_in_direction(u32 level_vertex_id, Fvector direction, float max_distance)
{
    direction.normalize_safe();
    direction.mul(max_distance);
    Fvector start_position = ai().level_graph().vertex_position(level_vertex_id);
    Fvector finish_position = Fvector(start_position).add(direction);
    u32 result = u32(-1);
    ai().level_graph().farthest_vertex_in_direction(level_vertex_id, start_position, finish_position, result, 0);
    return (ai().level_graph().valid_vertex_id(result) ? result : level_vertex_id);
}

Fvector vertex_position(u32 level_vertex_id)
{
    return (ai().level_graph().vertex_position(level_vertex_id));
}

bool valid_vertex(u32 level_vertex_id)
{
    return ai().level_graph().valid_vertex_id(level_vertex_id);
}

void map_add_object_spot(u16 id, LPCSTR spot_type, LPCSTR text)
{
    CMapLocation* ml = Level().MapManager().AddMapLocation(spot_type, id);
    if (xr_strlen(text))
    {
        ml->SetHint(text);
    }
}

void map_add_object_spot_ser(u16 id, LPCSTR spot_type, LPCSTR text)
{
    CMapLocation* ml = Level().MapManager().AddMapLocation(spot_type, id);
    if (xr_strlen(text))
        ml->SetHint(text);

    ml->SetSerializable(true);
}

void map_change_spot_hint(u16 id, LPCSTR spot_type, LPCSTR text)
{
    CMapLocation* ml = Level().MapManager().GetMapLocation(spot_type, id);
    if (!ml)
        return;
    ml->SetHint(text);
}

void map_remove_object_spot(u16 id, LPCSTR spot_type) { Level().MapManager().RemoveMapLocation(spot_type, id); }
u16 map_has_object_spot(u16 id, LPCSTR spot_type) { return Level().MapManager().HasMapLocation(spot_type, id); }
bool patrol_path_exists(LPCSTR patrol_path) { return (!!ai().patrol_paths().path(patrol_path, true)); }
LPCSTR get_name() {
	if (Level().name().size())
		return Level().name().c_str();

	//Alun: This fixes level.name() being an empty string when checking it while server entities are being registered
	return ai().game_graph().header().level(ai().level_graph().level_id()).name().c_str();
}
void prefetch_sound(LPCSTR name) { Level().PrefetchSound(name); }
CClientSpawnManager& get_client_spawn_manager() { return (Level().client_spawn_manager()); }

void add_dialog_to_render(CUIDialogWnd* pDialog) { CurrentGameUI()->AddDialogToRender(pDialog); }
void remove_dialog_to_render(CUIDialogWnd* pDialog) { CurrentGameUI()->RemoveDialogToRender(pDialog); }
void hide_indicators()
{
    if (CurrentGameUI())
    {
        CurrentGameUI()->HideShownDialogs();
        CurrentGameUI()->ShowGameIndicators(false);
        CurrentGameUI()->ShowCrosshair(false);
    }
    psActorFlags.set(AF_GODMODE_RT, TRUE);
}

void hide_indicators_safe()
{
    if (CurrentGameUI())
    {
        CurrentGameUI()->ShowGameIndicators(false);
        CurrentGameUI()->ShowCrosshair(false);

        CurrentGameUI()->OnExternalHideIndicators();
    }
    psActorFlags.set(AF_GODMODE_RT, TRUE);
}

void show_indicators()
{
    if (CurrentGameUI())
    {
        CurrentGameUI()->ShowGameIndicators(true);
        CurrentGameUI()->ShowCrosshair(true);
    }
    psActorFlags.set(AF_GODMODE_RT, FALSE);
}

void show_weapon(bool b) { psHUD_Flags.set(HUD_WEAPON_RT2, b); }
bool is_level_present() { return (!!g_pGameLevel); }
void add_call(const luabind::functor<bool>& condition, const luabind::functor<void>& action)
{
    luabind::functor<bool> _condition = condition;
    luabind::functor<void> _action = action;
    CPHScriptCondition* c = new CPHScriptCondition(_condition);
    CPHScriptAction* a = new CPHScriptAction(_action);
    Level().ph_commander_scripts().add_call(c, a);
}

void remove_call(const luabind::functor<bool>& condition, const luabind::functor<void>& action)
{
    CPHScriptCondition c(condition);
    CPHScriptAction a(action);
    Level().ph_commander_scripts().remove_call(&c, &a);
}

void add_call(const luabind::object& lua_object, LPCSTR condition, LPCSTR action)
{
    luabind::functor<bool> _condition = object_cast<luabind::functor<bool>>(lua_object[condition]);
    luabind::functor<void> _action = object_cast<luabind::functor<void>>(lua_object[action]);
    CPHScriptObjectConditionN* c = new CPHScriptObjectConditionN(lua_object, _condition);
    CPHScriptObjectActionN* a = new CPHScriptObjectActionN(lua_object, _action);
    Level().ph_commander_scripts().add_call_unique(c, c, a, a);
}

void remove_call(const luabind::object& lua_object, LPCSTR condition, LPCSTR action)
{
    CPHScriptObjectCondition c(lua_object, condition);
    CPHScriptObjectAction a(lua_object, action);
    Level().ph_commander_scripts().remove_call(&c, &a);
}

void add_call(
    const luabind::object& lua_object, const luabind::functor<bool>& condition, const luabind::functor<void>& action)
{
    CPHScriptObjectConditionN* c = new CPHScriptObjectConditionN(lua_object, condition);
    CPHScriptObjectActionN* a = new CPHScriptObjectActionN(lua_object, action);
    Level().ph_commander_scripts().add_call(c, a);
}

void remove_call(
    const luabind::object& lua_object, const luabind::functor<bool>& condition, const luabind::functor<void>& action)
{
    CPHScriptObjectConditionN c(lua_object, condition);
    CPHScriptObjectActionN a(lua_object, action);
    Level().ph_commander_scripts().remove_call(&c, &a);
}

void remove_calls_for_object(const luabind::object& lua_object)
{
    CPHSriptReqObjComparer c(lua_object);
    Level().ph_commander_scripts().remove_calls(&c);
}

cphysics_world_scripted* physics_world_scripted()
{
    return get_script_wrapper<cphysics_world_scripted>(*physics_world());
}
CEnvironment* environment() { return (g_pGamePersistent->pEnvironment); }
CEnvDescriptor* current_environment(CEnvironment* self) { return (self->CurrentEnv); }
extern bool g_bDisableAllInput;
void disable_input()
{
    g_bDisableAllInput = true;
#ifdef DEBUG
    Msg("input disabled");
#endif // #ifdef DEBUG
}
void enable_input()
{
    g_bDisableAllInput = false;
#ifdef DEBUG
    Msg("input enabled");
#endif // #ifdef DEBUG
}

void spawn_phantom(const Fvector& position) { Level().spawn_item("m_phantom", position, u32(-1), u16(-1), false); }
Fbox get_bounding_volume() { return Level().ObjectSpace.GetBoundingVolume(); }
void iterate_sounds(LPCSTR prefix, u32 max_count, const CScriptCallbackEx<void>& callback)
{
    for (int j = 0, N = _GetItemCount(prefix); j < N; ++j)
    {
        string_path fn, s;
        LPSTR S = (LPSTR)&s;
        _GetItem(prefix, j, s);
        if (FS.exist(fn, "$game_sounds$", S, ".ogg"))
            callback(prefix);

        for (u32 i = 0; i < max_count; ++i)
        {
            string_path name;
            xr_sprintf(name, "%s%d", S, i);
            if (FS.exist(fn, "$game_sounds$", name, ".ogg"))
                callback(name);
        }
    }
}

void iterate_sounds1(LPCSTR prefix, u32 max_count, luabind::functor<void> functor)
{
    CScriptCallbackEx<void> temp;
    temp.set(functor);
    iterate_sounds(prefix, max_count, temp);
}

void iterate_sounds2(LPCSTR prefix, u32 max_count, luabind::object object, luabind::functor<void> functor)
{
    CScriptCallbackEx<void> temp;
    temp.set(functor, object);
    iterate_sounds(prefix, max_count, temp);
}

#include "actoreffector.h"
float add_cam_effector(LPCSTR fn, int id, bool cyclic, LPCSTR cb_func)
{
    CAnimatorCamEffectorScriptCB* e = new CAnimatorCamEffectorScriptCB(cb_func);
    e->SetType((ECamEffectorType)id);
    e->SetCyclic(cyclic);
    e->Start(fn);
    Actor()->Cameras().AddCamEffector(e);
    return e->GetAnimatorLength();
}

float add_cam_effector2(LPCSTR fn, int id, bool cyclic, LPCSTR cb_func, float cam_fov)
{
    CAnimatorCamEffectorScriptCB* e = new CAnimatorCamEffectorScriptCB(cb_func);
    e->m_bAbsolutePositioning = true;
    e->m_fov = cam_fov;
    e->SetType((ECamEffectorType)id);
    e->SetCyclic(cyclic);
    e->Start(fn);
    Actor()->Cameras().AddCamEffector(e);
    return e->GetAnimatorLength();
}

void remove_cam_effector(int id) { Actor()->Cameras().RemoveCamEffector((ECamEffectorType)id); }
float get_snd_volume() { return psSoundVFactor; }
void set_snd_volume(float v)
{
    psSoundVFactor = v;
    clamp(psSoundVFactor, 0.0f, 1.0f);
}
#include "actor_statistic_mgr.h"
void add_actor_points(LPCSTR sect, LPCSTR detail_key, int cnt, int pts)
{
    return Actor()->StatisticMgr().AddPoints(sect, detail_key, cnt, pts);
}

void add_actor_points_str(LPCSTR sect, LPCSTR detail_key, LPCSTR str_value)
{
    return Actor()->StatisticMgr().AddPoints(sect, detail_key, str_value);
}

int get_actor_points(LPCSTR sect) { return Actor()->StatisticMgr().GetSectionPoints(sect); }
#include "ActorEffector.h"
void add_complex_effector(LPCSTR section, int id) { AddEffector(Actor(), id, section); }
void remove_complex_effector(int id) { RemoveEffector(Actor(), id); }
#include "postprocessanimator.h"
void add_pp_effector(LPCSTR fn, int id, bool cyclic)
{
    CPostprocessAnimator* pp = new CPostprocessAnimator(id, cyclic);
    pp->Load(fn);
    Actor()->Cameras().AddPPEffector(pp);
}

void remove_pp_effector(int id)
{
    CPostprocessAnimator* pp = smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

    if (pp)
        pp->Stop(1.0f);
}

void set_pp_effector_factor(int id, float f, float f_sp)
{
    CPostprocessAnimator* pp = smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

    if (pp)
        pp->SetDesiredFactor(f, f_sp);
}

void set_pp_effector_factor2(int id, float f)
{
    CPostprocessAnimator* pp = smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

    if (pp)
        pp->SetCurrentFactor(f);
}

#include "relation_registry.h"

int g_community_goodwill(LPCSTR _community, int _entity_id)
{
    CHARACTER_COMMUNITY c;
    c.set(_community);

    return RELATION_REGISTRY().GetCommunityGoodwill(c.index(), u16(_entity_id));
}

void g_set_community_goodwill(LPCSTR _community, int _entity_id, int val)
{
    CHARACTER_COMMUNITY c;
    c.set(_community);
    RELATION_REGISTRY().SetCommunityGoodwill(c.index(), u16(_entity_id), val);
}

void g_change_community_goodwill(LPCSTR _community, int _entity_id, int val)
{
    CHARACTER_COMMUNITY c;
    c.set(_community);
    RELATION_REGISTRY().ChangeCommunityGoodwill(c.index(), u16(_entity_id), val);
}

int g_get_community_relation(LPCSTR comm_from, LPCSTR comm_to)
{
    CHARACTER_COMMUNITY community_from;
    community_from.set(comm_from);
    CHARACTER_COMMUNITY community_to;
    community_to.set(comm_to);

    return RELATION_REGISTRY().GetCommunityRelation(community_from.index(), community_to.index());
}

void g_set_community_relation(LPCSTR comm_from, LPCSTR comm_to, int value)
{
    CHARACTER_COMMUNITY community_from;
    community_from.set(comm_from);
    CHARACTER_COMMUNITY community_to;
    community_to.set(comm_to);

    RELATION_REGISTRY().SetCommunityRelation(community_from.index(), community_to.index(), value);
}

int g_get_general_goodwill_between(u16 from, u16 to)
{
    CHARACTER_GOODWILL presonal_goodwill = RELATION_REGISTRY().GetGoodwill(from, to);
    VERIFY(presonal_goodwill != NO_GOODWILL);

    CSE_ALifeTraderAbstract* from_obj = smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(from));
    CSE_ALifeTraderAbstract* to_obj = smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(to));

    if (!from_obj || !to_obj)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "RELATION_REGISTRY::get_general_goodwill_between  : cannot convert obj to CSE_ALifeTraderAbstract!");
        return (0);
    }
    CHARACTER_GOODWILL community_to_obj_goodwill = RELATION_REGISTRY().GetCommunityGoodwill(from_obj->Community(), to);
    CHARACTER_GOODWILL community_to_community_goodwill =
        RELATION_REGISTRY().GetCommunityRelation(from_obj->Community(), to_obj->Community());

    return presonal_goodwill + community_to_obj_goodwill + community_to_community_goodwill;
}

u32 vertex_id(Fvector position) { return (ai().level_graph().vertex_id(position)); }
u32 render_get_dx_level() { return GEnv.Render->get_dx_level(); }
CUISequencer* g_tutorial = NULL;
CUISequencer* g_tutorial2 = NULL;

void start_tutorial(LPCSTR name)
{
    if (g_tutorial)
    {
        VERIFY(!g_tutorial2);
        g_tutorial2 = g_tutorial;
    };

    g_tutorial = new CUISequencer();
    g_tutorial->Start(name);
    if (g_tutorial2)
        g_tutorial->m_pStoredInputReceiver = g_tutorial2->m_pStoredInputReceiver;
}

void stop_tutorial()
{
    if (g_tutorial)
        g_tutorial->Stop();
}

LPCSTR tutorial_name()
{
    if (g_tutorial)
        return g_tutorial->m_name;
    return "invalid";
}

u32 PlayHudMotion(u8 hand, LPCSTR itm_name, LPCSTR anm_name, bool bMixIn = true, float speed = 1.f)
{
    return g_player_hud->script_anim_play(hand, itm_name, anm_name, bMixIn, speed);
}

void StopHudMotion() { g_player_hud->StopScriptAnim(); }

float MotionLength(LPCSTR section, LPCSTR name, float speed)
{
    return g_player_hud->motion_length_script(section, name, speed);
}

bool AllowHudMotion() { return g_player_hud->allow_script_anim(); }

void PlayBlendAnm(LPCSTR name, u8 part, float speed, float power, bool bLooped, bool no_restart)
{
    g_player_hud->PlayBlendAnm(name, part, speed, power, bLooped, no_restart);
}

void StopBlendAnm(LPCSTR name, bool bForce) { g_player_hud->StopBlendAnm(name, bForce); }

void StopAllBlendAnms(bool bForce) { g_player_hud->StopAllBlendAnms(bForce); }

float SetBlendAnmTime(LPCSTR name, float time) { return g_player_hud->SetBlendAnmTime(name, time); }

LPCSTR translate_string(LPCSTR str) { return *StringTable().translate(str); }
bool has_active_tutotial() { return (g_tutorial != NULL); }

//Alundaio: namespace level exports extension
#ifdef NAMESPACE_LEVEL_EXPORTS
//ability to update level netpacket
void g_send(NET_Packet& P, bool bReliable = false, bool bSequential = true, bool bHighPriority = false, bool bSendImmediately = false)
{
    Level().Send(P);
}

void u_event_gen(NET_Packet& P, u32 _event, u32 _dest)
{
    CGameObject::u_EventGen(P, _event, _dest);
}

void u_event_send(NET_Packet& P)
{
    CGameObject::u_EventSend(P);
}

//can spawn entities like bolts, phantoms, ammo, etc. which normally crash when using alife():create()
void spawn_section(pcstr sSection, Fvector3 vPosition, u32 LevelVertexID, u16 ParentID, bool bReturnItem = false)
{
    Level().spawn_item(sSection, vPosition, LevelVertexID, ParentID, bReturnItem);
}

//ability to get the target game_object at crosshair
CScriptGameObject* g_get_target_obj()
{
    collide::rq_result& RQ = HUD().GetCurrentRayQuery();
    if (RQ.O)
    {
        CGameObject* game_object = static_cast<CGameObject*>(RQ.O);
        if (game_object)
            return game_object->lua_game_object();
    }
    return nullptr;
}

void send_event_key_press(int dik)
{
	Level().IR_OnKeyboardPress(dik);
}
void send_event_key_release(int dik)
{
	Level().IR_OnKeyboardRelease(dik);
}
void send_event_key_hold(int dik)
{
	Level().IR_OnKeyboardHold(dik);
}
void send_event_mouse_wheel(int vol)
{
	Level().IR_OnMouseWheel(vol);
}

float g_get_target_dist()
{
    collide::rq_result& RQ = HUD().GetCurrentRayQuery();
    if (RQ.range)
        return RQ.range;
    return 0.f;
}

u32 g_get_target_element()
{
	collide::rq_result& RQ = HUD().GetCurrentRayQuery();
	if (RQ.element)
		return RQ.element;

	return 0;
}

void block_all_except_movement(bool b)
{
	g_block_all_except_movement = b;
}

bool only_movement_allowed()
{
	return g_block_all_except_movement;
}

void set_actor_allow_ladder(bool b)
{
	g_actor_allow_ladder = b;
}

bool actor_allow_ladder()
{
	return g_actor_allow_ladder;
}

float get_devices_psy_factor()
{
	if (Actor())
		return Actor()->GetDevicesPsyFactor();

	Msg("![get_devices_psy_factor]: Actor not found!");
	return 0;
}

void set_devices_psy_factor(float psy_factor)
{
	clamp(psy_factor, 0.0f, 1.0f);

	if (Actor())
	{
		Actor()->SetDevicesPsyFactor(psy_factor);
		return;
	}

	Msg("![set_devices_psy_factor]: Actor not found!");
}

u8 get_active_cam()
{
    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (actor)
        return (u8)actor->active_cam();

    return 255;
}

void set_active_cam(u8 mode)
{
    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (actor && mode <= eacMaxCam)
        actor->cam_Set((EActorCameras)mode);
}
#endif
//-Alundaio

// KD: raypick	
bool ray_pick(const Fvector& start, const Fvector& dir, float range,
              collide::rq_target tgt, script_rq_result& script_R,
              CScriptGameObject* ignore_object)
{
    collide::rq_result R;
    IGameObject* ignore = nullptr;
    if (ignore_object)
        ignore = smart_cast<IGameObject*>(&(ignore_object->object()));
    if (Level().ObjectSpace.RayPick(start, dir, range, tgt, R, ignore))
    {
        script_R.set(R);
        return true;
    }
    return false;
}

xrTime get_start_time()
{
    return xrTime(Level().GetStartGameTime());
}

void reload_language()
{
    StringTable().ReloadLanguage();
}

void LevelPressAction(EGameActions cmd) { Level().IR_OnKeyboardPress(cmd); }
void LevelReleaseAction(EGameActions cmd) { Level().IR_OnKeyboardRelease(cmd); }
void LevelHoldAction(EGameActions cmd) { Level().IR_OnKeyboardHold(cmd); }

CScriptGameObject* get_view_entity_script()
{
    CGameObject* pGameObject = smart_cast<CGameObject*>(Level().CurrentViewEntity());
    if (!pGameObject)
        return (0);

    return pGameObject->lua_game_object();
}

void set_view_entity_script(CScriptGameObject* go)
{
    IGameObject* o = smart_cast<IGameObject*>(&go->object());
    if (o)
        Level().SetViewEntity(o);
}

void qweasddMess1(NET_Packet& val1, bool val2 = false, bool val3 = true)
{
    u16 packetnum = 1;
    val1.r_begin(packetnum);

    shared_str packetstr;
    val1.r_stringZ(packetstr);

    LPCSTR str1 = val2 ? "Yes2" : "No2";
    LPCSTR str2 = val3 ? "Yes3" : "No3";
    Msg("qweasddMess1 -> packetstr = %s, val2 = %s, val3 = %s", packetstr, str1, str2);
}
void patrol_path_add(LPCSTR patrol_path, CPatrolPath* path) {
	ai().patrol_paths_raw().add_path(shared_str(patrol_path), path);
}


void patrol_path_remove(LPCSTR patrol_path) {
	ai().patrol_paths_raw().remove_path(shared_str(patrol_path));
}
// XXX nitrocaster: one can export enum like class, without defining dummy type
template<typename T>
struct EnumCallbackType {};

// clang-format off
IC static void CLevel_Export(lua_State* luaState)
{
    class_<CEnvDescriptor>("CEnvDescriptor")
        .def_readonly("fog_density", &CEnvDescriptor::fog_density)
        .def_readonly("far_plane", &CEnvDescriptor::far_plane),

        class_<CEnvironment>("CEnvironment").def("current", current_environment);

    module(luaState, "level")
    [
		def("patrol_path_add", &patrol_path_add),
		def("patrol_path_remove", &patrol_path_remove),
        //Alundaio: Extend level namespace exports
#ifdef NAMESPACE_LEVEL_EXPORTS
        def("send", &g_send),
        //def("send", (void(*)(NET_Packet&, bool))&g_send),
        //def("send", (void(*)(NET_Packet&, bool, bool))&g_send),
        //def("send", (void(*)(NET_Packet&, bool, bool, bool))&g_send),
        //def("send", (void(*)(NET_Packet&, bool, bool, bool, bool))&g_send), //allow the ability to send netpacket to level
        def("qweasddMess1", &qweasddMess1),
        //def("qweasddMess1", (void(*)(NET_Packet&, bool))&qweasddMess1),
        //def("qweasddMess1", (void(*)(NET_Packet&, bool, bool))&qweasddMess1),
        def("u_event_gen", &u_event_gen), //Send events via packet
        def("u_event_send", &u_event_send),
        def("get_target_obj", &g_get_target_obj), //intentionally named to what is in xray extensions
        def("get_target_dist", &g_get_target_dist),
        def("press_action", &LevelPressAction),
		def("release_action", &LevelReleaseAction),
		def("hold_action", &LevelHoldAction),
        def("get_target_element", &g_get_target_element), //Can get bone cursor is targeting
        def("get_view_entity", &get_view_entity_script),
        def("set_view_entity", &set_view_entity_script),
        def("spawn_item", &spawn_section),
        def("get_active_cam", &get_active_cam),
        def("set_active_cam", &set_active_cam),
        def("get_start_time", &get_start_time),
        def("valid_vertex", &valid_vertex),
#endif
        //Alundaio: END
        // obsolete\deprecated
        def("object_by_id", get_object_by_id),
#ifdef DEBUG
        def("debug_object", get_object_by_name),
        def("debug_actor", tpfGetActor),
        def("check_object", check_object),
#endif

		def("send_event_key_press",				&send_event_key_press),
		def("send_event_key_release",			&send_event_key_release),
		def("send_event_key_hold",				&send_event_key_hold),
		def("send_event_mouse_wheel",			&send_event_mouse_wheel),
		def("set_global_time_factor",			&set_global_time_factor),
		def("get_global_time_factor",			&get_global_time_factor),
		def("block_player_action",				&block_player_action),
		def("unblock_player_action",			&unblock_player_action),

        def("get_weather", get_weather),
        def("set_weather", set_weather),
        def("set_weather_fx", set_weather_fx),
        def("start_weather_fx_from_time", start_weather_fx_from_time),
        def("is_wfx_playing", is_wfx_playing),
        def("get_wfx_time", get_wfx_time),
        def("stop_weather_fx", stop_weather_fx),

        def("environment", environment),

        def("set_time_factor", set_time_factor),
        def("get_time_factor", get_time_factor),

        def("set_game_difficulty", set_game_difficulty),
        def("get_game_difficulty", get_game_difficulty),

        def("get_time_days", get_time_days),
        def("get_time_hours", get_time_hours),
        def("get_time_minutes", get_time_minutes),
        def("change_game_time", change_game_time),

        def("high_cover_in_direction", high_cover_in_direction),
        def("low_cover_in_direction", low_cover_in_direction),
        def("vertex_in_direction", vertex_in_direction),
        def("rain_factor", rain_factor),
        def("patrol_path_exists", patrol_path_exists),
        def("vertex_position", vertex_position),
        def("name", get_name),
        def("prefetch_sound", prefetch_sound),

        def("client_spawn_manager", get_client_spawn_manager),

        def("map_add_object_spot_ser", map_add_object_spot_ser),
        def("map_add_object_spot", map_add_object_spot),
        def("map_remove_object_spot", map_remove_object_spot),
        def("map_has_object_spot", map_has_object_spot),
        def("map_change_spot_hint", map_change_spot_hint),

        def("add_dialog_to_render", add_dialog_to_render),
        def("remove_dialog_to_render", remove_dialog_to_render),
        def("hide_indicators", hide_indicators),
        def("hide_indicators_safe", hide_indicators_safe),

        def("show_indicators", show_indicators),
        def("show_weapon", show_weapon),
        def("add_call", ((void (*)(const luabind::functor<bool>&, const luabind::functor<void>&)) & add_call)),
        def("add_call",
            ((void (*)(const luabind::object&, const luabind::functor<bool>&, const luabind::functor<void>&)) &
                add_call)),
        def("add_call", ((void (*)(const luabind::object&, LPCSTR, LPCSTR)) & add_call)),
        def("remove_call", ((void (*)(const luabind::functor<bool>&, const luabind::functor<void>&)) & remove_call)),
        def("remove_call",
            ((void (*)(const luabind::object&, const luabind::functor<bool>&, const luabind::functor<void>&)) &
                remove_call)),
        def("remove_call", ((void (*)(const luabind::object&, LPCSTR, LPCSTR)) & remove_call)),
        def("remove_calls_for_object", remove_calls_for_object), def("present", is_level_present),
        def("disable_input", disable_input), def("enable_input", enable_input), def("spawn_phantom", spawn_phantom),

        def("get_bounding_volume", get_bounding_volume),

        def("iterate_sounds", &iterate_sounds1), def("iterate_sounds", &iterate_sounds2),
        def("physics_world", &physics_world_scripted), def("get_snd_volume", &get_snd_volume),
        def("set_snd_volume", &set_snd_volume), def("add_cam_effector", &add_cam_effector),
        def("add_cam_effector2", &add_cam_effector2), def("remove_cam_effector", &remove_cam_effector),
        def("add_pp_effector", &add_pp_effector), def("set_pp_effector_factor", &set_pp_effector_factor),
        def("set_pp_effector_factor", &set_pp_effector_factor2), def("remove_pp_effector", &remove_pp_effector),

        def("add_complex_effector", &add_complex_effector), def("remove_complex_effector", &remove_complex_effector),

        def("vertex_id", &vertex_id),

        def("game_id", &GameID),
        def("ray_pick", &ray_pick)],

        module(luaState, "actor_stats")[def("add_points", &add_actor_points),
            def("add_points_str", &add_actor_points_str), def("get_points", &get_actor_points)];

    module(luaState)
    [
        class_<CRayPick>("ray_pick")
            .def(constructor<>())
            .def(constructor<Fvector&, Fvector&, float, collide::rq_target, CScriptGameObject*>())
            .def("set_position", &CRayPick::set_position)
            .def("set_direction", &CRayPick::set_direction)
            .def("set_range", &CRayPick::set_range)
            .def("set_flags", &CRayPick::set_flags)
            .def("set_ignore_object", &CRayPick::set_ignore_object)
            .def("query", &CRayPick::query)
            .def("get_result", &CRayPick::get_result)
            .def("get_object", &CRayPick::get_object)
            .def("get_distance", &CRayPick::get_distance)
            .def("get_element", &CRayPick::get_element),
        class_<script_rq_result>("rq_result")
            .def_readonly("object", &script_rq_result::O)
            .def_readonly("range", &script_rq_result::range)
            .def_readonly("element", &script_rq_result::element)
            .def(constructor<>()),
        class_<EnumCallbackType<collide::rq_target>>("rq_target")
        .enum_("targets")
        [
            value("rqtNone", int(collide::rqtNone)),
            value("rqtObject", int(collide::rqtObject)),
            value("rqtStatic", int(collide::rqtStatic)),
            value("rqtShape", int(collide::rqtShape)),
            value("rqtObstacle", int(collide::rqtObstacle)),
            value("rqtBoth", int(collide::rqtBoth)),
            value("rqtDyn", int(collide::rqtDyn))
        ]
    ];

    module(luaState)
    [
        def("command_line", &command_line),
        def("IsGameTypeSingle", REMOVE_NOEXCEPT(&IsGameTypeSingle)),
        def("IsDynamicMusic", &IsDynamicMusic),
        def("render_get_dx_level", &render_get_dx_level),
        def("IsImportantSave", &IsImportantSave)
    ];

    module(luaState, "relation_registry")
    [
        def("community_goodwill", &g_community_goodwill),
        def("set_community_goodwill", &g_set_community_goodwill),
        def("change_community_goodwill", &g_change_community_goodwill),

        def("community_relation", &g_get_community_relation),
        def("set_community_relation", &g_set_community_relation),
        def("get_general_goodwill_between", &g_get_general_goodwill_between)
    ];

    module(luaState, "game")
    [
        class_<xrTime>("CTime")
            .enum_("date_format")
            [
                value("DateToDay", int(InventoryUtilities::edpDateToDay)),
                value("DateToMonth", int(InventoryUtilities::edpDateToMonth)),
                value("DateToYear", int(InventoryUtilities::edpDateToYear))
            ]
            .enum_("time_format")
            [
                value("TimeToHours", int(InventoryUtilities::etpTimeToHours)),
                value("TimeToMinutes", int(InventoryUtilities::etpTimeToMinutes)),
                value("TimeToSeconds", int(InventoryUtilities::etpTimeToSeconds)),
                value("TimeToMilisecs", int(InventoryUtilities::etpTimeToMilisecs))
            ]
                              .def(constructor<>())
                              .def(constructor<const xrTime&>())
                              .def(const_self < xrTime())
                              .def(const_self <= xrTime())
                              .def(const_self > xrTime())
                              .def(const_self >= xrTime())
                              .def(const_self == xrTime())
                              .def(self + xrTime())
                              .def(self - xrTime())

                              .def("diffSec", &xrTime::diffSec_script)
                              .def("add", &xrTime::add_script)
                              .def("sub", &xrTime::sub_script)

                              .def("setHMS", &xrTime::setHMS)
                              .def("setHMSms", &xrTime::setHMSms)
                              .def("set", &xrTime::set)
                              .def("get", &xrTime::get, policy_list<out_value<2>, out_value<3>, out_value<4>,
                                                            out_value<5>, out_value<6>, out_value<7>, out_value<8>>())
                              .def("dateToString", &xrTime::dateToString)
                              .def("timeToString", &xrTime::timeToString),
        // declarations
        def("time", get_time), def("get_game_time", get_time_struct),
        //		def("get_surge_time",	Game::get_surge_time),
        //		def("get_object_by_name",Game::get_object_by_name),

        def("start_tutorial", &start_tutorial),
        def("stop_tutorial", &stop_tutorial),
        def("has_active_tutorial", &has_active_tutotial),
	    def("active_tutorial_name", &tutorial_name),
        def("translate_string", &translate_string),
	    def("play_hud_motion",		PlayHudMotion), 
	    def("stop_hud_motion",		StopHudMotion), 
	    def("get_motion_length",	MotionLength),
	    def("hud_motion_allowed",	AllowHudMotion),
	    def("play_hud_anm",			PlayBlendAnm), 
	    def("stop_hud_anm",			StopBlendAnm), 
	    def("stop_all_hud_anms",	StopAllBlendAnms),
	    def("set_hud_anm_time",		SetBlendAnmTime),
	    def("set_hud_anm_time",		SetBlendAnmTime),
	    def("only_allow_movekeys",	block_all_except_movement),
	    def("only_movekeys_allowed",only_movement_allowed),
	    def("set_actor_allow_ladder", set_actor_allow_ladder),
	    def("actor_ladder_allowed", actor_allow_ladder),
        def("reload_language", &reload_language),
        def("log_stack_trace", &xrDebug::LogStackTrace),
		def("get_devices_psy_factor", &get_devices_psy_factor),
		def("set_devices_psy_factor", &set_devices_psy_factor)
    ];
};
// clang-format on

SCRIPT_EXPORT_FUNC(CLevel, (), CLevel_Export);
