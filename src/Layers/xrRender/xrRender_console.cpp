#include "stdafx.h"
#pragma hdrstop

#include "xrRender_console.h"
#include "xrCore/xr_token.h"
#include	"../../xrEngine/x_ray.h"

u32 r2_aa_mode = 1;
xr_token r2_aa_mode_token[] = {   
    {"opt_noaa", 1}, 
    {"opt_fxaa", 2}, 
    {"opt_dlaa", 3},
    {0, 0}
};

u32 ps_Preset = 2;
const xr_token qpreset_token[] = {
    {"Minimum", 0},
    {"Low", 1},
    {"Default", 2},
    {"High", 3},
    {"Extreme", 4},
    {nullptr, 0}
};

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    u32 ps_r_ssao_mode = 4;
#else
    u32 ps_r_ssao_mode = 1;
#endif

const xr_token qssao_mode_token[] = {
    {"disabled_ao", 0},
    {"default_ao", 1},
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    {"hdao", 2},
    {"hbao", 3}, 
    {"ssdo", 4},
#endif // USE_DX10
    {nullptr, 0}
};

u32 ps_r_sun_shafts = 2;
const xr_token qsun_shafts_token[] = {
    {"st_opt_off", 0},
    {"st_opt_low", 1},
    {"st_opt_medium", 2},
    {"st_opt_high", 3},
    {nullptr, 0}
};

u32 ps_r_ssao = 3;
const xr_token qssao_token[] = {
    {"st_opt_off", 0},
    {"st_opt_low", 1},
    {"st_opt_medium", 2},
    {"st_opt_high", 3},
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    {"st_opt_ultra", 4},
#endif
    {nullptr, 0}
};

u32 ps_r_sun_quality = 1; // = 0;
const xr_token qsun_quality_token[] = {
    {"st_opt_low", 0},
    {"st_opt_medium", 1},
    {"st_opt_high", 2},
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    {"st_opt_ultra", 3},
    {"st_opt_extreme", 4},
#endif // USE_DX10
    {nullptr, 0}
};

u32 ps_r3_msaa = 0; // = 0;
const xr_token qmsaa_token[] = {
    {"st_opt_off", 0},
    {"2x", 1},
    {"4x", 2},
    //{"8x", 3},
    {nullptr, 0}
};

u32 ps_r3_msaa_atest = 2; // = 0;
const xr_token qmsaa__atest_token[] = {
    {"st_opt_off", 0},
    {"st_opt_atest_msaa_dx10_0", 1},
    {"st_opt_atest_msaa_dx10_1", 2},
    {nullptr, 0}
};

u32 ps_r3_minmax_sm = 3; // = 0;
const xr_token qminmax_sm_token[] = {
    {"off", 0},
    {"on", 1},
    {"auto", 2},
    {"autodetect", 3},
    {nullptr, 0}
};

u32 ps_r2_smap_size = 1536; // 1536 - default
const xr_token qsmap_size_token[] = {
    { "512x512", 512 },
    { "1024x1024", 1024 },
    { "1536x1536", 1536 },
    { "2048x2048", 2048 },
    { "4096x4096", 4096 },
    { "8192x8192", 8192 },
    { nullptr, 0 }
};

// “Off”
// “DX10.0 style [Standard]”
// “DX10.1 style [Higher quality]”

// Common
extern int psSkeletonUpdate;
extern float r__dtex_range;

// int ps_r__Supersample = 1;
int ps_r__LightSleepFrames = 10;

// Цветокоррекция

float ps_rcol = 1;
float ps_gcol = 1;
float ps_bcol = 1;
float ps_saturation = 0;
//float ps_r2_tnmp_exposure   = 7.0f; // r2-only
//float ps_r2_tnmp_gamma      = .25f; // r2-only
//float ps_r2_img_exposure    = 1.0f; // r2-only
//float ps_r2_img_gamma       = 1.0f; // r2-only

// Дождь на худе

float ps_r2_rain_drops_intensity = 0.00025f;
float ps_r2_rain_drops_speed     = 1.25f;
Flags32 ps_r2_rain_drops_flags = {R2FLAG_RAIN_DROPS};

// Шейдер стекла и прочие эффекты

Flags32 ps_r2_hud_mask_flags = {R_FLAG_HUD_MASK | R_FLAG_HUD_DYN_EFFECTS};

// Отключить бампы на r2+

Flags32 ps_r2_use_bump_flags = {R2FLAG_USE_BUMP};

// 

float ps_r__Detail_l_ambient = 0.9f;
float ps_r__Detail_l_aniso = 0.25f;
float ps_r__Detail_density = 0.3f;
float ps_r__Detail_rainbow_hemi = 0.75f;

// Оптимизация

float ps_r2_reflections_distance = 300.0f;

// Вращение древ

float ps_r__Tree_w_rot = 10.0f;
float ps_r__Tree_w_speed = 1.00f;
float ps_r__Tree_w_amp = 0.005f;
Fvector ps_r__Tree_Wave = {.1f, .01f, .11f};
float ps_r__Tree_SBC = 1.5f; // scale bias correct

float ps_r__WallmarkTTL = 50.f;
float ps_r__WallmarkSHIFT = 0.0001f;
float ps_r__WallmarkSHIFT_V = 0.0001f;

float ps_r__GLOD_ssa_start = 256.f;
float ps_r__GLOD_ssa_end = 64.f;
float ps_r__LOD = 0.75f;
// float ps_r__LOD_Power = 1.5f;
float ps_r__ssaDISCARD = 3.5f; // RO
float ps_r__ssaDONTSORT = 32.f; // RO
float ps_r__ssaHZBvsTEX = 96.f; // RO

int ps_r__tf_Anisotropic = 8;

// R1
float ps_r1_ssaLOD_A = 64.f;
float ps_r1_ssaLOD_B = 48.f;
float ps_r1_tf_Mipbias = 0.0f;
Flags32 ps_r1_flags = {R1FLAG_DLIGHTS}; // r1-only
float ps_r1_lmodel_lerp = 0.1f;
float ps_r1_dlights_clip = 40.f;
float ps_r1_pps_u = 0.f;
float ps_r1_pps_v = 0.f;

// R1-specific
int ps_r1_GlowsPerFrame = 16; // r1-only
float ps_r1_fog_luminance = 1.1f; // r1-only
int ps_r1_SoftwareSkinning = 0; // r1-only

// R2
float ps_r2_ssaLOD_A = 64.f;
float ps_r2_ssaLOD_B = 48.f;
float ps_r2_tf_Mipbias = 0.0f;

extern ENGINE_API Fvector4 ps_ssfx_rain_1;
extern ENGINE_API Fvector4 ps_ssfx_rain_2;
extern ENGINE_API Fvector4 ps_ssfx_rain_3;

