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

void (__thiscall* CCParticleSystem_update)(CCParticleSystem* self, float dt);
void __fastcall CCParticleSystem_update_H(CCParticleSystem* self, void*, float dt) {
    _newCount = 0;

    CCPoint curr = self->getPosition();

    // don't interpolate if running for the first time
    if(_prevPositions.find(self) == _prevPositions.end()) {
        CCParticleSystem_update(self, dt);
        _prevPositions[self] = curr;
        return;
    }

    CCPoint prev = _prevPositions[self];
    _prevPositions[self] = curr;

    _xDiff = curr.x - prev.x;
    _yDiff = curr.y - prev.y;

    // don't interpolate if didn't move or moved too fast
    if(_xDiff == 0.f && _yDiff == 0.f || _xDiff >= tooFast || _yDiff >= tooFast) {
        CCParticleSystem_update(self, dt);
        return;
    }

    _newCount = nextParticlesCount(self, dt);
    _newIndex = 0;

    CCParticleSystem_update(self, dt);
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

void hook(HMODULE module, const char* symbol, void* detour, void** orig) {
    MH_CreateHook(reinterpret_cast<void*>(GetProcAddress(module, symbol)), detour, orig);
}

DWORD WINAPI mainThread(void* hModule) {
    MH_Initialize();

    auto cocos2d = GetModuleHandle("libcocos2d.dll");

    hook(cocos2d, "?update@CCParticleSystem@cocos2d@@UAEXM@Z",
        reinterpret_cast<void*>(CCParticleSystem_update_H),
        reinterpret_cast<void**>(&CCParticleSystem_update));

    hook(cocos2d, "?initParticle@CCParticleSystem@cocos2d@@QAEXPAUsCCParticle@2@@Z",
        reinterpret_cast<void*>(CCParticleSystem_initParticle_H),
        reinterpret_cast<void**>(&CCParticleSystem_initParticle));

    hook(cocos2d, "?resetSystem@CCParticleSystem@cocos2d@@QAEXXZ",
        reinterpret_cast<void*>(CCParticleSystem_resetSystem_H),
        reinterpret_cast<void**>(&CCParticleSystem_resetSystem));
    hook(cocos2d, "?resumeSystem@CCParticleSystem@cocos2d@@QAEXXZ",
        reinterpret_cast<void*>(CCParticleSystem_resumeSystem_H),
        reinterpret_cast<void**>(&CCParticleSystem_resumeSystem));
    hook(cocos2d, "??1CCParticleSystem@cocos2d@@UAE@XZ",
        reinterpret_cast<void*>(CCParticleSystem_dector_H),
        reinterpret_cast<void**>(&CCParticleSystem_dector));

    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if(reason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0x100, mainThread, handle, 0, 0);
    return TRUE;
}
