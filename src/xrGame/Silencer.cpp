///////////////////////////////////////////////////////////////
// Silencer.cpp
// Silencer - апгрейд оружия глушитель и лазер, пока тут побудут - надо остальные аддоны подтянуть
///////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "silencer.h"

CSilencer::CSilencer() {}
CSilencer::~CSilencer() {}
BOOL CSilencer::net_Spawn(CSE_Abstract* DC) { return (inherited::net_Spawn(DC)); }
void CSilencer::Load(LPCSTR section) { inherited::Load(section); }
void CSilencer::net_Destroy() { inherited::net_Destroy(); }
void CSilencer::UpdateCL() { inherited::UpdateCL(); }
void CSilencer::OnH_A_Chield() { inherited::OnH_A_Chield(); }
void CSilencer::OnH_B_Independent(bool just_before_destroy) { inherited::OnH_B_Independent(just_before_destroy); }
void CSilencer::renderable_Render() { inherited::renderable_Render(); }

CLaser::CLaser() {}
CLaser::~CLaser() {}
BOOL CLaser::net_Spawn(CSE_Abstract* DC) { return (inherited::net_Spawn(DC)); }
void CLaser::Load(LPCSTR section) { inherited::Load(section); }
void CLaser::net_Destroy() { inherited::net_Destroy(); }
void CLaser::UpdateCL() { inherited::UpdateCL(); }
void CLaser::OnH_A_Chield() { inherited::OnH_A_Chield(); }
void CLaser::OnH_B_Independent(bool just_before_destroy) { inherited::OnH_B_Independent(just_before_destroy); }
void CLaser::renderable_Render() { inherited::renderable_Render(); }