// R2-specific
Flags32 ps_r2_ls_flags = {R2FLAG_SUN
    //| R2FLAG_SUN_IGNORE_PORTALS
    | R2FLAG_EXP_DONT_TEST_UNSHADOWED | R2FLAG_USE_NVSTENCIL | R2FLAG_EXP_SPLIT_SCENE | R2FLAG_EXP_MT_CALC |
    R3FLAG_DYN_WET_SURF | R3FLAG_VOLUMETRIC_SMOKE
    //| R3FLAG_MSAA
    | R3FLAG_MSAA_OPT
    | R3FLAG_GBUFFER_OPT | R2FLAG_DETAIL_BUMP | R2FLAG_DOF | R2FLAG_SOFT_PARTICLES | R2FLAG_SOFT_WATER |
    R2FLAG_STEEP_PARALLAX | R2FLAG_SUN_FOCUS | R2FLAG_SUN_TSM | R2FLAG_TONEMAP | R2FLAG_VOLUMETRIC_LIGHTS}; // r2-only

Flags32 ps_r2_ls_flags_ext = {
    /*R2FLAGEXT_SSAO_OPT_DATA |*/ R2FLAGEXT_SSAO_HALF_DATA | R2FLAGEXT_ENABLE_TESSELLATION};

BOOL ps_clear_models_on_unload = 0; // Alundaio
BOOL ps_use_precompiled_shaders = 0; // Alundaio
BOOL ps_grass_shadow = 0; // Alundaio

float ps_r2_gloss_factor = 10.0f;
float ps_r2_gloss_min = 0.0f;

float ps_r2_df_parallax_h = 0.02f;
float ps_r2_df_parallax_range = 75.f;
float ps_r2_tonemap_middlegray = 0.5f; // r2-only
float ps_r2_tonemap_adaptation = 1.f; // r2-only
float ps_r2_tonemap_low_lum = 0.1f; // r2-only
float ps_r2_tonemap_amount = 0.7f; // r2-only
float ps_r2_ls_bloom_kernel_g = 3.f; // r2-only
float ps_r2_ls_bloom_kernel_b = .7f; // r2-only
float ps_r2_ls_bloom_speed = 100.f; // r2-only
float ps_r2_ls_bloom_kernel_scale = .7f; // r2-only // gauss
float ps_r2_ls_dsm_kernel = .7f; // r2-only
float ps_r2_ls_psm_kernel = .7f; // r2-only
float ps_r2_ls_ssm_kernel = .7f; // r2-only
float ps_r2_ls_bloom_threshold = .00001f; // r2-only
float ps_r2_mblur = .1f; // .5f
int ps_r2_GI_depth = 1; // 1..5
int ps_r2_GI_photons = 16; // 8..64
float ps_r2_GI_clip = EPS_L; // EPS
float ps_r2_GI_refl = .9f; // .9f
float ps_r2_ls_depth_scale = 1.00001f; // 1.00001f
float ps_r2_ls_depth_bias = -0.0003f; // -0.0001f
float ps_r2_ls_squality = 1.0f; // 1.00f
float ps_r2_sun_tsm_projection = 0.3f; // 0.18f
float ps_r2_sun_tsm_bias = -0.01f; //
float ps_r2_sun_near = 20.f; // 12.0f

extern float OLES_SUN_LIMIT_27_01_07; //    actually sun_far

float ps_r2_sun_near_border = 0.75f; // 1.0f
float ps_r2_sun_depth_far_scale = 1.00000f; // 1.00001f
float ps_r2_sun_depth_far_bias = -0.00002f; // -0.0000f
float ps_r2_sun_depth_near_scale = 1.0000f; // 1.00001f
float ps_r2_sun_depth_near_bias = 0.00001f; // -0.00005f
float ps_r2_sun_lumscale = 1.0f; // 1.0f
float ps_r2_sun_lumscale_hemi = 1.0f; // 1.0f
float ps_r2_sun_lumscale_amb = 1.0f;
float ps_r2_gmaterial = 2.2f; //
float ps_r2_zfill = 0.25f; // .1f

float ps_r2_dhemi_sky_scale = 0.08f; // 1.5f
float ps_r2_dhemi_light_scale = 0.2f;
float ps_r2_dhemi_light_flow = 0.1f;
int ps_r2_dhemi_count = 5; // 5
int ps_r2_wait_sleep = 0;

float ps_r2_lt_smooth = 1.f; // 1.f
float ps_r2_slight_fade = 0.5f; // 1.f

// Laser
Fvector4 ps_shader_wpn_laser = {.0f, .0f, .0f, .0f};

// debug
Fvector4 ps_pp_bloom_thresh = {.7, .8f, .9f, .0f};
Fvector4 ps_pp_bloom_weight = {.33f, .33f, .33f, .0f};

Fvector4 ps_dev_param_1 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_2 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_3 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_4 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_5 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_6 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_7 = {.0f, .0f, .0f, .0f};
Fvector4 ps_dev_param_8 = {.0f, .0f, .0f, .0f};

//  x - min (0), y - focus (1.4), z - max (100)
Fvector3 ps_r2_dof = Fvector3().set(-1.25f, 1.4f, 600.f);
float ps_r2_dof_sky = 30; //    distance to sky
float ps_r2_dof_kernel_size = 5.0f; //  7.0f

float ps_r3_dyn_wet_surf_near = 10.f; // 10.0f
float ps_r3_dyn_wet_surf_far = 30.f; // 30.0f
int ps_r3_dyn_wet_surf_sm_res = 256; // 256

// AVO: detail draw radius
Flags32 ps_common_flags = {0}; // r1-only
u32 ps_steep_parallax = 0;
int ps_r__detail_radius = 49;
Ivector ps_r2_details_opt = {60, 90, 120};

#ifdef DETAIL_RADIUS // управление радиусом отрисовки травы
u32 dm_size = 24;
u32 dm_cache1_line = 12; // dm_size*2/dm_cache1_count
u32 dm_cache_line = 49; // dm_size+1+dm_size
u32 dm_cache_size = 2401; // dm_cache_line*dm_cache_line
float dm_fade = 47.5; // float(2*dm_size)-.5f;
u32 dm_current_size = 24;
u32 dm_current_cache1_line = 12; // dm_current_size*2/dm_cache1_count
u32 dm_current_cache_line = 49; // dm_current_size+1+dm_current_size
u32 dm_current_cache_size = 2401; // dm_current_cache_line*dm_current_cache_line
float dm_current_fade = 47.5; // float(2*dm_current_size)-.5f;
#endif
float ps_current_detail_density = 0.6f;
float ps_current_detail_scale = 1.f;
xr_token ext_quality_token[] = {
    {"qt_off", 0}, {"qt_low", 1}, {"qt_medium", 2}, {"qt_high", 3}, {"qt_extreme", 4}, {nullptr, 0}};
//-AVO

Flags32 ps_actor_shadow_flags = {0};

#ifndef _EDITOR
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"

#if defined(USE_DX10) || defined(USE_DX11)
#include "Layers/xrRenderDX10/StateManager/dx10SamplerStateCache.h"
#endif // USE_DX10

