#pragma once

#include "inventory_item.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem
{
private:
    typedef CInventoryItem inherited;

protected:
    CPhysicItem* m_physic_item;

    u8 m_iMaxUses;
    bool m_bRemoveAfterUse;
    float m_fWeightFull;
    float m_fWeightEmpty;

public:
    CEatableItem();
    virtual ~CEatableItem();
    virtual IFactoryObject* _construct();
    virtual CEatableItem* cast_eatable_item() { return this; }
    virtual void Load(LPCSTR section);
    void load(IReader& packet) override;
    void save(NET_Packet& packet) override;
    virtual bool Useful() const;

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Import(NET_Packet& P);					// import from server
    virtual void net_Export(NET_Packet& P);					// export to server

    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();
    virtual bool UseBy(CEntityAlive* npc);

	void UpdateInRuck();
	void UpdateUseAnim();
    void HideWeapon();
    void StartAnimation();
    bool m_bHasAnimation;
    bool m_bActivated;
    bool m_bItmStartAnim;
    int m_iAnimHandsCnt;
    int m_iAnimLength;
    float m_fEffectorIntensity;
    LPCSTR anim_sect;
    shared_str use_cam_effector;
    ref_sound m_using_sound;

    virtual bool Empty() const { return GetRemainingUses() == 0; }
    bool CanDelete() const { return m_bRemoveAfterUse == true; }
    virtual u8 GetMaxUses() { return m_iMaxUses; }
    virtual u8 GetRemainingUses() const { return (u8)roundf(((float)m_iMaxUses)*m_fCondition); }
    void SetRemainingUses(u8 value) { m_fCondition = ((float)value / (float)m_iMaxUses); clamp(m_fCondition, 0.f, 1.f); };
    float Weight() const override;
};
