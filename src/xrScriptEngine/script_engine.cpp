////////////////////////////////////////////////////////////////////////////
//  Module      : script_engine.cpp
//  Created     : 01.04.2004
//  Modified    : 01.04.2004
//  Author      : Dmitriy Iassenev
//  Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "script_engine.hpp"
#include "script_process.hpp"
#include "script_thread.hpp"
#include "ScriptExporter.hpp"
#include "BindingsDumper.hpp"
#ifdef USE_DEBUGGER
#include "script_debugger.hpp"
#endif
#include <stdarg.h>
#include "Common/Noncopyable.hpp"
#include "xrCore/ModuleLookup.hpp"
#include "luabind/class_info.hpp"

Flags32 g_LuaDebug;

#define SCRIPT_GLOBAL_NAMESPACE "_G"

static const char* file_header_old = "local function script_name() \
return \"%s\" \
end \
local this = {} \
%s this %s \
setmetatable(this, {__index = " SCRIPT_GLOBAL_NAMESPACE "}) \
setfenv(1, this) ";

static const char* file_header_new = "local function script_name() \
return \"%s\" \
end \
local this = {} \
this." SCRIPT_GLOBAL_NAMESPACE " = " SCRIPT_GLOBAL_NAMESPACE " \
%s this %s \
setfenv(1, this) ";

static const char* file_header = nullptr;

static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (!nsize)
    {
        xr_free(ptr);
        return nullptr;
    }
    return xr_realloc(ptr, nsize);
}

static void* __cdecl luabind_allocator(void* context, const void* pointer, size_t const size)
{
    if (!size)
    {
        void* non_const_pointer = const_cast<LPVOID>(pointer);
        xr_free(non_const_pointer);
        return nullptr;
    }
    if (!pointer)
    {
        return xr_malloc(size);
    }
    void* non_const_pointer = const_cast<LPVOID>(pointer);
    return xr_realloc(non_const_pointer, size);
}

namespace
{
    void LuaJITLogError(lua_State* ls, const char* msg)
    {
        const char* info = nullptr;
        if (!lua_isnil(ls, -1))
        {
            info = lua_tostring(ls, -1);
            lua_pop(ls, 1);
        }
        Msg("! LuaJIT: %s (%s)", msg, info ? info : "no info");
    }
    // tries to execute 'jit'+command
    bool RunJITCommand(lua_State* ls, const char* command)
    {
        string128 buf;
        xr_strcpy(buf, "jit.");
        xr_strcat(buf, command);
        if (luaL_dostring(ls, buf))
        {
            LuaJITLogError(ls, "Unrecognized command");
            return false;
        }
        return true;
    }
}

const char* const CScriptEngine::GlobalNamespace = SCRIPT_GLOBAL_NAMESPACE;
Lock CScriptEngine::stateMapLock;
xr_unordered_map<lua_State*, CScriptEngine*> CScriptEngine::stateMap;

string4096 CScriptEngine::g_ca_stdout;

void CScriptEngine::reinit()
{
    stateMapLock.Enter();
    stateMap.reserve(32); // 32 lua states should be enough
    stateMapLock.Leave();
    if (m_virtual_machine)
    {
        lua_close(m_virtual_machine);
        UnregisterState(m_virtual_machine);
    }

    m_virtual_machine = luaL_newstate();
    if (!m_virtual_machine)
    {
        Log("! ERROR : Cannot initialize script virtual machine!");
        return;
    }
    RegisterState(m_virtual_machine, this);
    if (strstr(Core.Params, "-_g"))
        file_header = file_header_new;
    else
        file_header = file_header_old;
    scriptBufferSize = 1024 * 1024;
    scriptBuffer = xr_alloc<char>(scriptBufferSize);
}

int CScriptEngine::vscript_log(LuaMessageType luaMessageType, LPCSTR caFormat, va_list marker)
{
    //if (!g_LuaDebug.test(1) && luaMessageType != LuaMessageType::Error)
    //    return 0;
    LPCSTR S = "", SS = "";
    LPSTR S1;
    string4096 S2;
    switch (luaMessageType)
    {
    case LuaMessageType::Info:
        S = "* [LUA] ";
        SS = "[INFO]        ";
        break;
    case LuaMessageType::Error:
        S = "! [LUA] ";
        SS = "[ERROR]       ";
        break;
    case LuaMessageType::Message:
        S = "[LUA] ";
        SS = "[MESSAGE]     ";
        break;
    case LuaMessageType::HookCall:
        S = "[LUA][HOOK_CALL] ";
        SS = "[CALL]        ";
        break;
    case LuaMessageType::HookReturn:
        S = "[LUA][HOOK_RETURN] ";
        SS = "[RETURN]      ";
        break;
    case LuaMessageType::HookLine:
        S = "[LUA][HOOK_LINE] ";
        SS = "[LINE]        ";
        break;
    case LuaMessageType::HookCount:
        S = "[LUA][HOOK_COUNT] ";
        SS = "[COUNT]       ";
        break;
    case LuaMessageType::HookTailReturn:
        S = "[LUA][HOOK_TAIL_RETURN] ";
        SS = "[TAIL_RETURN] ";
        break;
    default: NODEFAULT;
    }
    xr_strcpy(S2, S);
    S1 = S2 + xr_strlen(S);
    int l_iResult = vsprintf(S1, caFormat, marker);
    Msg("%s", S2);
    xr_strcpy(S2, SS);
    S1 = S2 + xr_strlen(SS);
//    vsprintf(S1, caFormat, marker);
    xr_strcat(S2, "\r\n");
    m_output.w(S2, xr_strlen(S2));
    return l_iResult;
}