//-----------------------------------------------------------------------

// AVO: detail draw radius
#ifdef DETAIL_RADIUS
class CCC_detail_radius : public CCC_Integer
{
public:
    void apply()
    {
        dm_current_size = iFloor((float)ps_r__detail_radius / 4) * 2;
        dm_current_cache1_line = dm_current_size * 2 / 4; // assuming cache1_count = 4
        dm_current_cache_line = dm_current_size + 1 + dm_current_size;
        dm_current_cache_size = dm_current_cache_line * dm_current_cache_line;
        dm_current_fade = float(2 * dm_current_size) - .5f;
    }

    CCC_detail_radius(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};

    void Execute(LPCSTR args) override
    {
        CCC_Integer::Execute(args);
        apply();
    }

    void Status(TStatus& S) override { CCC_Integer::Status(S); }
};

//-AVO
#endif

class CCC_DetailsOpt : public CCC_IVector3
{
public:
    CCC_DetailsOpt(LPCSTR N, Ivector* V, const Ivector _min, const Ivector _max) : CCC_IVector3(N, V, _min, _max) {}
    virtual void Execute(LPCSTR args)
    {
        Ivector v;
        if ((3 != sscanf(args, "%d,%d,%d", &v.x, &v.y, &v.z)) && (3 != sscanf(args, "(%d, %d, %d)", &v.x, &v.y, &v.z)))
        {
            InvalidSyntax();
        }
        else if (v.x > v.y || v.y > v.z)
        {
            InvalidSyntax();
        }
        else
        {
            CCC_IVector3::Execute(args);
        }
    }
};

class CCC_tf_Aniso : public CCC_Integer
{
public:
    void apply()
    {
        if (nullptr == HW.pDevice)
            return;
        int val = *value;
        clamp(val, 1, 16);
#if defined(USE_OGL)
        // TODO: OGL: Implement aniso filtering.
#elif defined(USE_DX10) || defined(USE_DX11)
        SSManager.SetMaxAnisotropy(val);
#else
        for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
            CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, val));
#endif // USE_DX10
    }
    CCC_tf_Aniso(LPCSTR N, int* v) : CCC_Integer(N, v, 1, 16){};
    virtual void Execute(LPCSTR args)
    {
        CCC_Integer::Execute(args);
        apply();
    }
    virtual void Status(TStatus& S)
    {
        CCC_Integer::Status(S);
        apply();
    }
};
class CCC_tf_MipBias : public CCC_Float
{
public:
    void apply()
    {
        if (nullptr == HW.pDevice)
            return;

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
// TODO: DX10: Implement mip bias control
// VERIFY(!"apply not implmemented.");
#else // USE_DX10
        for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
            CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)value)));
#endif // USE_DX10
    }

    CCC_tf_MipBias(LPCSTR N, float* v) : CCC_Float(N, v, -3.f, +3.f) {}
    virtual void Execute(LPCSTR args)
    {
        CCC_Float::Execute(args);
        apply();
    }
    virtual void Status(TStatus& S)
    {
        CCC_Float::Status(S);
        apply();
    }
};
class CCC_R2GM : public CCC_Float
{
public:
    CCC_R2GM(LPCSTR N, float* v) : CCC_Float(N, v, 0.f, 4.f) { *v = 0; };
    virtual void Execute(LPCSTR args)
    {
        if (0 == xr_strcmp(args, "on"))
        {
            ps_r2_ls_flags.set(R2FLAG_GLOBALMATERIAL, TRUE);
        }
        else if (0 == xr_strcmp(args, "off"))
        {
            ps_r2_ls_flags.set(R2FLAG_GLOBALMATERIAL, FALSE);
        }
        else
        {
            CCC_Float::Execute(args);
            if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))
            {
                static LPCSTR name[4] = {"oren", "blin", "phong", "metal"};
                float mid = *value;
                int m0 = iFloor(mid) % 4;
                int m1 = (m0 + 1) % 4;
                float frc = mid - float(iFloor(mid));
                Msg("* material set to [%s]-[%s], with lerp of [%f]", name[m0], name[m1], frc);
            }
        }
    }
};
class CCC_Screenshot : public IConsole_Command
{
public:
    CCC_Screenshot(LPCSTR N) : IConsole_Command(N){};
    virtual void Execute(LPCSTR args)
    {
        if (GEnv.isDedicatedServer)
            return;

        string_path name;
        name[0] = 0;
        sscanf(args, "%s", name);
        LPCSTR image = xr_strlen(name) ? name : 0;
        GEnv.Render->Screenshot(IRender::SM_NORMAL, image);
    }
};

class CCC_RestoreQuadIBData : public IConsole_Command
{
public:
    CCC_RestoreQuadIBData(LPCSTR N) : IConsole_Command(N){};
    virtual void Execute(LPCSTR /*args*/) { RCache.RestoreQuadIBData(); }
};

class CCC_ModelPoolStat : public IConsole_Command
{
public:
    CCC_ModelPoolStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR /*args*/) { RImplementation.Models->dump(); }
};

class CCC_SSAO_Mode : public CCC_Token
{
public:
    CCC_SSAO_Mode(LPCSTR N, u32* V, const xr_token* T) : CCC_Token(N, V, T){};

    virtual void Execute(LPCSTR args)
    {
        CCC_Token::Execute(args);

        switch (*value)
        {
        case 0:
        {
            ps_r_ssao = 0;
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
            break;
        }
        case 1:
        {
            if (ps_r_ssao == 0)
            {
                ps_r_ssao = 1;
            }
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HALF_DATA, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
            break;
        }
        case 2:
        {
            if (ps_r_ssao == 0)
            {
                ps_r_ssao = 1;
            }
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 1);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HALF_DATA, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
            break;
        }
        case 3:
        {
            if (ps_r_ssao == 0)
            {
                ps_r_ssao = 1;
            }
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 1);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 1);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
            break;
        }
        case 4: {
            if (ps_r_ssao == 0)
            {
                ps_r_ssao = 1;
            }
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 1);
            ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 1);
            break;
        }
        }
    }
};

//-----------------------------------------------------------------------
class CCC_Preset : public CCC_Token
{
public:
    CCC_Preset(LPCSTR N, u32* V, const xr_token* T) : CCC_Token(N, V, T){};

    virtual void Execute(LPCSTR args)
    {
        CCC_Token::Execute(args);
        string_path _cfg;
        string_path cmd;

        switch (*value)
        {
        case 0: xr_strcpy(_cfg, "rspec_minimum.ltx"); break;
        case 1: xr_strcpy(_cfg, "rspec_low.ltx"); break;
        case 2: xr_strcpy(_cfg, "rspec_default.ltx"); break;
        case 3: xr_strcpy(_cfg, "rspec_high.ltx"); break;
        case 4: xr_strcpy(_cfg, "rspec_extreme.ltx"); break;
        }
        FS.update_path(_cfg, "$game_config$", _cfg);
        strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
        Console->Execute(cmd);
    }
};

