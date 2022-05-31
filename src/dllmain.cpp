#include "includes.h"

static constexpr float tooFast = 50.f;

// i'm too lazy to make a formula out of this
// so instead i'm just gonna simulate the amount of times initParticle will be called h
// (this is basically a decomp of the first half of CCParticleSystem::update)
size_t nextParticlesCount(CCParticleSystem* self, float dt) {
    auto m_bIsActive = *(bool*)((uintptr_t)self + 356);
    auto m_fEmissionRate = *(float*)((uintptr_t)self + 496);

    if(!m_bIsActive || m_fEmissionRate == 0.f)
        return 0;

    size_t finalCount = 0;

    auto m_uParticleCount = *(unsigned int*)((uintptr_t)self + 360);
    auto m_uTotalParticles = *(unsigned int*)((uintptr_t)self + 500);

    float emissionPeriod = 1.f / m_fEmissionRate;
    unsigned int curParticleCount = m_uParticleCount;
    float startEmissionPeriod = emissionPeriod;

    if(curParticleCount >= m_uTotalParticles)
        return 0;

    auto m_fEmitCounter = *(float*)((uintptr_t)self + 332);

    m_fEmitCounter += dt;
    do {
        if(m_fEmitCounter <= emissionPeriod)
            break;

        if(curParticleCount != m_uTotalParticles) {
            finalCount++;
            m_uParticleCount++;
            emissionPeriod = startEmissionPeriod;
        }

        curParticleCount = m_uParticleCount;
        m_fEmitCounter -= emissionPeriod;
    } while(curParticleCount < m_uTotalParticles);

    return finalCount;
}

std::unordered_map<CCParticleSystem*, CCPoint> _prevPositions;
size_t _newCount = 0;
size_t _newIndex = 0;
float _xDiff;
float _yDiff;

void updateEnd(CCParticleSystem* self, float dt);
void (__thiscall* CCParticleSystem_update)(CCParticleSystem* self, float dt);
void __fastcall CCParticleSystem_update_H(CCParticleSystem* self, void*, float dt) {
    _newCount = 0;

    // don't interpolate if running for the first time
    if(_prevPositions.find(self) == _prevPositions.end()) {
        updateEnd(self, dt);
        return;
    }

    CCPoint prev = _prevPositions[self];

    float curSelfX;
    float curSelfY;
    self->getPosition(&curSelfX, &curSelfY);

    _xDiff = curSelfX - prev.x;
    _yDiff = curSelfY - prev.y;

    // don't interpolate if didn't move or moved too fast
    if(_xDiff == 0.f && _yDiff == 0.f || _xDiff >= tooFast || _yDiff >= tooFast) {
        updateEnd(self, dt);
        return;
    }

    _newCount = nextParticlesCount(self, dt);
    _newIndex = 0;

    updateEnd(self, dt);
}
void updateEnd(CCParticleSystem* self, float dt) {
    CCParticleSystem_update(self, dt);
    _prevPositions[self] = self->getPosition();
}

void (__thiscall* CCParticleSystem_initParticle)(CCParticleSystem* self, tCCParticle* particle);
void __fastcall CCParticleSystem_initParticle_H(CCParticleSystem* self, void*, tCCParticle* particle) {
    CCParticleSystem_initParticle(self, particle);

    if(_newCount == 0)
        return;

    float t = (float)_newIndex / _newCount;

    // don't interpolate on x if didn't move on x
    if(_xDiff != 0.f) {
        float tx = t * _xDiff;
        particle->pos.x = tx + (particle->pos.x - _xDiff);
        particle->startPos.x = tx + (particle->startPos.x - _xDiff);
    }

    // don't interpolate on y if didn't move on y
    if(_yDiff != 0.f) {
        float ty = t * _yDiff;
        particle->pos.y = ty + (particle->pos.y - _yDiff);
        particle->startPos.y = ty + (particle->startPos.y - _yDiff);
    }

    _newIndex++;
}

void resetPrevPosition(CCParticleSystem* self) {
    _prevPositions.erase(self);
}

void (__thiscall* CCParticleSystem_resetSystem)(CCParticleSystem* self);
void __fastcall CCParticleSystem_resetSystem_H(CCParticleSystem* self) {
    resetPrevPosition(self);
    CCParticleSystem_resetSystem(self);
}

void (__thiscall* CCParticleSystem_resumeSystem)(CCParticleSystem* self);
void __fastcall CCParticleSystem_resumeSystem_H(CCParticleSystem* self) {
    resetPrevPosition(self);
    CCParticleSystem_resumeSystem(self);
}

void (__thiscall* CCParticleSystem_dector)(CCParticleSystem* self);
void __fastcall CCParticleSystem_dector_H(CCParticleSystem* self) {
    resetPrevPosition(self);
    CCParticleSystem_dector(self);
}

DWORD WINAPI mainThread(void* hModule) {
    MH_Initialize();

    auto cocos2dBase = reinterpret_cast<uintptr_t>(GetModuleHandle("libcocos2d.dll"));

    MH_CreateHook(reinterpret_cast<void*>(cocos2dBase + 0xb9600), CCParticleSystem_update_H,
        reinterpret_cast<void**>(&CCParticleSystem_update));

    MH_CreateHook(reinterpret_cast<void*>(cocos2dBase + 0xb6e40), CCParticleSystem_initParticle_H,
        reinterpret_cast<void**>(&CCParticleSystem_initParticle));

    MH_CreateHook(reinterpret_cast<void*>(cocos2dBase + 0xb8e70), CCParticleSystem_resetSystem_H,
        reinterpret_cast<void**>(&CCParticleSystem_resetSystem));
    MH_CreateHook(reinterpret_cast<void*>(cocos2dBase + 0xb8ed0), CCParticleSystem_resumeSystem_H,
        reinterpret_cast<void**>(&CCParticleSystem_resumeSystem));
    MH_CreateHook(reinterpret_cast<void*>(cocos2dBase + 0xb68e0), CCParticleSystem_dector_H,
        reinterpret_cast<void**>(&CCParticleSystem_dector));

    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if(reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, mainThread, handle, 0, 0);
    }
    return TRUE;
}