void CScriptEngine::print_stack(lua_State* L)
{
    if (!m_stack_is_ready || logReenterability)
        return;

    logReenterability = true;
    m_stack_is_ready = false;

    if (L == nullptr)
        L = lua();


    Log("\nLua Stack");
    lua_Debug l_tDebugInfo;
    for (int i = 0; lua_getstack(L, i, &l_tDebugInfo); i++)
    {
        lua_getinfo(L, "nSlu", &l_tDebugInfo);
        if (!l_tDebugInfo.name)
            Msg("%2d : [%s] %s(%d)", i, l_tDebugInfo.what, l_tDebugInfo.short_src, l_tDebugInfo.currentline);
        else if (!xr_strcmp(l_tDebugInfo.what, "C"))
            Msg("%2d : [C  ] %s", i, l_tDebugInfo.name);
        else
        {
            Msg("%2d : [%s] %s(%d) : %s", i, l_tDebugInfo.what, l_tDebugInfo.short_src,
                l_tDebugInfo.currentline, l_tDebugInfo.name);
        }

        // Giperion: verbose log
        Log("Lua state dump locals: ");
        pcstr name = nullptr;
        int VarID = 1;
        try
        {
            while ((name = lua_getlocal(L, &l_tDebugInfo, VarID++)) != nullptr)
            {
                LogVariable(L, name, 1);

                lua_pop(L, 1); /* remove variable value */
            }
        }
        catch (...)
        {
            Log("Can't dump lua state - Engine corrupted");
        }
        Log("End of Lua state dump.\n");
        // -Giperion
    }
    m_stack_is_ready = true;
    logReenterability = false;
}

void CScriptEngine::LogTable(lua_State* luaState, pcstr S, int level)
{
    if (!lua_istable(luaState, -1))
        return;

    lua_pushnil(luaState); /* first key */
    while (lua_next(luaState, -2) != 0)
    {
        char sname[256];
        char sFullName[256];
        lua_pushvalue(luaState, -2); //Debrovski: we should push clone of the key on top of stack
        xr_sprintf(sname, "%s", lua_tostring(luaState, -1)); //...and execute lua_tostring on this clone, not original key
        xr_sprintf(sFullName, "%s.%s", S, sname);            //..coz executing lua_tostring on original key(if not string) confuses lua_next
        lua_pop(luaState, 1);
        LogVariable(luaState, sFullName, level + 1);
        lua_pop(luaState, 1); /* removes `value'; keeps `key' for next iteration */
    }
}

void CScriptEngine::LogVariable(lua_State* luaState, pcstr name, int level)
{
    using namespace luabind::detail;
    const int ntype = lua_type(luaState, -1);
    const pcstr type = lua_typename(luaState, ntype);

    char tabBuffer[32] = {0};
    memset(tabBuffer, '\t', level);

    string512 value;

    switch (ntype)
    {
    case LUA_TNIL:
        xr_strcpy(value, "nil");
        break;

    case LUA_TFUNCTION:
        xr_strcpy(value, "[function]");
        break;

    case LUA_TTHREAD:
        xr_strcpy(value, "[thread]");
        break;

    case LUA_TNUMBER:
        xr_sprintf(value, "%f", lua_tonumber(luaState, -1));
        break;

    case LUA_TBOOLEAN:
        xr_sprintf(value, "%s", lua_toboolean(luaState, -1) ? "true" : "false");
        break;

    case LUA_TSTRING:
        xr_sprintf(value, "%.127s", lua_tostring(luaState, -1));
        break;

    case LUA_TTABLE:
    {
        if (level <= 3)
        {
            Msg("%s Table: %s", tabBuffer, name);
            LogTable(luaState, name, level + 1);
            return;
        }
        break;
    }

    // XXX: can we process lightuserdata like userdata? In other words, is this fallthrough allowed?
    // case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
    {
        object_rep* object = get_instance(luaState, -1);
        if (!object)
        {
            xr_strcpy(value, "Error! Can't get instance!");
            break;
        }

        class_rep* rep = object->crep();
        if (!rep)
        {
            xr_strcpy(value, "Error! Class userdata is null!");
            break;
        }



        pcstr className = rep->name();
        if (className)
        {
            xr_sprintf(value, "'%s' -> %s", className, m_userdataObjectLoggerFunc(object).c_str());
        }


        break;
    }

    default:
        xr_strcpy(value, "[not available]");
        break;
    }

    Msg("%s %s %s : %s", tabBuffer, type, name, value);
}

int CScriptEngine::script_log(LuaMessageType message, LPCSTR caFormat, ...)
{
    va_list marker;
    va_start(marker, caFormat);
    int result = vscript_log(message, caFormat, marker);
    va_end(marker);

#if defined(DEBUG) || defined(COC_EDITION)
    if (message == LuaMessageType::Error)
        print_stack();
#endif

    return result;
}