class CCC_memory_stats : public IConsole_Command
{
public:
    CCC_memory_stats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR /*args*/)
    {
        // TODO: OGL: Implement memory usage statistics.
#ifndef USE_OGL
        u32 m_base = 0;
        u32 c_base = 0;
        u32 m_lmaps = 0;
        u32 c_lmaps = 0;

        RImplementation.ResourcesGetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);

        Msg("memory usage  mb \t \t video    \t managed      \t system \n");

        const float MiB = 1024 * 1024; // XXX: use it as common enum value (like in X-Ray 2.0)
        const u32* mem_usage = HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex];

        float vb_video = mem_usage[D3DPOOL_DEFAULT] / MiB;
        float vb_managed = mem_usage[D3DPOOL_MANAGED] / MiB;
        float vb_system = mem_usage[D3DPOOL_SYSTEMMEM] / MiB;
        Msg("vertex buffer      \t \t %f \t %f \t %f ", vb_video, vb_managed, vb_system);

        float ib_video = mem_usage[D3DPOOL_DEFAULT] / MiB;
        float ib_managed = mem_usage[D3DPOOL_MANAGED] / MiB;
        float ib_system = mem_usage[D3DPOOL_SYSTEMMEM] / MiB;
        Msg("index buffer      \t \t %f \t %f \t %f ", ib_video, ib_managed, ib_system);

        float textures_managed = (m_base + m_lmaps) / MiB;
        Msg("textures          \t \t %f \t %f \t %f ", 0.f, textures_managed, 0.f);

        mem_usage = HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget];
        float rt_video = mem_usage[D3DPOOL_DEFAULT] / MiB;
        float rt_managed = mem_usage[D3DPOOL_MANAGED] / MiB;
        float rt_system = mem_usage[D3DPOOL_SYSTEMMEM] / MiB;
        Msg("R-Targets         \t \t %f \t %f \t %f ", rt_video, rt_managed, rt_system);

        Msg("\nTotal             \t \t %f \t %f \t %f ", vb_video + ib_video + rt_video,
            textures_managed + vb_managed + ib_managed + rt_managed, vb_system + ib_system + rt_system);
#endif // USE_OGL
    }
};

#if RENDER != R_R1 && RENDER != R_GL
#include "r__pixel_calculator.h"
class CCC_BuildSSA : public IConsole_Command
{
public:
    CCC_BuildSSA(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR /*args*/)
    {
#if !defined(USE_DX10) && !defined(USE_DX11)
        //  TODO: DX10: Implement pixel calculator
        r_pixel_calculator c;
        c.run();
#endif //   USE_DX10
    }
};
#endif

class CCC_DofFar : public CCC_Float
{
public:
    CCC_DofFar(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max) {}
    virtual void Execute(LPCSTR args)
    {
        float v = float(atof(args));

        if (v < ps_r2_dof.y + 0.1f)
        {
            char pBuf[256];
            _snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value greater or equal to r2_dof_focus+0.1");
            Msg("~ Invalid syntax in call to '%s'", cName);
            Msg("~ Valid arguments: %s", pBuf);
            Console->Execute("r2_dof_focus");
        }
        else
        {
            CCC_Float::Execute(args);
            if (g_pGamePersistent)
                g_pGamePersistent->SetBaseDof(ps_r2_dof);
        }
    }

    //  CCC_Dof should save all data as well as load from config
    virtual void Save(IWriter* /*F*/) { ; }
};

class CCC_DofNear : public CCC_Float
{
public:
    CCC_DofNear(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max) {}
    virtual void Execute(LPCSTR args)
    {
        float v = float(atof(args));

        if (v > ps_r2_dof.y - 0.1f)
        {
            char pBuf[256];
            _snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value less or equal to r2_dof_focus-0.1");
            Msg("~ Invalid syntax in call to '%s'", cName);
            Msg("~ Valid arguments: %s", pBuf);
            Console->Execute("r2_dof_focus");
        }
        else
        {
            CCC_Float::Execute(args);
            if (g_pGamePersistent)
                g_pGamePersistent->SetBaseDof(ps_r2_dof);
        }
    }

    // CCC_Dof should save all data as well as load from config
    virtual void Save(IWriter* /*F*/) { ; }
};

class CCC_DofFocus : public CCC_Float
{
public:
    CCC_DofFocus(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max) {}
    virtual void Execute(LPCSTR args)
    {
        float v = float(atof(args));

        if (v > ps_r2_dof.z - 0.1f)
        {
            char pBuf[256];
            _snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value less or equal to r2_dof_far-0.1");
            Msg("~ Invalid syntax in call to '%s'", cName);
            Msg("~ Valid arguments: %s", pBuf);
            Console->Execute("r2_dof_far");
        }
        else if (v < ps_r2_dof.x + 0.1f)
        {
            char pBuf[256];
            _snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value greater or equal to r2_dof_far-0.1");
            Msg("~ Invalid syntax in call to '%s'", cName);
            Msg("~ Valid arguments: %s", pBuf);
            Console->Execute("r2_dof_near");
        }
        else
        {
            CCC_Float::Execute(args);
            if (g_pGamePersistent)
                g_pGamePersistent->SetBaseDof(ps_r2_dof);
        }
    }

    //  CCC_Dof should save all data as well as load from config
    virtual void Save(IWriter* /*F*/) { ; }
};

class CCC_Dof : public CCC_Vector3
{
public:
    CCC_Dof(LPCSTR N, Fvector* V, const Fvector _min, const Fvector _max) : CCC_Vector3(N, V, _min, _max) { ; }
    virtual void Execute(LPCSTR args)
    {
        Fvector v;
        if (3 != sscanf(args, "%f,%f,%f", &v.x, &v.y, &v.z))
            InvalidSyntax();
        else if ((v.x > v.y - 0.1f) || (v.z < v.y + 0.1f))
        {
            InvalidSyntax();
            Msg("x <= y - 0.1");
            Msg("y <= z - 0.1");
        }
        else
        {
            CCC_Vector3::Execute(args);
            if (g_pGamePersistent)
                g_pGamePersistent->SetBaseDof(ps_r2_dof);
        }
    }
    virtual void Status(TStatus& S) { xr_sprintf(S, "%f,%f,%f", value->x, value->y, value->z); }
    virtual void Info(TInfo& I)
    {
        xr_sprintf(I, "vector3 in range [%f,%f,%f]-[%f,%f,%f]", min.x, min.y, min.z, max.x, max.y, max.z);
    }
};

class CCC_Vector4 : public IConsole_Command
{
protected:
    Fvector4* value;
    Fvector4 min, max;

public:
    CCC_Vector4(LPCSTR N, Fvector4* V, const Fvector4 _min, const Fvector4 _max) : IConsole_Command(N), value(V)
    {
        min.set(_min);
        max.set(_max);
    };
    const Fvector4 GetValue() const { return *value; };
    Fvector4* GetValuePtr() const { return value; };

