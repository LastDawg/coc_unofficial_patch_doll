#include "stdafx.h"
#include "bolt.h"
#include "ParticlesObject.h"
#include "xrPhysics/PhysicsShell.h"
#include "xr_level_controller.h"
#include "actor.h"
#include "inventory.h"

bool bLessBolts = READ_IF_EXISTS(pSettings, r_bool, "gameplay", "less_bolts", false);

CBolt::CBolt(void) { m_thrower_id = u16(-1); }
CBolt::~CBolt(void) {}
void CBolt::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    IGameObject* o = H_Parent()->H_Parent();
    if (o)
        SetInitiator(o->ID());
}

void CBolt::State(u32 state, u32 old_state)
{
    switch (GetState())
    {
    case eThrowEnd: {
        if (bLessBolts)
        {
            if (m_pPhysicsShell)
                m_pPhysicsShell->Deactivate();
            xr_delete(m_pPhysicsShell);
            m_dwDestroyTime = 0xffffffff;
            PutNextToSlot();
            if (Local())
            {
                // Msg("Destroying local bolt[%d][%d]", ID(), Device.dwFrame);
                DestroyObject();
            }
        }
    }
    break;
    }
    inherited::State(state, old_state);
}

void CBolt::Throw()
{
    CMissile* l_pBolt = smart_cast<CMissile*>(m_fake_missile);
    if (!l_pBolt)
        return;
    l_pBolt->set_destroy_time(u32(m_dwDestroyTimeMax / phTimefactor));
    inherited::Throw();
    spawn_fake_missile();
}

bool CBolt::Useful() const
{
    if (bLessBolts)
        return true;
    else
        return false;
}

bool CBolt::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;
    /*
        switch(cmd)
        {
        case kDROP:
            {
                if(flags&CMD_START)
                {
                    m_throw = false;
                    if(State() == MS_IDLE) State(MS_THREATEN);
                }
                else if(State() == MS_READY || State() == MS_THREATEN)
                {
                    m_throw = true;
                    if(State() == MS_READY) State(MS_THROW);
                }
            }
            return true;
        }
    */
    return false;
}

void CBolt::PutNextToSlot()
{
    if (OnClient())
        return;

    VERIFY(!getDestroy());
    NET_Packet P;
    if (m_pInventory)
    {
        m_pInventory->Ruck(this);

        this->u_EventGen(P, GEG_PLAYER_ITEM2RUCK, this->H_Parent()->ID());
        P.w_u16(this->ID());
        this->u_EventSend(P);
    }
    else
        Msg("! Bolt PutNextToSlot : m_pInventory = NULL [%d][%d]", ID(), Device.dwFrame);

    if (smart_cast<CInventoryOwner*>(H_Parent()) && m_pInventory)
    {
        CBolt* pNext = smart_cast<CBolt*>(m_pInventory->Same(this, true));
        if (!pNext)
            pNext = smart_cast<CBolt*>(m_pInventory->SameSlot(BOLT_SLOT, this, true));

        VERIFY(pNext != this);

        if (pNext && m_pInventory->Slot(pNext->BaseSlot(), pNext))
        {
            pNext->u_EventGen(P, GEG_PLAYER_ITEM2SLOT, pNext->H_Parent()->ID());
            P.w_u16(pNext->ID());
            P.w_u16(pNext->BaseSlot());
            pNext->u_EventSend(P);
            m_pInventory->SetActiveSlot(pNext->BaseSlot());
        }
    }
}

void CBolt::activate_physic_shell()
{
    inherited::activate_physic_shell();
    m_pPhysicsShell->SetAirResistance(.0001f);
}

void CBolt::SetInitiator(u16 id) { m_thrower_id = id; }
u16 CBolt::Initiator() { return m_thrower_id; }