bool CScriptEngine::parse_namespace(LPCSTR caNamespaceName, LPSTR b, u32 b_size, LPSTR c, u32 c_size)
{
    *b = 0;
    *c = 0;
    LPSTR S2;
    STRCONCAT(S2, caNamespaceName);
    LPSTR S = S2;
    for (int i = 0;; i++)
    {
        if (!xr_strlen(S))
        {
            script_log(LuaMessageType::Error, "the namespace name %s is incorrect!", caNamespaceName);
            return false;
        }
        LPSTR S1 = strchr(S, '.');
        if (S1)
            *S1 = 0;
        if (i)
            xr_strcat(b, b_size, "{");
        xr_strcat(b, b_size, S);
        xr_strcat(b, b_size, "=");
        if (i)
            xr_strcat(c, c_size, "}");
        if (S1)
            S = ++S1;
        else
            break;
    }
    return true;
}

bool CScriptEngine::load_buffer(
lua_State* L, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int l_iErrorCode;
    if (caNameSpaceName && xr_strcmp(GlobalNamespace, caNameSpaceName))
    {
        string512 insert, a, b;
        LPCSTR header = file_header;
        if (!parse_namespace(caNameSpaceName, a, sizeof(a), b, sizeof(b)))
            return false;
        xr_sprintf(insert, header, caNameSpaceName, a, b);
        u32 str_len = xr_strlen(insert);
        u32 const total_size = str_len + tSize;
        if (total_size >= scriptBufferSize)
        {
            scriptBufferSize = total_size;
            scriptBuffer = (char*)xr_realloc(scriptBuffer, scriptBufferSize);
        }
        xr_strcpy(scriptBuffer, total_size, insert);
        CopyMemory(scriptBuffer + str_len, caBuffer, u32(tSize));
        l_iErrorCode = luaL_loadbuffer(L, scriptBuffer, tSize + str_len, caScriptName);
    }
    else
        l_iErrorCode = luaL_loadbuffer(L, caBuffer, tSize, caScriptName);
    if (l_iErrorCode)
    {
        onErrorCallback(L, caScriptName, l_iErrorCode);
        return false;
    }
    return true;
}

bool CScriptEngine::do_file(LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
    int start = lua_gettop(lua());
    string_path l_caLuaFileName;
    IReader* l_tpFileReader = FS.r_open(caScriptName);
    if (!l_tpFileReader)
    {
        script_log(LuaMessageType::Error, "Cannot open file \"%s\"", caScriptName);
        return false;
    }
    strconcat(sizeof(l_caLuaFileName), l_caLuaFileName, "@", caScriptName);
    if (!load_buffer(lua(), static_cast<LPCSTR>(l_tpFileReader->pointer()), (size_t)l_tpFileReader->length(),
        l_caLuaFileName, caNameSpaceName))
    {
        // VERIFY(lua_gettop(lua())>=4);
        // lua_pop(lua(), 4);
        // VERIFY(lua_gettop(lua())==start-3);
        lua_settop(lua(), start);
        FS.r_close(l_tpFileReader);
        return false;
    }
    FS.r_close(l_tpFileReader);
    int errFuncId = -1;
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        errFuncId = debugger()->PrepareLua(lua());
#endif
#endif
    if (0) //.
    {
        for (int i = 0; lua_type(lua(), -i - 1); i++)
            Msg("%2d : %s", -i - 1, lua_typename(lua(), lua_type(lua(), -i - 1)));
    }
    // because that's the first and the only call of the main chunk - there is no point to compile it
    // luaJIT_setmode(lua(), 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF); // Oles
    int l_iErrorCode = lua_pcall(lua(), 0, 0, (-1 == errFuncId) ? 0 : errFuncId); // new_Andy
// luaJIT_setmode(lua(), 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON); // Oles
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        debugger()->UnPrepareLua(lua(), errFuncId);
#endif
#endif
    if (l_iErrorCode)
    {
        onErrorCallback(lua(), caScriptName, l_iErrorCode);
        return false;
    }
    return true;
}

bool CScriptEngine::load_file_into_namespace(LPCSTR caScriptName, LPCSTR caNamespaceName)
{
    int start = lua_gettop(lua());
    if (!do_file(caScriptName, caNamespaceName))
    {
        lua_settop(lua(), start);
        return false;
    }
    VERIFY(lua_gettop(lua()) == start);
    return true;
}