    virtual void Execute(LPCSTR args)
    {
        Fvector4 v;
        if (4 != sscanf(args, "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w))
        {
            if (4 != sscanf(args, "(%f,%f,%f,%f)", &v.x, &v.y, &v.z, &v.w))
            {
                InvalidSyntax();
                return;
            }
        }

        if (v.x < min.x || v.y < min.y || v.z < min.z || v.w < min.w)
        {
            InvalidSyntax();
            return;
        }
        if (v.x > max.x || v.y > max.y || v.z > max.z || v.w > max.w)
        {
            InvalidSyntax();
            return;
        }
        value->set(v);
    }

    virtual void Status(TStatus& S)
    {
        xr_sprintf(S, sizeof(S), "(%f, %f, %f, %f)", value->x, value->y, value->z, value->w);
    }

    virtual void Info(TInfo& I)
    {
        xr_sprintf(I, sizeof(I), "vector4 in range [%e,%e,%e,%e]-[%e,%e,%e,%e]", min.x, min.y, min.z, min.w, max.x,
            max.y, max.z, max.w);
    }

    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        TStatus str;
        xr_sprintf(str, sizeof(str), "(%e, %e, %e, %e) (current) [(%e,%e,%e,%e)-(%e,%e,%e,%e)]", value->x, value->y,
            value->z, value->w, min.x, min.y, min.z, min.w, max.x, max.y, max.z, max.w);
        tips.push_back(str);
        IConsole_Command::fill_tips(tips, mode);
    }
};

class CCC_DumpResources : public IConsole_Command
{
public:
    CCC_DumpResources(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR /*args*/)
    {
        RImplementation.Models->dump();
        RImplementation.Resources->Dump(false);
    }
};

class CCC_SunshaftsIntensity : public CCC_Float
{
public:
    CCC_SunshaftsIntensity(LPCSTR N, float* V, float _min, float _max) : CCC_Float(N, V, _min, _max) {}
    virtual void Save(IWriter*) { ; }
};

//  Allow real-time fog config reload
#if (RENDER == R_R3) || (RENDER == R_R4)
#ifdef DEBUG

#include "Layers/xrRenderDX10/3DFluid/dx103DFluidManager.h"

class CCC_Fog_Reload : public IConsole_Command
{
public:
    CCC_Fog_Reload(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR /*args*/) { FluidManager.UpdateProfiles(); }
};
#endif // DEBUG
#endif // (RENDER == R_R3) || (RENDER == R_R4)

//-----------------------------------------------------------------------
void xrRender_initconsole()
{

	Fvector4 twb_min = {0.f, 0.f, 0.f, 0.f};
    Fvector4 twb_max = {1.f, 1.f, 1.f, 1.f};
    CMD4(CCC_Vector4, "r__bloom_weight", &ps_pp_bloom_weight, twb_min, twb_max);
    CMD4(CCC_Vector4, "r__bloom_thresh", &ps_pp_bloom_thresh, twb_min, twb_max);

    Fvector4 tw2_min = {-100.f, -100.f, -100.f, -100.f};
    Fvector4 tw2_max = {100.f, 100.f, 100.f, 100.f};

    // Laser
    //CMD4(CCC_Vector4, "wpn_laser", &ps_shader_wpn_laser, tw2_min, tw2_max);

    // Debug
	CMD4(CCC_Vector4, "shader_param_1", &ps_dev_param_1, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_2", &ps_dev_param_2, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_3", &ps_dev_param_3, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_4", &ps_dev_param_4, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_5", &ps_dev_param_5, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_6", &ps_dev_param_6, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_7", &ps_dev_param_7, tw2_min, tw2_max);
    CMD4(CCC_Vector4, "shader_param_8", &ps_dev_param_8, tw2_min, tw2_max);

	// Screen Space Shaders
    CMD4(CCC_Vector4, "ssfx_wpn_dof_1", &ps_ssfx_wpn_dof_1, tw2_min, tw2_max);
    CMD4(CCC_Float, "ssfx_wpn_dof_2", &ps_ssfx_wpn_dof_2, 0.0f, 1.0f);

    CMD3(CCC_Mask, "r3_new_ssfx_rain", &ps_r2_ls_flags_ext, R4FLAGEXT_NEW_RAIN);
    CMD4(CCC_Vector4, "ssfx_rain_1", &ps_ssfx_rain_1, Fvector4().set(0, 0, 0, 0), Fvector4().set(10, 5, 5, 2));
    CMD4(CCC_Vector4, "ssfx_rain_2", &ps_ssfx_rain_2, Fvector4().set(0, 0, 0, 0), Fvector4().set(1, 10, 10, 10));
    CMD4(CCC_Vector4, "ssfx_rain_3", &ps_ssfx_rain_3, Fvector4().set(0, 0, 0, 0), Fvector4().set(1, 10, 10, 10));

    CMD3(CCC_Preset, "_preset", &ps_Preset, qpreset_token);

    CMD4(CCC_Integer, "rs_skeleton_update", &psSkeletonUpdate, 2, 128);
#ifdef DEBUG
    CMD1(CCC_DumpResources, "dump_resources");
#endif // DEBUG

    CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175);

    // Common
    CMD1(CCC_Screenshot, "screenshot");

    //  Igor: just to test bug with rain/particles corruption
    CMD1(CCC_RestoreQuadIBData, "r_restore_quad_ib_data");
#ifdef DEBUG
#if RENDER != R_R1 && RENDER != R_GL
    CMD1(CCC_BuildSSA, "build_ssa");
#endif
    CMD4(CCC_Integer, "r__lsleep_frames", &ps_r__LightSleepFrames, 4, 30);
    CMD4(CCC_Float, "r__ssa_glod_start", &ps_r__GLOD_ssa_start, 128, 512);
    CMD4(CCC_Float, "r__ssa_glod_end", &ps_r__GLOD_ssa_end, 16, 96);
    CMD4(CCC_Float, "r__wallmark_shift_pp", &ps_r__WallmarkSHIFT, 0.0f, 1.f);
    CMD4(CCC_Float, "r__wallmark_shift_v", &ps_r__WallmarkSHIFT_V, 0.0f, 1.f);
    CMD1(CCC_ModelPoolStat, "stat_models");
#endif // DEBUG
    CMD4(CCC_Float, "r__wallmark_ttl", &ps_r__WallmarkTTL, 1.0f, 10.f * 60.f);

    CMD4(CCC_Integer, "r__supersample", &ps_r__Supersample, 1, 8);

    Fvector tw_min, tw_max;

    CMD4(CCC_Float, "r__geometry_lod", &ps_r__LOD, 0.1f, 1.2f);
    //CMD4(CCC_Float, "r__geometry_lod_pow", &ps_r__LOD_Power, 0, 2);

