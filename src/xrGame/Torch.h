#pragma once

#include "inventory_item_object.h"
#include "hudsound.h"

class CLAItem;

class CTorch : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

protected:
    float fBrightness;
    CLAItem* lanim;

    u16 guid_bone;
    shared_str light_trace_bone;

    float m_delta_h;
    Fvector2 m_prev_hp;
    bool m_switched_on;
	int             torch_mode;
	float           range;
	float           range_o;
	float           range2;
	float           range_o2;
	float           glow_radius;
	float           glow_radius2;
	Fvector			m_focus;
    ref_light light_render;
    ref_light light_omni;
    ref_glow glow_render;
    shared_str m_light_section;
    Fvector m_torch_offset;
    Fvector m_omni_offset;
    float m_torch_inertion_speed_max;
    float m_torch_inertion_speed_min;

	float m_fDecayRate;
    float m_fPassiveDecayRate;

    bool m_bTorchModeEnabled; // ����� ������

    virtual bool install_upgrade_impl(LPCSTR section, bool test);

    IC int GetTorchNV_Type() const { return m_NightVisionType; }

private:
    inline bool can_use_dynamic_lights();

public:
    CTorch();
    virtual ~CTorch();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server

    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);

    virtual void OnMoveToSlot(const SInvItemPlace& prev);
    virtual void OnMoveToRuck(const SInvItemPlace& prev);
    virtual void UpdateCL();

    void ConditionUpdate();
	void SwitchTorchMode();
    void Switch();
    void Switch(bool light_on);
    void ProcessSwitch();
    bool torch_active() const;
    void UpdateUseAnim();
    shared_str m_sShaderNightVisionSect;
    u32 m_NightVisionType;
    float m_fNightVisionLumFactor;

    virtual bool can_be_attached() const;

    // CAttachableItem
    virtual void enable(bool value);

	int m_iActionTiming;
    int m_iAnimLength;
    bool m_bActivated;
    bool m_bSwitched;

    shared_str m_NightVisionSect;
    bool m_bNightVisionEnabled; // ����� ���
protected:
    HUD_SOUND_COLLECTION m_sounds;
    enum EStats
    {
        eTorchActive = (1 << 0),
        eNightVisionActive = (1 << 1),
        eAttached = (1 << 2)
    };

public:
    virtual bool use_parent_ai_locations() const { return (!H_Parent()); }
    virtual void create_physic_shell();
    virtual void activate_physic_shell();
    virtual void setup_physic_shell();

    virtual void afterDetach();
    virtual void renderable_Render();
	ref_sound 		m_switch_sound;
};

#undef script_type_list
#define script_type_list save_type_list(CTorch)