bool CScriptEngine::namespace_loaded(LPCSTR name, bool remove_from_stack)
{
    int start = lua_gettop(lua());
    lua_pushstring(lua(), GlobalNamespace);
    lua_rawget(lua(), LUA_GLOBALSINDEX);
    string256 S2;
    xr_strcpy(S2, name);
    LPSTR S = S2;
    for (;;)
    {
        if (!xr_strlen(S))
        {
            VERIFY(lua_gettop(lua()) >= 1);
            lua_pop(lua(), 1);
            VERIFY(start == lua_gettop(lua()));
            return false;
        }
        LPSTR S1 = strchr(S, '.');
        if (S1)
            *S1 = 0;
        lua_pushstring(lua(), S);
        lua_rawget(lua(), -2);
        if (lua_isnil(lua(), -1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua()) >= 2);
            lua_pop(lua(), 2);
            VERIFY(start == lua_gettop(lua()));
            return false; // there is no namespace!
        }
        else if (!lua_istable(lua(), -1))
        {
            // lua_settop(lua(), 0);
            VERIFY(lua_gettop(lua()) >= 1);
            lua_pop(lua(), 1);
            VERIFY(start == lua_gettop(lua()));
            FATAL(" Error : the namespace name is already being used by the non-table object!\n");
            return false;
        }
        lua_remove(lua(), -2);
        if (S1)
            S = ++S1;
        else
            break;
    }
    if (!remove_from_stack)
        VERIFY(lua_gettop(lua()) == start + 1);
    else
    {
        VERIFY(lua_gettop(lua()) >= 1);
        lua_pop(lua(), 1);
        VERIFY(lua_gettop(lua()) == start);
    }
    return true;
}

bool CScriptEngine::object(LPCSTR identifier, int type)
{
    int start = lua_gettop(lua());
    lua_pushnil(lua());
    while (lua_next(lua(), -2))
    {
        if (lua_type(lua(), -1) == type && !xr_strcmp(identifier, lua_tostring(lua(), -2)))
        {
            VERIFY(lua_gettop(lua()) >= 3);
            lua_pop(lua(), 3);
            VERIFY(lua_gettop(lua()) == start - 1);
            return true;
        }
        lua_pop(lua(), 1);
    }
    VERIFY(lua_gettop(lua()) >= 1);
    lua_pop(lua(), 1);
    VERIFY(lua_gettop(lua()) == start - 1);
    return false;
}

bool CScriptEngine::object(LPCSTR namespace_name, LPCSTR identifier, int type)
{
    int start = lua_gettop(lua());
    if (xr_strlen(namespace_name) && !namespace_loaded(namespace_name, false))
    {
        VERIFY(lua_gettop(lua()) == start);
        return false;
    }
    bool result = object(identifier, type);
    VERIFY(lua_gettop(lua()) == start);
    return result;
}

luabind::object CScriptEngine::name_space(LPCSTR namespace_name)
{
    string256 S1;
    xr_strcpy(S1, namespace_name);
    LPSTR S = S1;
    luabind::object lua_namespace = luabind::globals(lua());
    for (;;)
    {
        if (!xr_strlen(S))
            return lua_namespace;
        LPSTR I = strchr(S, '.');
        if (!I)
            return lua_namespace[(const char*)S];
        *I = 0;
        lua_namespace = lua_namespace[(const char*)S];
        S = I + 1;
    }
}

struct raii_guard : private Noncopyable
{
    CScriptEngine* m_script_engine;
    int m_error_code;
    const char*& m_error_description;
    bool m_breakIfError;

    raii_guard(CScriptEngine* scriptEngine, int error_code, const char*& error_description, bool breakIfError)
        : m_script_engine(scriptEngine), m_error_code(error_code), m_error_description(error_description),
          m_breakIfError(breakIfError)
    {}

    ~raii_guard()
    {
#ifdef DEBUG
        const bool lua_studio_connected = !!m_script_engine->debugger();
        if (!lua_studio_connected)
#endif
        {
#ifdef DEBUG
            static const bool break_on_assert = !!strstr(Core.Params, "-break_on_assert");
#else
            static const bool break_on_assert = true; //Alundaio: Can't get a proper stack trace with this enabled
#endif
            if (!m_error_code)
                return; // Check "lua_pcall_failed" before changing this!

            if (break_on_assert && m_breakIfError)
                R_ASSERT2(!m_error_code, m_error_description);
            else
                Msg("! SCRIPT ERROR: %s", m_error_description);
        }
    }
};

bool CScriptEngine::print_output(lua_State* L, pcstr caScriptFileName, int errorCode, pcstr caErrorText, bool breakIfError)
{
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    if (errorCode)
        print_error(L, errorCode);
    scriptEngine->print_stack(L);
    pcstr S = "see call_stack for details!";
    raii_guard guard(scriptEngine, errorCode, caErrorText ? caErrorText : S, breakIfError);
    if (!lua_isstring(L, -1))
        return false;
    S = lua_tostring(L, -1);
    if (!xr_strcmp(S, "cannot resume dead coroutine"))
    {
        VERIFY2("Please do not return any values from main!!!", caScriptFileName);
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
        if (debugger() && debugger()->Active())
        {
            debugger()->Write(S);
            debugger()->ErrorBreak();
        }
#endif
    }
    else
    {
        if (!errorCode)
            scriptEngine->script_log(LuaMessageType::Info, "Output from %s", caScriptFileName);
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
        if (debugger() && debugger()->Active())
        {
            debugger()->Write(S);
            debugger()->ErrorBreak();
        }
#endif
    }
    if (caErrorText)
        S = caErrorText;
    return true;
}