    CMD4(CCC_Float, "r__detail_density", &ps_current_detail_density, 0.3f, 1.0f); //AVO: extended from 0.2f to 0.04f and replaced variable
    CMD4(CCC_Float, "r__detail_scale", &ps_current_detail_scale, 0.2f, 1.6f);

#ifdef DEBUG
    CMD4(CCC_Float, "r__detail_l_ambient", &ps_r__Detail_l_ambient, .5f, .95f);
    CMD4(CCC_Float, "r__detail_l_aniso", &ps_r__Detail_l_aniso, .1f, .5f);

    CMD4(CCC_Float, "r__d_tree_w_amp", &ps_r__Tree_w_amp, .001f, 1.f);
    CMD4(CCC_Float, "r__d_tree_w_rot", &ps_r__Tree_w_rot, .01f, 100.f);
    CMD4(CCC_Float, "r__d_tree_w_speed", &ps_r__Tree_w_speed, 1.0f, 10.f);

    tw_min.set(EPS, EPS, EPS);
    tw_max.set(2, 2, 2);
    CMD4(CCC_Vector3, "r__d_tree_wave", &ps_r__Tree_Wave, tw_min, tw_max);
#endif // DEBUG

    CMD2(CCC_tf_Aniso, "r__tf_aniso", &ps_r__tf_Anisotropic); // {1..16}

    // R1
    CMD4(CCC_Float, "r1_ssa_lod_a", &ps_r1_ssaLOD_A, 16, 96);
    CMD4(CCC_Float, "r1_ssa_lod_b", &ps_r1_ssaLOD_B, 16, 64);
    CMD4(CCC_Float, "r1_lmodel_lerp", &ps_r1_lmodel_lerp, 0, 0.333f);
    CMD2(CCC_tf_MipBias, "r1_tf_mipbias", &ps_r1_tf_Mipbias); // {-3 +3}
    CMD3(CCC_Mask, "r1_dlights", &ps_r1_flags, R1FLAG_DLIGHTS);
    CMD4(CCC_Float, "r1_dlights_clip", &ps_r1_dlights_clip, 10.f, 150.f);
    CMD4(CCC_Float, "r1_pps_u", &ps_r1_pps_u, -1.f, +1.f);
    CMD4(CCC_Float, "r1_pps_v", &ps_r1_pps_v, -1.f, +1.f);

    // R1-specific
    CMD4(CCC_Integer, "r1_glows_per_frame", &ps_r1_GlowsPerFrame, 2, 32);
    CMD3(CCC_Mask, "r1_detail_textures", &ps_r2_ls_flags, R1FLAG_DETAIL_TEXTURES);

    CMD4(CCC_Float, "r1_fog_luminance", &ps_r1_fog_luminance, 0.2f, 5.f);

    // Software Skinning
    // 0 - disabled (renderer can override)
    // 1 - enabled
    // 2 - forced hardware skinning (renderer can not override)
    CMD4(CCC_Integer, "r1_software_skinning", &ps_r1_SoftwareSkinning, 0, 2);

    // R2
    CMD4(CCC_Float, "r2_ssa_lod_a", &ps_r2_ssaLOD_A, 16, 96);
    CMD4(CCC_Float, "r2_ssa_lod_b", &ps_r2_ssaLOD_B, 32, 64);
    CMD2(CCC_tf_MipBias, "r2_tf_mipbias", &ps_r2_tf_Mipbias);

    // R2-specific
    CMD2(CCC_R2GM, "r2em", &ps_r2_gmaterial);
    CMD3(CCC_Mask, "r2_tonemap", &ps_r2_ls_flags, R2FLAG_TONEMAP);
    CMD4(CCC_Float, "r2_tonemap_middlegray", &ps_r2_tonemap_middlegray, 0.0f, 2.0f);
    CMD4(CCC_Float, "r2_tonemap_adaptation", &ps_r2_tonemap_adaptation, 0.01f, 10.0f);
    CMD4(CCC_Float, "r2_tonemap_lowlum", &ps_r2_tonemap_low_lum, 0.0001f, 1.0f);
    CMD4(CCC_Float, "r2_tonemap_amount", &ps_r2_tonemap_amount, 0.0000f, 1.0f);
    CMD4(CCC_Float, "r2_ls_bloom_kernel_scale", &ps_r2_ls_bloom_kernel_scale, 0.5f, 2.f);
    CMD4(CCC_Float, "r2_ls_bloom_kernel_g", &ps_r2_ls_bloom_kernel_g, 1.f, 7.f);
    CMD4(CCC_Float, "r2_ls_bloom_kernel_b", &ps_r2_ls_bloom_kernel_b, 0.01f, 1.f);
    CMD4(CCC_Float, "r2_ls_bloom_threshold", &ps_r2_ls_bloom_threshold, 0.f, 1.f);
    CMD4(CCC_Float, "r2_ls_bloom_speed", &ps_r2_ls_bloom_speed, 0.f, 100.f);
    CMD3(CCC_Mask, "r2_ls_bloom_fast", &ps_r2_ls_flags, R2FLAG_FASTBLOOM);
    CMD4(CCC_Float, "r2_ls_dsm_kernel", &ps_r2_ls_dsm_kernel, .1f, 3.f);
    CMD4(CCC_Float, "r2_ls_psm_kernel", &ps_r2_ls_psm_kernel, .1f, 3.f);
    CMD4(CCC_Float, "r2_ls_ssm_kernel", &ps_r2_ls_ssm_kernel, .1f, 3.f);
    CMD4(CCC_Float, "r2_ls_squality", &ps_r2_ls_squality, .5f, 1.f);

    CMD3(CCC_Mask, "r2_zfill", &ps_r2_ls_flags, R2FLAG_ZFILL);
    CMD4(CCC_Float, "r2_zfill_depth", &ps_r2_zfill, .001f, .5f);
    CMD3(CCC_Mask, "r2_allow_r1_lights", &ps_r2_ls_flags, R2FLAG_R1LIGHTS);

    CMD3(CCC_Mask, "r__actor_shadow", &ps_actor_shadow_flags, RFLAG_ACTOR_SHADOW); // Swartz

#ifdef DEBUG
    CMD3(CCC_Mask, "r2_use_nvdbt", &ps_r2_ls_flags, R2FLAG_USE_NVDBT);
    CMD3(CCC_Mask, "r2_mt", &ps_r2_ls_flags, R2FLAG_EXP_MT_CALC);
#endif // DEBUG

    CMD3(CCC_Mask, "r2_sun", &ps_r2_ls_flags, R2FLAG_SUN);
    CMD3(CCC_Mask, "r2_sun_details", &ps_r2_ls_flags, R2FLAG_SUN_DETAILS);
    CMD3(CCC_Mask, "r2_sun_focus", &ps_r2_ls_flags, R2FLAG_SUN_FOCUS);
    // CMD3(CCC_Mask, "r2_exp_splitscene", &ps_r2_ls_flags, R2FLAG_EXP_SPLIT_SCENE);
    // CMD3(CCC_Mask, "r2_exp_donttest_uns", &ps_r2_ls_flags, R2FLAG_EXP_DONT_TEST_UNSHADOWED);
    CMD3(CCC_Mask, "r2_exp_donttest_shad", &ps_r2_ls_flags, R2FLAG_EXP_DONT_TEST_SHADOWED);

