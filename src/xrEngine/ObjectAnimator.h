#ifndef ObjectAnimatorH
#define ObjectAnimatorH
#pragma once

#include "xrCore/Animation/Motion.hpp"

// refs
class ENGINE_API CObjectAnimator
{
    using MotionVec = xr_vector<COMotion*>;

public:
    const SAnimParams& anim_param() { return m_MParam; }
    bool bLoop;

    shared_str m_Name;

    Fmatrix m_XFORM;
    SAnimParams m_MParam;
    MotionVec m_Motions;
    float m_Speed;

    COMotion* m_Current;
    void LoadMotions(LPCSTR fname);
    void SetActiveMotion(COMotion* mot);
    COMotion* FindMotionByName(LPCSTR name);

    CObjectAnimator();
    virtual ~CObjectAnimator();

    void Clear();
    void Load(LPCSTR name);
    IC LPCSTR Name() const { return *m_Name; }
    float& Speed() { return m_Speed; }
    COMotion* Play(bool bLoop, LPCSTR name = 0);
    void Pause(bool val) { return m_MParam.Pause(val); }
    void Stop();
    IC BOOL IsPlaying() const { return m_MParam.bPlay; }
    IC const Fmatrix& XFORM() const { return m_XFORM; }
    float GetLength() const;
    // Update
    void Update(float dt);
    void DrawPath();
};

#endif // ObjectAnimatorH