void CScriptEngine::print_error(lua_State* L, int iErrorCode)
{
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    switch (iErrorCode)
    {
    case LUA_ERRRUN:
        Log("\n\nSCRIPT RUNTIME ERROR");
        break;
    case LUA_ERRMEM:
        Log("\n\nSCRIPT ERROR (memory allocation)");
        break;
    case LUA_ERRERR:
        Log("\n\nSCRIPT ERROR (while running the error handler function)");
        break;
    case LUA_ERRFILE:
        Log("\n\nSCRIPT ERROR (while running file)");
        break;
    case LUA_ERRSYNTAX:
        Log("\n\nSCRIPT SYNTAX ERROR");
        break;
    case LUA_YIELD:
        Log("\n\nThread is yielded");
        break;
    default: NODEFAULT;
    }
}

void CScriptEngine::flush_log()
{
    string_path log_file_name;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_lua.log");
    FS.update_path(log_file_name, "$logs$", log_file_name);
    m_output.save_to(log_file_name);
}

int CScriptEngine::error_log(LPCSTR format, ...)
{
    va_list marker;
    va_start(marker, format);
    LPCSTR S = "! [LUA][ERROR] ";
    LPSTR S1;
    string4096 S2;
    xr_strcpy(S2, S);
    S1 = S2 + xr_strlen(S);
    int result = vsprintf(S1, format, marker);
    va_end(marker);
    Msg("%s", S2);
    return result;
}

#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
#include "script_debugger.hpp"
#else
#include "LuaStudio/LuaStudio.hpp"
typedef cs::lua_studio::create_world_function_type create_world_function_type;
typedef cs::lua_studio::destroy_world_function_type destroy_world_function_type;
static create_world_function_type s_create_world = nullptr;
static destroy_world_function_type s_destroy_world = nullptr;
static LogCallback s_old_log_callback = nullptr;
#endif
#endif

#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
static void log_callback(void* context, const char* message)
{
    if (s_old_log_callback)
        s_old_log_callback(message);
    CScriptEngine* scriptEngine = (CScriptEngine*)context;
    if (!scriptEngine->debugger())
        return;
    scriptEngine->debugger()->add_log_line(message);
}

void CScriptEngine::initialize_lua_studio(lua_State* state, cs::lua_studio::world*& world, lua_studio_engine*& engine)
{
    engine = 0;
    world = 0;
    u32 const old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

    const auto s_script_debugger_module = XRay::LoadModule(CS_LUA_STUDIO_BACKEND_FILE_NAME);
    SetErrorMode(old_error_mode);
    if (!s_script_debugger_module->IsLoaded())
    {
        Msg("! cannot load %s dynamic library", CS_LUA_STUDIO_BACKEND_FILE_NAME);
        return;
    }

    s_create_world = 
        (create_world_function_type)s_script_debugger_module->GetProcAddress("_cs_lua_studio_backend_create_world@12");
    R_ASSERT2(s_create_world, "can't find function \"cs_lua_studio_backend_create_world\"");

    s_destroy_world = 
        (destroy_world_function_type)s_script_debugger_module->GetProcAddress("_cs_lua_studio_backend_destroy_world@4");
    R_ASSERT2(s_destroy_world, "can't find function \"cs_lua_studio_backend_destroy_world\" in the library");

    engine = new lua_studio_engine();
    world = s_create_world(*engine, false, false);
    VERIFY(world);

    s_old_log_callback = SetLogCB(LogCallback(log_callback, this));
    RunJITCommand(state, "off()");
    world->add(state);
}

void CScriptEngine::finalize_lua_studio(lua_State* state, cs::lua_studio::world*& world, lua_studio_engine*& engine)
{
    world->remove(state);
    VERIFY(world);
    s_destroy_world(world);
    world = nullptr;
    VERIFY(engine);
    xr_delete(engine);
    SetLogCB(s_old_log_callback);
}

void CScriptEngine::try_connect_to_debugger()
{
    if (m_lua_studio_world)
        return;
    initialize_lua_studio(lua(), m_lua_studio_world, m_lua_studio_engine);
}

void CScriptEngine::disconnect_from_debugger()
{
    if (!m_lua_studio_world)
        return;
    finalize_lua_studio(lua(), m_lua_studio_world, m_lua_studio_engine);
}
#endif

CScriptEngine::CScriptEngine(bool is_editor)
{
    luabind::allocator = &luabind_allocator;
    luabind::allocator_context = nullptr;
    m_current_thread = nullptr;
    m_stack_is_ready = false;
    m_virtual_machine = nullptr;
    m_stack_level = 0;
    m_reload_modules = false;
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    static_assert(false, "Do not define USE_LUA_STUDIO macro without USE_DEBUGGER macro");
    m_scriptDebugger = nullptr;
    restartDebugger();
#else
    m_lua_studio_world = nullptr;
    m_lua_studio_engine = nullptr;
#endif
#endif
    m_is_editor = is_editor;
}

CScriptEngine::~CScriptEngine()
{
    if (m_virtual_machine)
        lua_close(m_virtual_machine);
    while (!m_script_processes.empty())
        remove_script_process(m_script_processes.begin()->first);
#if defined(DEBUG) || defined(COC_EDITION)
    flush_log();
#endif
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    xr_delete(m_scriptDebugger);
#else
    disconnect_from_debugger();
#endif
#endif
    if (scriptBuffer)
        xr_free(scriptBuffer);
}

void CScriptEngine::unload()
{
    lua_settop(lua(), m_stack_level);
    m_last_no_file_length = 0;
    *m_last_no_file = 0;
}