    CMD3(CCC_Mask, "r2_sun_tsm", &ps_r2_ls_flags, R2FLAG_SUN_TSM);
    CMD4(CCC_Float, "r2_sun_tsm_proj", &ps_r2_sun_tsm_projection, .001f, 0.8f);
    CMD4(CCC_Float, "r2_sun_tsm_bias", &ps_r2_sun_tsm_bias, -0.5, +0.5);
    CMD4(CCC_Float, "r2_sun_near", &ps_r2_sun_near, 1.f, 150.f); // AVO: extended from 50.f to 150.f
#if RENDER != R_R1
    CMD4(CCC_Float, "r2_sun_far", &OLES_SUN_LIMIT_27_01_07, 51.f, 180.f);
#endif
    CMD4(CCC_Float, "r2_sun_near_border", &ps_r2_sun_near_border, .5f, 1.0f);
    CMD4(CCC_Float, "r2_sun_depth_far_scale", &ps_r2_sun_depth_far_scale, 0.5, 1.5);
    CMD4(CCC_Float, "r2_sun_depth_far_bias", &ps_r2_sun_depth_far_bias, -0.5, +0.5);
    CMD4(CCC_Float, "r2_sun_depth_near_scale", &ps_r2_sun_depth_near_scale, 0.5, 1.5);
    CMD4(CCC_Float, "r2_sun_depth_near_bias", &ps_r2_sun_depth_near_bias, -0.5, +0.5);
    CMD4(CCC_Float, "r2_sun_lumscale", &ps_r2_sun_lumscale, -1.0, +3.0);
    CMD4(CCC_Float, "r2_sun_lumscale_hemi", &ps_r2_sun_lumscale_hemi, 0.0, +3.0);
    CMD4(CCC_Float, "r2_sun_lumscale_amb", &ps_r2_sun_lumscale_amb, 0.0, +3.0);

    CMD4(CCC_Float, "r2_mblur", &ps_r2_mblur, 0.0f, 1.0f);

    CMD3(CCC_Mask, "r2_gi", &ps_r2_ls_flags, R2FLAG_GI);
    CMD4(CCC_Float, "r2_gi_clip", &ps_r2_GI_clip, EPS, 0.1f);
    CMD4(CCC_Integer, "r2_gi_depth", &ps_r2_GI_depth, 1, 5);
    CMD4(CCC_Integer, "r2_gi_photons", &ps_r2_GI_photons, 8, 256);
    CMD4(CCC_Float, "r2_gi_refl", &ps_r2_GI_refl, EPS_L, 0.99f);

    CMD4(CCC_Integer, "r2_wait_sleep", &ps_r2_wait_sleep, 0, 1);

#ifndef MASTER_GOLD
    CMD4(CCC_Integer, "r2_dhemi_count", &ps_r2_dhemi_count, 4, 25);
    CMD4(CCC_Float, "r2_dhemi_sky_scale", &ps_r2_dhemi_sky_scale, 0.0f, 100.f);
    CMD4(CCC_Float, "r2_dhemi_light_scale", &ps_r2_dhemi_light_scale, 0, 100.f);
    CMD4(CCC_Float, "r2_dhemi_light_flow", &ps_r2_dhemi_light_flow, 0, 1.f);
    CMD4(CCC_Float, "r2_dhemi_smooth", &ps_r2_lt_smooth, 0.f, 10.f);
    CMD3(CCC_Mask, "rs_hom_depth_draw", &ps_r2_ls_flags_ext, R_FLAGEXT_HOM_DEPTH_DRAW);
    CMD3(CCC_Mask, "r2_shadow_cascede_zcul", &ps_r2_ls_flags_ext, R2FLAGEXT_SUN_ZCULLING);

#endif // DEBUG

    CMD4(CCC_Float, "r2_ls_depth_scale", &ps_r2_ls_depth_scale, 0.5, 1.5);
    CMD4(CCC_Float, "r2_ls_depth_bias", &ps_r2_ls_depth_bias, -0.5, +0.5);

    CMD4(CCC_Float, "r2_parallax_h", &ps_r2_df_parallax_h, .0f, .5f);
    //  CMD4(CCC_Float,     "r2_parallax_range",    &ps_r2_df_parallax_range,   5.0f,   175.0f  );

    CMD4(CCC_Float, "r2_slight_fade", &ps_r2_slight_fade, .2f, 1.f);

    // Igor: Depth of field
    tw_min.set(-10000, -10000, 0);
    tw_max.set(10000, 10000, 10000);
    CMD4(CCC_Dof, "r2_dof", &ps_r2_dof, tw_min, tw_max);
    CMD4(CCC_DofNear, "r2_dof_near", &ps_r2_dof.x, tw_min.x, tw_max.x);
    CMD4(CCC_DofFocus, "r2_dof_focus", &ps_r2_dof.y, tw_min.y, tw_max.y);
    CMD4(CCC_DofFar, "r2_dof_far", &ps_r2_dof.z, tw_min.z, tw_max.z);

    CMD4(CCC_Float, "r2_dof_kernel", &ps_r2_dof_kernel_size, .0f, 10.f);
    CMD4(CCC_Float, "r2_dof_sky", &ps_r2_dof_sky, -10000.f, 10000.f);
    CMD3(CCC_Mask, "r2_dof_enable", &ps_r2_ls_flags, R2FLAG_DOF);

    // float ps_r2_dof_near = 0.f; // 0.f
    // float ps_r2_dof_focus = 1.4f; // 1.4f

    if (Core.ParamFlags.test(xrCore::dev))
        CMD4(CCC_SunshaftsIntensity, "r__sunshafts_intensity", &ccSunshaftsIntensity, 0.f, 1.f);

    CMD3(CCC_Mask, "r2_volumetric_lights", &ps_r2_ls_flags, R2FLAG_VOLUMETRIC_LIGHTS);
    // CMD3(CCC_Mask, "r2_sun_shafts", &ps_r2_ls_flags, R2FLAG_SUN_SHAFTS);
    CMD3(CCC_Token, "r2_sun_shafts", &ps_r_sun_shafts, qsun_shafts_token);
    CMD3(CCC_SSAO_Mode, "r2_ssao_mode", &ps_r_ssao_mode, qssao_mode_token);
    CMD3(CCC_Token, "r2_ssao", &ps_r_ssao, qssao_token);
    CMD3(CCC_Mask, "r2_ssao_blur", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_BLUR); // Need restart
    CMD3(CCC_Mask, "r2_ssao_opt_data", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_OPT_DATA); // Need restart
    CMD3(CCC_Mask, "r2_ssao_half_data", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_HALF_DATA); // Need restart
    CMD3(CCC_Mask, "r2_ssao_hbao", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_HBAO); // Need restart
    CMD3(CCC_Mask, "r2_ssao_hdao", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_HDAO); // Need restart
    CMD3(CCC_Mask, "r2_ssao_ssdo", &ps_r2_ls_flags_ext, R2FLAGEXT_SSAO_SSDO); // Need restart
    CMD3(CCC_Mask, "r4_enable_tessellation", &ps_r2_ls_flags_ext, R2FLAGEXT_ENABLE_TESSELLATION); // Need restart
    CMD3(CCC_Mask, "r4_wireframe", &ps_r2_ls_flags_ext, R2FLAGEXT_WIREFRAME); // Need restart
    CMD3(CCC_Mask, "r2_steep_parallax", &ps_r2_ls_flags, R2FLAG_STEEP_PARALLAX);
    CMD3(CCC_Mask, "r2_detail_bump", &ps_r2_ls_flags, R2FLAG_DETAIL_BUMP);
    CMD3(CCC_Mask, "r2_use_bump", &ps_r2_use_bump_flags, R2FLAG_USE_BUMP); // Need restart