void CScriptEngine::onErrorCallback(lua_State* L, pcstr scriptName, int errorCode, pcstr err)
{
    print_output(L, scriptName, errorCode, err);
    on_error(L);

    xrDebug::Fatal(DEBUG_INFO, "LUA error: %s", err);
}

int CScriptEngine::lua_panic(lua_State* L)
{
    onErrorCallback(L, "", LUA_ERRRUN, "PANIC");
    return 0;
}

void CScriptEngine::lua_error(lua_State* L)
{
    pcstr err = lua_tostring(L, -1);
    onErrorCallback(L, "", LUA_ERRRUN, err);
}

int CScriptEngine::lua_pcall_failed(lua_State* L)
{
    const bool isString = lua_isstring(L, -1);
    const pcstr err = isString ? lua_tostring(L, -1) : "";

    onErrorCallback(L, "", LUA_ERRRUN, err);

    if (isString)
        lua_pop(L, 1);
    return LUA_ERRRUN;
}
#if 1 //!XRAY_EXCEPTIONS
void CScriptEngine::lua_cast_failed(lua_State* L, const luabind::type_id& info)
{
    string128 buf;
    xr_sprintf(buf, "LUA error: cannot cast lua value to %s", info.name());
    onErrorCallback(L, "", LUA_ERRRUN, buf);
}
#endif

void CScriptEngine::setup_callbacks()
{
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (debugger())
        debugger()->PrepareLuaBind();
#endif
#endif
#ifdef USE_DEBUGGER
#ifndef USE_LUA_STUDIO
    if (!debugger() || !debugger()->Active())
#endif
#endif
    {
#if !XRAY_EXCEPTIONS
        luabind::set_error_callback(CScriptEngine::lua_error);
#endif

        luabind::set_pcall_callback([](lua_State* L) { lua_pushcfunction(L, CScriptEngine::lua_pcall_failed); });
    }
#if !XRAY_EXCEPTIONS
    luabind::set_cast_failed_callback(CScriptEngine::lua_cast_failed);
#endif
    lua_atpanic(lua(), CScriptEngine::lua_panic);
}

#ifdef DEBUG
#include "script_thread.hpp"

void CScriptEngine::lua_hook_call(lua_State* L, lua_Debug* dbg)
{
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    if (scriptEngine->current_thread())
        scriptEngine->current_thread()->script_hook(L, dbg);
    else
        scriptEngine->m_stack_is_ready = true;
}
#endif

int CScriptEngine::auto_load(lua_State* L)
{
    if (lua_gettop(L) < 2 || !lua_istable(L, 1) || !lua_isstring(L, 2))
    {
        lua_pushnil(L);
        return 1;
    }
    CScriptEngine* scriptEngine = GetInstance(L);
    VERIFY(scriptEngine);
    scriptEngine->process_file_if_exists(lua_tostring(L, 2), false);
    lua_rawget(L, 1);
    return 1;
}

//Alun: Allow directory structuring for scripts
using script_list_type = xr_map<xr_string, xr_string>;
static script_list_type xray_scripts;

void CScriptEngine::setup_auto_load()
{
    luaL_newmetatable(lua(), "XRAY_AutoLoadMetaTable");
    lua_pushstring(lua(), "__index");
    lua_pushcfunction(lua(), CScriptEngine::auto_load);
    lua_settable(lua(), -3);
    lua_pushstring(lua(), GlobalNamespace);
    lua_gettable(lua(), LUA_GLOBALSINDEX);
    luaL_getmetatable(lua(), "XRAY_AutoLoadMetaTable");
    lua_setmetatable(lua(), -2);
    //. ??????????
    // lua_settop(lua(), 0);

    //Alun: Allow directory structuring for scripts
    xray_scripts.clear();

    FS_FileSet fset;
    FS.file_list(fset, "$game_scripts$", FS_ListFiles, "*.script");
    FS_FileSet::iterator fit = fset.begin();
    FS_FileSet::iterator fit_e = fset.end();

    for (; fit != fit_e; ++fit)
    {
        string_path	fn1, fn2;
        _splitpath((*fit).name.c_str(), 0, fn1, fn2, 0);

        FS.update_path(fn1, "$game_scripts$", fn1);
        strconcat(sizeof(fn1), fn1, fn1, fn2, ".script");

        xray_scripts.insert({ xr_string(fn2), xr_string(fn1) });
    }
}

void CScriptEngine::init(ExporterFunc exporterFunc, bool loadGlobalNamespace)
{
#ifdef USE_LUA_STUDIO
    bool lua_studio_connected = !!m_lua_studio_world;
    if (lua_studio_connected)
        m_lua_studio_world->remove(lua());
#endif
    reinit();
    luabind::open(lua());
    // XXX: temporary workaround to preserve backwards compatibility with game scripts
    luabind::disable_super_deprecation();
    luabind::bind_class_info(lua());
    setup_callbacks();
    if (exporterFunc)
        exporterFunc(lua());
    if (std::strstr(Core.Params, "-dump_bindings") && !bindingsDumped)
    {
        bindingsDumped = true;
        static int dumpId = 1;
        string_path filePath;
        xr_sprintf(filePath, "ScriptBindings_%d.txt", dumpId++);
        FS.update_path(filePath, "$app_data_root$", filePath);
        IWriter* writer = FS.w_open(filePath);
        BindingsDumper dumper;
        BindingsDumper::Options options = {};
        options.ShiftWidth = 4;
        options.IgnoreDerived = true;
        options.StripThis = true;
        dumper.Dump(lua(), writer, options);
        FS.w_close(writer);
    }
    // initialize lua standard library functions
    struct luajit
    {
        static void open_lib(lua_State* L, pcstr module_name, lua_CFunction function)
        {
            lua_pushcfunction(L, function);
            lua_pushstring(L, module_name);
            lua_call(L, 1, 0);
        }
    };
    luajit::open_lib(lua(), "", luaopen_base);
    luajit::open_lib(lua(), LUA_LOADLIBNAME, luaopen_package);
    luajit::open_lib(lua(), LUA_TABLIBNAME, luaopen_table);
    luajit::open_lib(lua(), LUA_IOLIBNAME, luaopen_io);
    luajit::open_lib(lua(), LUA_OSLIBNAME, luaopen_os);
    luajit::open_lib(lua(), LUA_MATHLIBNAME, luaopen_math);
    luajit::open_lib(lua(), LUA_STRLIBNAME, luaopen_string);
    luajit::open_lib(lua(), LUA_BITLIBNAME, luaopen_bit);
    luajit::open_lib(lua(), LUA_FFILIBNAME, luaopen_ffi);
#ifdef DEBUG
    luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);
#endif
    if (strstr(Core.Params, "-dev") || strstr(Core.Params, "-dbg"))
        luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);

    // XXX nitrocaster: with vanilla scripts, '-nojit' option requires script profiler to be disabled. The reason
    // is that lua hooks somehow make 'super' global unavailable (is's used all over the vanilla scripts).
    // You can disable script profiler by commenting out the following lines in the beginning of _g.script:
    // if (jit == nil) then
    //     profiler.setup_hook()
    // end
    if (!strstr(Core.Params, "-nojit"))
    {
        luajit::open_lib(lua(), LUA_JITLIBNAME, luaopen_jit);
        RunJITCommand(lua(), "opt.start(2)");
    }
#ifdef USE_LUA_STUDIO
    if (m_lua_studio_world || strstr(Core.Params, "-lua_studio"))
    {
        if (!lua_studio_connected)
            try_connect_to_debugger();
        else
        {
            RunJITCommand(lua(), "off()");
            m_lua_studio_world->add(lua());
        }
    }
#endif
    setup_auto_load();
    m_stack_is_ready = true;

#if defined(DEBUG) && !defined(USE_LUA_STUDIO)
#if defined(USE_DEBUGGER)
    if (!debugger() || !debugger()->Active())
#endif
        lua_sethook(lua(), CScriptEngine::lua_hook_call, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
#endif
    if (loadGlobalNamespace)
    {
        bool save = m_reload_modules;
        m_reload_modules = true;
        process_file_if_exists("_g", false);
        m_reload_modules = save;
    }
    m_stack_level = lua_gettop(lua());
    setvbuf(stderr, g_ca_stdout, _IOFBF, sizeof(g_ca_stdout));
}

void CScriptEngine::remove_script_process(const ScriptProcessor& process_id)
{
    CScriptProcessStorage::iterator I = m_script_processes.find(process_id);
    if (I != m_script_processes.end())
    {
        xr_delete((*I).second);
        m_script_processes.erase(I);
    }
}

bool CScriptEngine::load_file(const char* scriptName, const char* namespaceName)
{
    if (!process_file(scriptName))
        return false;
    string1024 initializerName;
    xr_strcpy(initializerName, scriptName);
    xr_strcat(initializerName, "_initialize");
    if (object(namespaceName, initializerName, LUA_TFUNCTION))
    {
        // lua_dostring(lua(), xr_strcat(initializerName, "()"));
        luabind::functor<void> f;
        R_ASSERT(functor(initializerName, f));
        f();
    }
    return true;
}

bool CScriptEngine::process_file_if_exists(LPCSTR file_name, bool warn_if_not_exist)
{
    if (!*file_name)
        return false;

    if (!m_reload_modules && namespace_loaded(file_name))
        return false;

    script_list_type::iterator it = xray_scripts.find(xr_string(file_name));
    if (it != xray_scripts.end())
    {
        if (Core.ParamFlags.test(Core.verboselog))
            Msg("* loading script %s.script", file_name);
        m_reload_modules = false;

        return load_file_into_namespace(it->second.c_str(), strcmp(file_name, "_g") == 0 ? "_G" : file_name);
    }

    if (warn_if_not_exist)
        Msg("Variable %s not found; No script by this name exists, either.", file_name);

    return false;
}

bool CScriptEngine::process_file(LPCSTR file_name) { return process_file_if_exists(file_name, true); }
bool CScriptEngine::process_file(LPCSTR file_name, bool reload_modules)
{
    m_reload_modules = reload_modules;
    bool result = process_file_if_exists(file_name, true);
    m_reload_modules = false;
    return result;
}