    CMD3(CCC_Token, "r2_sun_quality", &ps_r_sun_quality, qsun_quality_token);
    CMD3(CCC_Token, "r2_aa_mode", &r2_aa_mode, r2_aa_mode_token);

    // Igor: need restart
    CMD3(CCC_Mask, "r2_soft_water", &ps_r2_ls_flags, R2FLAG_SOFT_WATER);
    CMD3(CCC_Mask, "r2_soft_particles", &ps_r2_ls_flags, R2FLAG_SOFT_PARTICLES);

    CMD3(CCC_Token, "r2_smap_size", &ps_r2_smap_size, qsmap_size_token);

    // Цветокоррекция
	CMD4(CCC_Float, "r_color_r",    &ps_rcol, 0.0f, 2.55f);
    CMD4(CCC_Float, "r_color_g",    &ps_gcol, 0.0f, 2.55f);
    CMD4(CCC_Float, "r_color_b",    &ps_bcol, 0.0f, 2.55f);
    CMD4(CCC_Float, "r_saturation", &ps_saturation, -1.0f, +1.0f);
//    CMD4(CCC_Float, "r__exposure",  &ps_r2_img_exposure, 0.5f, 4.0f);
//    CMD4(CCC_Float, "r__gamma",     &ps_r2_img_gamma, 0.5f, 2.2f);

    // Дождь на худе
    CMD3(CCC_Mask, "r2_raindrops", &ps_r2_rain_drops_flags, R2FLAG_RAIN_DROPS);
    CMD4(CCC_Float, "r2_rain_drops_intensity", &ps_r2_rain_drops_intensity, 0.f, 1.f);
    CMD4(CCC_Float, "r2_rain_drops_speed", &ps_r2_rain_drops_speed, 0.8f, 5.f);

    // Шейдер стекла и прочие эффекты
	CMD3(CCC_Mask, "r2_hud_mask", &ps_r2_hud_mask_flags, R_FLAG_HUD_MASK);
    CMD3(CCC_Mask, "r2_hud_dyn_effects", &ps_r2_hud_mask_flags, R_FLAG_HUD_DYN_EFFECTS);

	CMD4(CCC_Float, "r2_gloss_factor", &ps_r2_gloss_factor, .0f, 50.f);
    CMD4(CCC_Float, "r2_gloss_min", &ps_r2_gloss_min, .001f, 1.0f);

    // CMD3(CCC_Mask, "r3_msaa", &ps_r2_ls_flags, R3FLAG_MSAA);
    CMD3(CCC_Token, "r3_msaa", &ps_r3_msaa, qmsaa_token);
    // CMD3(CCC_Mask, "r3_msaa_hybrid", &ps_r2_ls_flags, R3FLAG_MSAA_HYBRID);
    // CMD3(CCC_Mask, "r3_msaa_opt", &ps_r2_ls_flags, R3FLAG_MSAA_OPT);
    CMD3(CCC_Mask, "r3_gbuffer_opt", &ps_r2_ls_flags, R3FLAG_GBUFFER_OPT);
    //CMD3(CCC_Mask, "r3_use_dx10_1", &ps_r2_ls_flags, (u32)R3FLAG_USE_DX10_1);
    //CMD3(CCC_Token, "r3_msaa_alphatest", &ps_r3_msaa_atest, qmsaa__atest_token);
    CMD3(CCC_Token, "r3_minmax_sm", &ps_r3_minmax_sm, qminmax_sm_token);

#ifdef DETAIL_RADIUS
    CMD4(CCC_detail_radius, "r__detail_radius", &ps_r__detail_radius, 49, 200);
#endif
    Ivector dopt_min, dopt_max;
    dopt_min.set(1, 1, 1);
    dopt_max.set(390, 390, 390);
    CMD4(CCC_DetailsOpt, "r2_details_opt", &ps_r2_details_opt, dopt_min, dopt_max);
    CMD4(CCC_Integer, "r__clear_models_on_unload", &ps_clear_models_on_unload, 0, 1); // Alundaio
    CMD4(CCC_Integer, "r__use_precompiled_shaders", &ps_use_precompiled_shaders, 0, 1); // Alundaio
    CMD4(CCC_Integer, "r__enable_grass_shadow", &ps_grass_shadow, 0, 1); // Alundaio

//  Allow real-time fog config reload
#if (RENDER == R_R3) || (RENDER == R_R4)
#ifdef DEBUG
    CMD1(CCC_Fog_Reload, "r3_fog_reload");
#endif // DEBUG
#endif // (RENDER == R_R3) || (RENDER == R_R4)

    CMD3(CCC_Mask, "r3_dynamic_wet_surfaces", &ps_r2_ls_flags, R3FLAG_DYN_WET_SURF);
    CMD4(CCC_Float, "r3_dynamic_wet_surfaces_near", &ps_r3_dyn_wet_surf_near, 10, 70);
    CMD4(CCC_Float, "r3_dynamic_wet_surfaces_far", &ps_r3_dyn_wet_surf_far, 30, 100);
    CMD4(CCC_Integer, "r3_dynamic_wet_surfaces_sm_res", &ps_r3_dyn_wet_surf_sm_res, 64, 2048);

    CMD3(CCC_Mask, "r3_volumetric_smoke", &ps_r2_ls_flags, R3FLAG_VOLUMETRIC_SMOKE);
    CMD1(CCC_memory_stats, "render_memory_stats");
    CMD4(CCC_Float,	"r3_reflections_dist", &ps_r2_reflections_distance, 100.f, 1000.f); //Дальность отражений

    // CMD3(CCC_Mask, "r2_sun_ignore_portals", &ps_r2_ls_flags, R2FLAG_SUN_IGNORE_PORTALS);
}

void xrRender_apply_tf()
{
    Console->Execute("r__tf_aniso");
#if RENDER == R_R1
    Console->Execute("r1_tf_mipbias");
#else
    Console->Execute("r2_tf_mipbias");
#endif
}

#endif