bool CScriptEngine::function_object(LPCSTR function_to_call, luabind::object& object, int type)
{
    if (!xr_strlen(function_to_call))
        return false;

    if (Core.ParamFlags.test(Core.verboselog))
        Msg("lua func call: %s", function_to_call);

    string256 name_space, function;
    parse_script_namespace(function_to_call, name_space, sizeof(name_space), function, sizeof(function));
    if (xr_strcmp(name_space, GlobalNamespace))
    {
        LPSTR file_name = strchr(name_space, '.');
        if (!file_name)
            process_file(name_space);
        else
        {
            *file_name = 0;
            process_file(name_space);
            *file_name = '.';
        }
    }
    if (!this->object(name_space, function, type))
        return false;
    luabind::object lua_namespace = this->name_space(name_space);
    object = lua_namespace[function];
    return true;
}

void CScriptEngine::add_script_process(const ScriptProcessor& process_id, CScriptProcess* script_process)
{
    VERIFY(m_script_processes.find(process_id) == m_script_processes.end());
    m_script_processes.insert(std::make_pair(process_id, script_process));
}

CScriptProcess* CScriptEngine::script_process(const ScriptProcessor& process_id) const
{
    auto it = m_script_processes.find(process_id);
    if (it != m_script_processes.end())
        return it->second;
    return nullptr;
}

void CScriptEngine::parse_script_namespace(const char* name, char* ns, u32 nsSize, char* func, u32 funcSize)
{
    auto p = strrchr(name, '.');
    if (!p)
    {
        xr_strcpy(ns, nsSize, GlobalNamespace);
        p = name - 1;
    }
    else
    {
        VERIFY(u32(p - name + 1) <= nsSize);
        strncpy(ns, name, p - name);
        ns[p - name] = 0;
    }
    xr_strcpy(func, funcSize, p + 1);
}

#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
void CScriptEngine::stopDebugger()
{
    if (debugger())
    {
        xr_delete(m_scriptDebugger);
        Msg("Script debugger stopped.");
    }
    else
        Msg("Script debugger not present.");
}

void CScriptEngine::restartDebugger()
{
    if (debugger())
        stopDebugger();
    m_scriptDebugger = new CScriptDebugger(this);
    debugger()->PrepareLuaBind();
    Msg("Script debugger restarted.");
}
#endif

CScriptEngine* CScriptEngine::GetInstance(lua_State* state)
{
    CScriptEngine* instance = nullptr;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it != stateMap.end())
        instance = it->second;
    stateMapLock.Leave();
    return instance;
}

bool CScriptEngine::RegisterState(lua_State* state, CScriptEngine* scriptEngine)
{
    bool result = false;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it == stateMap.end())
    {
        stateMap.insert({state, scriptEngine});
        result = true;
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::UnregisterState(lua_State* state)
{
    if (!state)
        return true;
    bool result = false;
    stateMapLock.Enter();
    auto it = stateMap.find(state);
    if (it != stateMap.end())
    {
        stateMap.erase(it);
        result = true;
    }
    stateMapLock.Leave();
    return result;
}

bool CScriptEngine::no_file_exists(LPCSTR file_name, u32 string_length)
{
    if (m_last_no_file_length != string_length)
        return false;
    return !memcmp(m_last_no_file, file_name, string_length);
}

void CScriptEngine::add_no_file(LPCSTR file_name, u32 string_length)
{
    m_last_no_file_length = string_length;
    CopyMemory(m_last_no_file, file_name, string_length + 1);
}

void CScriptEngine::collect_all_garbage()
{
    lua_gc(lua(), LUA_GCCOLLECT, 0);
    lua_gc(lua(), LUA_GCCOLLECT, 0);
}

void CScriptEngine::on_error(lua_State* state)
{
    CScriptEngine* scriptEngine = GetInstance(state);
    VERIFY(scriptEngine);
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
    if (!scriptEngine->debugger())
        return;
    scriptEngine->debugger()->on_error(state);
#endif
}

CScriptProcess* CScriptEngine::CreateScriptProcess(shared_str name, shared_str scripts)
{
    return new CScriptProcess(this, name, scripts);
}

CScriptThread* CScriptEngine::CreateScriptThread(LPCSTR caNamespaceName, bool do_string, bool reload)
{
    auto thread = new CScriptThread(this, caNamespaceName, do_string, reload);
    lua_State* threadLua = thread->lua();
    if (threadLua)
        RegisterState(threadLua, this);
    else
        xr_delete(thread);
    return thread;
}

void CScriptEngine::DestroyScriptThread(const CScriptThread* thread)
{
#ifdef DEBUG
    Msg("* Destroying script thread %s", *thread->script_name());
#endif
    try
    {
#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
        if (debugger())
            debugger()->remove(thread->lua());
#endif
#ifndef LUABIND_HAS_BUGS_WITH_LUA_THREADS
        luaL_unref(lua(), LUA_REGISTRYINDEX, thread->thread_reference());
#endif
    }
    catch (...)
    {
    }
    UnregisterState(thread->lua());
}

bool CScriptEngine::is_editor()
{
    return m_is_editor;
}

void CScriptEngine::SetUserdataObjectLoggerFunc(decltype(m_userdataObjectLoggerFunc) in_func)
{
    m_userdataObjectLoggerFunc = in_func;
}
