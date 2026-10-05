// GTA Vice City / GTA III - The Definitive Edition (1.112) in-game trainer.
// Built twice: with GAME_VC for ViceCity.exe and GAME_III for LibertyCity.exe.
// Every address was reverse-engineered from the 1.0.112.48699928 executables.
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "MinHook.h"
#include "imgui.h"
#include "../core/common.h"
#include "../core/gamethread.h"
#include "../core/log.h"
#include "../core/menu.h"
#include "../core/render.h"
#include "classic_data.h"

using namespace core;

namespace classic
{
// ---------- addresses (RVAs) and struct offsets ----------
#if defined(GAME_VC)
namespace addr
{
constexpr uintptr_t PlayerInFocus = 0x508749B, Players = 0x5247A90;
constexpr uintptr_t TimerUpdate = 0x10DA770, TimeScale = 0x5120D48;
constexpr uintptr_t VehicleCheat = 0x10A9170; // void (int model)
constexpr uintptr_t SetGameClock = 0x1092370; // void (uint8 hours, uint8 minutes)
constexpr uintptr_t Hours = 0x4F83DF7, Minutes = 0x4F83DF6;
constexpr uintptr_t Weather[3] = {0x518C6FC, 0x5188D1C, 0x5188D20};
constexpr uintptr_t TimeStep = 0x4DEF5B0;       // float CTimer::ms_fTimeStep
constexpr uintptr_t Camera = 0x4E2A200;         // CCamera TheCamera (matrix inline)
constexpr uintptr_t RadarTrace = 0x5244C80;     // CBlip[75], 0x38 each
constexpr uintptr_t FindGroundZ = 0x10E2740;    // float CWorld::FindGroundZFor3DCoord(x, y, z, bool*, CEntity**)
constexpr uintptr_t GiveWeapon = 0x11070A0;     // void CPed::GiveWeapon(CPed*, int type, int ammo)
constexpr uintptr_t RequestModel = 0x1182A50;   // void CStreaming::RequestModel(int model, int flags, int)
constexpr uintptr_t LoadAllRequested = 0x11845A0; // void CStreaming::LoadAllRequestedModels(bool)
constexpr uintptr_t UpdateColours = 0x11E2F50;  // void (CVehicle*): pushes new paint to the renderer
constexpr uintptr_t ChangePlayerModel = 0x10A9930; // void (const char* modelName), used by the skin cheats
constexpr uintptr_t ModelInfo = 0x517B170;      // CBaseModelInfo* ms_modelInfoPtrs[], name at +8
constexpr uintptr_t PedTypes = 0x5123660;       // CPedType* ms_apPedType[17], threat mask at +0x18
} // namespace addr
namespace off
{
constexpr uintptr_t InfoStride = 0x1A8, Money = 0xF0;
constexpr int MaxHealth = 0x197, MaxArmor = 0x198;
constexpr int InfiniteSprint = 0x194, FastReload = 0x195, Fireproof = 0x196; // set by the game's script commands
constexpr uintptr_t Health = 0x4E4, Armor = 0x4E8, InVehicle = 0x558, Vehicle = 0x550, Wanted = 0x898;
constexpr uintptr_t WantedLevel = 0x20, Weapons = 0x5E8, WeaponStride = 0x20;
constexpr int WeaponCount = 10;
constexpr uintptr_t VehHealth = 0x320, VehType = 0x3DC;
constexpr uintptr_t MoveSpeed = 0xFC, VehColour = 0x290;
constexpr int BlipCount = 75, BlipStride = 0x38, BlipInUse = 0x27, BlipWaypoint = 0x36;
} // namespace off
constexpr int kChaosForStars[7] = {0, 70, 200, 570, 1220, 2420, 4820};
#elif defined(GAME_III)
namespace addr
{
constexpr uintptr_t PlayerInFocus = 0x4FC306F, Players = 0x516E060;
constexpr uintptr_t TimerUpdate = 0x10A6880, TimeScale = 0x5042F50;
constexpr uintptr_t TankCheat = 0x10876F0;       // spawns the model stored at its "mov [model], 122"
constexpr uintptr_t TankModelImm = 0x1087789;    // imm32 of that mov
constexpr uintptr_t RequestModel = 0x112D440;    // void (int model, int flags, int)
constexpr uintptr_t LoadAllRequested = 0x112EBD0; // void (bool)
constexpr uintptr_t ModelLoaded = 0x5086380;     // uint8 state per model, stride 32 (1 = loaded)
constexpr uintptr_t SetGameClock = 0x10B4570;    // void (?, uint8 hours, uint8 minutes)
constexpr uintptr_t FixAutomobile = 0x115B320;   // void (CAutomobile*)
constexpr uintptr_t Hours = 0x4FC3066, Minutes = 0x4FC3067;
constexpr uintptr_t Weather[3] = {0x505F394, 0x5086238, 0x5086234};
constexpr uintptr_t TimeStep = 0x503A5BC;        // float CTimer::ms_fTimeStep
constexpr uintptr_t Camera = 0x4D9F880;          // CCamera TheCamera (matrix inline)
constexpr uintptr_t RadarTrace = 0x516D700;      // CBlip[32], 0x30 each
constexpr uintptr_t FindGroundZ = 0x10AE5D0;     // float CWorld::FindGroundZFor3DCoord(x, y, z, bool*, CEntity**)
constexpr uintptr_t GiveWeapon = 0x10D5B00;      // void CPed::GiveWeapon(CPed*, int type, int ammo)
constexpr uintptr_t UpdateColours = 0x1175520;   // void (CVehicle*): pushes new paint to the renderer
constexpr uintptr_t PedTypes = 0x504EAF0;        // CPedType* ms_apPedType[17], threat mask at +0x18
} // namespace addr
namespace off
{
constexpr uintptr_t InfoStride = 0x178, Money = 0xF8;
constexpr int MaxHealth = -1, MaxArmor = -1; // GTA III always uses 100
constexpr int InfiniteSprint = 0x170, FastReload = 0x171, Fireproof = -1; // no fireproof perk in III
constexpr uintptr_t Health = 0x430, Armor = 0x434, InVehicle = 0x4A0, Vehicle = 0x498, Wanted = 0x7A8;
constexpr uintptr_t WantedLevel = 0x18, Weapons = 0x510, WeaponStride = 0x20;
constexpr int WeaponCount = 13;
constexpr uintptr_t VehHealth = 0x330, VehType = 0x3CC;
constexpr uintptr_t MoveSpeed = 0x114, VehColour = 0x2A0;
constexpr int BlipCount = 32, BlipStride = 0x30, BlipInUse = 0x23, BlipWaypoint = 0x2E;
} // namespace off
constexpr int kChaosForStars[7] = {0, 60, 220, 420, 820, 1620, 3220};
#endif

// Entities keep their matrix inline: right at +0x08, forward +0x18, up +0x28, position +0x38.
constexpr uintptr_t kRight = 0x08, kForward = 0x18, kUp = 0x28, kPos = 0x38;
constexpr uintptr_t kVtblTeleport = 0x58; // CEntity::Teleport(CVector*)

// ---------- game access ----------

uint8_t* PlayerInfo()
{
    uint8_t idx = At<uint8_t>(addr::PlayerInFocus);
    if (idx > 1) idx = 0;
    return &At<uint8_t>(addr::Players + idx * off::InfoStride);
}

void* PlayerPed() { return *reinterpret_cast<void**>(PlayerInfo()); }

void* PlayerVehicle(void* ped)
{
    if (!ped || !Field<uint8_t>(ped, off::InVehicle)) return nullptr;
    return Field<void*>(ped, off::Vehicle);
}

float MaxHealth() { return off::MaxHealth < 0 ? 100.0f : (float)std::max(100, (int)PlayerInfo()[off::MaxHealth]); }
float MaxArmor() { return off::MaxArmor < 0 ? 100.0f : (float)std::max(100, (int)PlayerInfo()[off::MaxArmor]); }
int& Money() { return *reinterpret_cast<int*>(PlayerInfo() + off::Money); }
float* PosOf(void* e) { return &Field<float>(e, kPos); }

void SetWanted(void* ped, int stars)
{
    void* w = ped ? Field<void*>(ped, off::Wanted) : nullptr;
    if (!w) return;
    stars = std::clamp(stars, 0, 6);
    Field<int>(w, 0) = kChaosForStars[stars];
    Field<int>(w, off::WantedLevel) = stars;
}

float TimeStep() { return std::clamp(At<float>(addr::TimeStep), 0.01f, 3.0f); }

float* Velocity(void* e) { return &Field<float>(e, off::MoveSpeed); }

// Zeroes linear and angular velocity (they're stored back to back).
void Stop(void* e)
{
    float* v = Velocity(e);
    for (int i = 0; i < 6; i++) v[i] = 0.0f;
}

// The game's own teleport: moves the entity between world sectors and resets its physics.
void TeleportEntity(void* e, float x, float y, float z)
{
    float v[3] = {x, y, z};
    void** vtbl = *reinterpret_cast<void***>(e);
    reinterpret_cast<void (*)(void*, float*)>(vtbl[kVtblTeleport / 8])(e, v);
}

float GroundZ(float x, float y, bool* found)
{
    *found = false;
    return Fn<float (*)(float, float, float, bool*, void*)>(addr::FindGroundZ)(x, y, 1000.0f, found, nullptr);
}

// The marker placed on the pause-menu map: an in-use blip with the remaster's waypoint flag.
bool FindWaypoint(float& x, float& y)
{
    for (int i = 0; i < off::BlipCount; i++)
    {
        uint8_t* b = &At<uint8_t>(addr::RadarTrace + i * off::BlipStride);
        if (b[off::BlipInUse] == 1 && b[off::BlipWaypoint] == 1)
        {
            x = *reinterpret_cast<float*>(b + 0xC);
            y = *reinterpret_cast<float*>(b + 0x10);
            return true;
        }
    }
    return false;
}

using CheatFn = void (*)();

// Toggle cheats start with "cmp byte [rip+X], 0 / sete byte [rip+X]": X is their on/off flag.
uint8_t* CheatFlag(unsigned rva)
{
    const uint8_t* p = &At<uint8_t>(rva);
    if (p[0] == 0x80 && p[1] == 0x3D && p[6] == 0x00 && p[7] == 0x0F && p[8] == 0x94)
        return const_cast<uint8_t*>(p + 7 + *reinterpret_cast<const int32_t*>(p + 2));
    return nullptr;
}

void RepairVehicleNow(void* veh)
{
    Field<float>(veh, off::VehHealth) = 1000.0f;
    if (Field<int>(veh, off::VehType) != 0) return; // only cars have the damage model
#if defined(GAME_VC)
    for (int i = 0; i < 5; i++) Field<uint8_t>(veh, 0x3E4 + i) = 0; // what the game's own health cheat resets
#else
    Field<uint8_t>(veh, 0x3DC) = 0;
    Field<int>(veh, 0x700) = 0;
    Fn<void (*)(void*)>(addr::FixAutomobile)(veh);
#endif
}

// ---------- state ----------

struct State
{
    bool god = false, armor = false, neverWanted = false, infAmmo = false;
    bool vehGod = false, vehAutoRepair = false, vehKeys = true, speedometer = false;
    bool freezeTime = false, freezeWeather = false, customSpeed = false, showCoords = false;
    bool noclip = false;
    int wanted = 0, hour = 12, minute = 0, weather = 0, colour1 = 0, colour2 = 0;
    float speed = 1.0f, noclipSpeed = 1.0f, boost = 1.0f;
} s;

struct Hotkeys
{
    int waypoint = VK_F5, noclip = VK_F6, boost = VK_ADD, stop = VK_SUBTRACT, jump = VK_MULTIPLY;
} g_keys;

float g_slots[3][3];
bool g_slotUsed[3];

// A player perk flag (the rewards for side missions). Turning one off restores whatever the game had,
// so a perk you earned yourself isn't taken away.
struct Perk
{
    int offset;
    bool on = false;
    bool applied = false;
    uint8_t saved = 0;

    void Apply()
    {
        if (offset < 0) return;
        uint8_t& flag = PlayerInfo()[offset];
        if (on)
        {
            if (!applied) saved = flag;
            applied = true;
            flag = 1;
        }
        else if (applied)
        {
            flag = saved;
            applied = false;
        }
    }
};

Perk g_sprint{off::InfiniteSprint}, g_fastReload{off::FastReload}, g_fireproof{off::Fireproof};

void SetWeatherNow(int id)
{
    for (uintptr_t a : addr::Weather) At<int16_t>(a) = (int16_t)id;
}

// ---------- no-clip and waypoint teleport (game thread) ----------

void* g_noclipEntity = nullptr;
float g_noclipPos[3];

void NoclipFrame(void* ped)
{
    void* veh = PlayerVehicle(ped);
    void* e = veh ? veh : ped;
    float* p = PosOf(e);
    if (!s.noclip)
    {
        if (g_noclipEntity)
        {
            // Put the player back on the ground below instead of letting them fall from the sky.
            bool found;
            float z = GroundZ(p[0], p[1], &found);
            if (found && z < p[2]) TeleportEntity(e, p[0], p[1], z + 1.0f);
            g_noclipEntity = nullptr;
        }
        return;
    }
    if (g_noclipEntity != e)
    {
        for (int i = 0; i < 3; i++) g_noclipPos[i] = p[i];
        g_noclipEntity = e;
    }
    if (!menu::IsOpen())
    {
        const float* f = &At<float>(addr::Camera + kForward); // where the camera looks
        float len = sqrtf(f[0] * f[0] + f[1] * f[1]);
        float rx = len > 0.01f ? f[1] / len : 1.0f, ry = len > 0.01f ? -f[0] / len : 0.0f;
        float step = 0.6f * s.noclipSpeed * TimeStep();
        if (KeyDown(VK_SHIFT)) step *= 4.0f;
        if (KeyDown(VK_MENU)) step *= 0.25f;
        float move[3] = {0, 0, 0};
        if (KeyDown('W')) for (int i = 0; i < 3; i++) move[i] += f[i];
        if (KeyDown('S')) for (int i = 0; i < 3; i++) move[i] -= f[i];
        if (KeyDown('D')) { move[0] += rx; move[1] += ry; }
        if (KeyDown('A')) { move[0] -= rx; move[1] -= ry; }
        if (KeyDown(VK_SPACE)) move[2] += 1.0f;
        if (KeyDown(VK_CONTROL)) move[2] -= 1.0f;
        for (int i = 0; i < 3; i++) g_noclipPos[i] += move[i] * step;
    }
    for (int i = 0; i < 3; i++) p[i] = g_noclipPos[i];
    Stop(e);
}

struct PendingWaypoint
{
    bool active = false;
    float x = 0, y = 0;
    int frames = 0;
} g_wp;

void TeleportToWaypoint()
{
    void* ped = PlayerPed();
    if (!ped) return;
    float x, y;
    if (!FindWaypoint(x, y))
    {
        menu::Notify("Place a waypoint on the map first");
        return;
    }
    void* veh = PlayerVehicle(ped);
    void* e = veh ? veh : ped;
    bool found;
    float z = GroundZ(x, y, &found);
    if (found)
    {
        TeleportEntity(e, x, y, z + 1.0f);
        menu::Notify("Teleported to the waypoint");
        return;
    }
    // The ground there isn't streamed in yet: wait above it until its collision loads.
    TeleportEntity(e, x, y, 200.0f);
    g_wp = {true, x, y, 0};
    menu::Notify("Loading the area...");
}

void WaypointFrame(void* ped)
{
    if (!g_wp.active) return;
    void* veh = PlayerVehicle(ped);
    void* e = veh ? veh : ped;
    bool found;
    float z = GroundZ(g_wp.x, g_wp.y, &found);
    if (found || ++g_wp.frames > 400)
    {
        TeleportEntity(e, g_wp.x, g_wp.y, found ? z + 1.0f : 15.0f);
        g_wp.active = false;
        menu::Notify(found ? "Teleported to the waypoint" : "Teleported (couldn't find the ground there)");
        return;
    }
    float* p = PosOf(e);
    p[0] = g_wp.x;
    p[1] = g_wp.y;
    p[2] = 200.0f;
    Stop(e);
}

void HotkeyFrame(void* ped)
{
    if (menu::IsOpen()) return;
    if (KeyPressed(g_keys.waypoint)) TeleportToWaypoint();
    if (KeyPressed(g_keys.noclip))
    {
        s.noclip = !s.noclip;
        menu::Notify("No-clip %s", s.noclip ? "ON (WASD, Space/Ctrl, Shift = fast)" : "OFF");
    }
    void* veh = PlayerVehicle(ped);
    if (!s.vehKeys || !veh || s.noclip) return;
    float* v = Velocity(veh);
    const float* f = &Field<float>(veh, kForward);
    if (KeyDown(g_keys.boost))
        for (int i = 0; i < 3; i++) v[i] += f[i] * 0.025f * s.boost * TimeStep();
    if (KeyPressed(g_keys.stop)) Stop(veh);
    if (KeyPressed(g_keys.jump)) v[2] += 0.25f;
}

// ---------- riot mode ----------
// The game's riot cheat makes every ped type hostile to everything, and "everyone attacks you" adds the player
// to every threat list. Neither can be undone in game (and both end up in the save), so the trainer applies
// them itself and puts the original threat lists back when they're turned off.

constexpr int kPedTypeCount = 17;
constexpr uint32_t kRiotThreats = 0xFFFFF, kPlayerThreat = 1;

struct PedThreats
{
    bool riot = false, attack = false;
    bool haveDefaults = false, applied = false;
    uint32_t defaults[kPedTypeCount]; // from ped.dat, read before any save is loaded
    uint32_t before[kPedTypeCount];   // what the game had when riot / attack was switched on
} g_threats;

uint32_t* Threats(int i)
{
    void* type = At<void*>(addr::PedTypes + i * 8);
    return type ? &Field<uint32_t>(type, 0x18) : nullptr;
}

void ThreatsFrame()
{
    PedThreats& t = g_threats;
    for (int i = 0; i < kPedTypeCount; i++)
        if (!Threats(i)) return; // ped types not created yet
    if (!t.haveDefaults)
    {
        for (int i = 0; i < kPedTypeCount; i++) t.defaults[i] = *Threats(i);
        t.haveDefaults = true;
    }
    if (t.riot || t.attack)
    {
        if (!t.applied)
        {
            for (int i = 0; i < kPedTypeCount; i++) t.before[i] = *Threats(i);
            t.applied = true;
        }
        for (int i = 0; i < kPedTypeCount; i++)
        {
            uint32_t v = t.riot ? kRiotThreats : t.before[i];
            if (t.attack) v |= kPlayerThreat;
            *Threats(i) = v;
        }
    }
    else if (t.applied)
    {
        // A save made during riot mode still has the riot lists, so fall back to the ped.dat ones.
        for (int i = 0; i < kPedTypeCount; i++)
            *Threats(i) = t.before[i] == kRiotThreats ? t.defaults[i] : t.before[i];
        t.applied = false;
    }
}

void ResetPeds()
{
    gt::Post([] {
        PedThreats& t = g_threats;
        t.riot = t.attack = t.applied = false;
        if (!t.haveDefaults) return;
        for (int i = 0; i < kPedTypeCount; i++)
            if (uint32_t* p = Threats(i)) *p = t.defaults[i];
        menu::Notify("Peds are back to normal");
    });
}

void PerFrame()
{
    ThreatsFrame();
    void* ped = PlayerPed();
    if (!ped) return;
    NoclipFrame(ped);
    WaypointFrame(ped);
    HotkeyFrame(ped);
    if (s.god && Field<float>(ped, off::Health) < MaxHealth()) Field<float>(ped, off::Health) = MaxHealth();
    if (s.armor && Field<float>(ped, off::Armor) < MaxArmor()) Field<float>(ped, off::Armor) = MaxArmor();
    if (s.neverWanted) SetWanted(ped, 0);
    g_sprint.Apply();
    g_fastReload.Apply();
    g_fireproof.Apply();
    if (s.infAmmo)
        for (int i = 0; i < off::WeaponCount; i++)
        {
            uint8_t* slot = (uint8_t*)ped + off::Weapons + i * off::WeaponStride;
            int& total = *reinterpret_cast<int*>(slot + 0xC);
            if (*reinterpret_cast<int*>(slot) > 0 && total > 0 && total < 9000) total = 9999;
        }
    if (void* veh = PlayerVehicle(ped))
    {
        float hp = Field<float>(veh, off::VehHealth);
        if (s.vehAutoRepair && hp < 1000.0f) RepairVehicleNow(veh);
        else if (s.vehGod && hp < 1000.0f) Field<float>(veh, off::VehHealth) = 1000.0f;
    }
    if (s.freezeTime)
    {
        At<uint8_t>(addr::Hours) = (uint8_t)s.hour;
        At<uint8_t>(addr::Minutes) = (uint8_t)s.minute;
    }
    if (s.freezeWeather) SetWeatherNow(s.weather);
    if (s.customSpeed) At<float>(addr::TimeScale) = s.speed;
}

// ---------- actions (run on the game thread) ----------

void Post(std::function<void()> fn) { gt::Post(std::move(fn)); }

void SpawnVehicle(int model, const char* name)
{
    Post([model, name] {
        if (!PlayerPed()) return;
#if defined(GAME_VC)
        Fn<void (*)(int)>(addr::VehicleCheat)(model);
#else
        // Load the model ourselves, then run the tank cheat with its model constant swapped.
        Fn<void (*)(int, int, int)>(addr::RequestModel)(model, 0, 0);
        Fn<void (*)(int)>(addr::LoadAllRequested)(0);
        if (At<uint8_t>(addr::ModelLoaded + model * 32) != 1)
        {
            menu::Notify("Couldn't load %s", name);
            return;
        }
        int32_t* imm = &At<int32_t>(addr::TankModelImm);
        DWORD old;
        VirtualProtect(imm, 4, PAGE_EXECUTE_READWRITE, &old);
        *imm = model;
        Fn<void (*)()>(addr::TankCheat)();
        *imm = 122;
        VirtualProtect(imm, 4, old, &old);
        FlushInstructionCache(GetCurrentProcess(), imm, 4);
#endif
        menu::Notify("Spawned %s", name);
    });
}

void Teleport(float x, float y, float z)
{
    Post([x, y, z] {
        void* ped = PlayerPed();
        if (!ped) return;
        void* veh = PlayerVehicle(ped);
        void* e = veh ? veh : ped;
        TeleportEntity(e, x, y, z);
        if (e == g_noclipEntity)
        {
            g_noclipPos[0] = x;
            g_noclipPos[1] = y;
            g_noclipPos[2] = z;
        }
    });
}

void GiveWeapon(const WeaponModel& w)
{
    WeaponModel copy = w;
    Post([copy] {
        void* ped = PlayerPed();
        if (!ped) return;
#if defined(GAME_VC)
        Fn<void (*)(int, int, int)>(addr::RequestModel)(copy.model, 1, 0);
        if (copy.type == 33) Fn<void (*)(int, int, int)>(addr::RequestModel)(294, 1, 0); // minigun's second model
        Fn<void (*)(bool)>(addr::LoadAllRequested)(false);
#endif
        Fn<void (*)(void*, int, int)>(addr::GiveWeapon)(ped, copy.type, copy.ammo);
        menu::Notify("Got %s", copy.name);
    });
}

void PaintVehicle(int c1, int c2)
{
    Post([c1, c2] {
        void* veh = PlayerVehicle(PlayerPed());
        if (!veh) { menu::Notify("You're not in a vehicle"); return; }
        Field<uint8_t>(veh, off::VehColour) = (uint8_t)c1;
        Field<uint8_t>(veh, off::VehColour + 1) = (uint8_t)c2;
        Fn<void (*)(void*)>(addr::UpdateColours)(veh);
    });
}

void SetTime(int h, int m)
{
    Post([h, m] {
#if defined(GAME_VC)
        Fn<void (*)(uint8_t, uint8_t)>(addr::SetGameClock)((uint8_t)h, (uint8_t)m);
#else
        Fn<void (*)(void*, uint8_t, uint8_t)>(addr::SetGameClock)(nullptr, (uint8_t)h, (uint8_t)m);
#endif
    });
}

void Cheat(const CheatInfo& c)
{
    unsigned rva = c.rva;
    const char* name = c.name;
    Post([rva, name] {
        Fn<CheatFn>(rva)();
        if (uint8_t* flag = CheatFlag(rva)) menu::Notify("%s: %s", name, *flag ? "ON" : "OFF");
        else menu::Notify("%s", name);
    });
}

void RepairVehicle()
{
    Post([] {
        void* veh = PlayerVehicle(PlayerPed());
        if (!veh) { menu::Notify("You're not in a vehicle"); return; }
        RepairVehicleNow(veh);
        menu::Notify("Vehicle repaired");
    });
}

void FlipVehicle()
{
    Post([] {
        void* veh = PlayerVehicle(PlayerPed());
        if (!veh) { menu::Notify("You're not in a vehicle"); return; }
        float* right = &Field<float>(veh, kRight);
        float* fwd = &Field<float>(veh, kForward);
        float* up = &Field<float>(veh, kUp);
        float fx = fwd[0], fy = fwd[1];
        float len = sqrtf(fx * fx + fy * fy);
        if (len < 0.01f) { fx = 0; fy = 1; len = 1; }
        fx /= len;
        fy /= len;
        fwd[0] = fx; fwd[1] = fy; fwd[2] = 0;
        up[0] = 0; up[1] = 0; up[2] = 1;
        right[0] = fy; right[1] = -fx; right[2] = 0;
        Field<float>(veh, kPos + 8) += 1.0f;
        menu::Notify("Vehicle flipped upright");
    });
}

// ---------- menus ----------

template <size_t N> void CheatRows(const CheatInfo (&list)[N])
{
    for (const CheatInfo& c : list)
    {
        uint8_t* flag = CheatFlag(c.rva);
        bool hit = flag ? menu::ToggleState(c.name, *flag != 0) : menu::Action(c.name);
        if (hit) Cheat(c);
    }
}

#if defined(GAME_VC)
// Ped models the game's own "random outfit" cheat may pick (it skips 8 and 28).
const char* ModelName(int id)
{
    void* mi = At<void*>(addr::ModelInfo + id * 8);
    if (!mi) return nullptr;
    const char* name = reinterpret_cast<const char*>(mi) + 8;
    for (int i = 0; i < 24; i++)
    {
        if (!name[i]) return i ? name : nullptr;
        if (name[i] < 32 || name[i] > 126) return nullptr;
    }
    return nullptr;
}

void ChangeSkin(const char* name)
{
    std::string n = name;
    Post([n] {
        void* ped = PlayerPed();
        if (!ped || PlayerVehicle(ped)) { menu::Notify("Get out of the vehicle first"); return; }
        Fn<void (*)(const char*)>(addr::ChangePlayerModel)(n.c_str());
        menu::Notify("Skin: %s", n.c_str());
    });
}

void SkinMenu()
{
    if (menu::Action("Tommy (default)")) ChangeSkin("player");
    for (int id = 1; id <= 0x5F; id++)
    {
        if (id == 8 || id == 28) continue;
        if (const char* name = ModelName(id))
            if (menu::Action(name, "Changes your character model.")) ChangeSkin(name);
    }
}
#endif

void PlayerMenu()
{
    if (void* ped = PlayerPed())
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f / %.0f", Field<float>(ped, off::Health), Field<float>(ped, off::Armor));
        menu::Info("Health / armor", buf);
        snprintf(buf, sizeof(buf), "$%d", Money());
        menu::Info("Money", buf);
    }
    menu::Toggle("God mode", &s.god, "Keeps your health full every frame.");
    menu::Toggle("Infinite armor", &s.armor);
    menu::Toggle("Infinite stamina", &g_sprint.on, "Sprint forever (the Paramedic mission reward).");
    if (off::Fireproof >= 0) menu::Toggle("Fireproof", &g_fireproof.on, "You can't be set on fire.");
    menu::Toggle("Never wanted", &s.neverWanted, "Clears your wanted level every frame.");
    if (menu::Int("Wanted level", &s.wanted, 0, 6, 1, "Left/Right to choose, Enter to apply."))
    {
        int stars = s.wanted;
        Post([stars] { SetWanted(PlayerPed(), stars); });
    }
    if (menu::Action("Refill health & armor"))
        Post([] {
            if (void* p = PlayerPed())
            {
                Field<float>(p, off::Health) = MaxHealth();
                Field<float>(p, off::Armor) = MaxArmor();
            }
        });
    if (menu::Action("Add $10,000")) Post([] { Money() += 10000; });
    if (menu::Action("Add $1,000,000")) Post([] { Money() += 1000000; });
    if (menu::Action("Max money ($999,999,999)")) Post([] { Money() = 999999999; });
    CheatRows(kPlayerCheats);
#if defined(GAME_VC)
    menu::Submenu("Change skin", SkinMenu);
#endif
    menu::Toggle("Show coordinates", &s.showCoords);
}

void SpawnerMenu()
{
    static int group = 0;
    static const char* names[std::size(kVehicleGroups)];
    for (size_t i = 0; i < std::size(kVehicleGroups); i++) names[i] = kVehicleGroups[i].name;
    menu::Choice("Category", &group, names, (int)std::size(kVehicleGroups), "Left/Right to change category.");
    const VehicleGroup& g = kVehicleGroups[group];
    for (int i = 0; i < g.count; i++)
        if (menu::Action(g.models[i].name, "Spawns next to you.")) SpawnVehicle(g.models[i].id, g.models[i].name);
}

void VehicleCheatsMenu() { CheatRows(kVehicleCheats); }

void ColoursMenu()
{
    if (void* veh = PlayerVehicle(PlayerPed()))
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d / %d", Field<uint8_t>(veh, off::VehColour), Field<uint8_t>(veh, off::VehColour + 1));
        menu::Info("Current colours", buf);
    }
    if (menu::Int("Primary colour", &s.colour1, 0, kMaxColour, 1, "Left/Right to repaint."))
        PaintVehicle(s.colour1, s.colour2);
    if (menu::Int("Secondary colour", &s.colour2, 0, kMaxColour, 1, "Left/Right to repaint."))
        PaintVehicle(s.colour1, s.colour2);
    if (menu::Action("Random colours"))
    {
        s.colour1 = rand() % (kMaxColour + 1);
        s.colour2 = rand() % (kMaxColour + 1);
        PaintVehicle(s.colour1, s.colour2);
    }
    if (menu::Action("Black")) PaintVehicle(s.colour1 = 0, s.colour2 = 0);
    if (menu::Action("White")) PaintVehicle(s.colour1 = 1, s.colour2 = 1);
}

void VehicleMenu()
{
    menu::Submenu("Spawn vehicle", SpawnerMenu);
    if (void* veh = PlayerVehicle(PlayerPed()))
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f / 1000", Field<float>(veh, off::VehHealth));
        menu::Info("Vehicle health", buf);
    }
    menu::Toggle("Vehicle god mode", &s.vehGod, "Keeps engine health at 1000: no fire, no explosion.");
    menu::Toggle("Auto-repair", &s.vehAutoRepair, "Repairs your vehicle whenever it takes damage.");
    if (menu::Action("Repair vehicle")) RepairVehicle();
    if (menu::Action("Flip upright")) FlipVehicle();
    menu::Submenu("Paint", ColoursMenu);
    static char keysHelp[96];
    snprintf(keysHelp, sizeof(keysHelp), "%s = boost, %s = stop dead, %s = jump.", KeyName(g_keys.boost).c_str(),
             KeyName(g_keys.stop).c_str(), KeyName(g_keys.jump).c_str());
    menu::Toggle("Boost / stop / jump keys", &s.vehKeys, keysHelp);
    menu::Float("Boost power", &s.boost, 0.2f, 5.0f, 0.2f, "%.1fx");
    menu::Toggle("Show speedometer", &s.speedometer);
    menu::Submenu("Vehicle cheats", VehicleCheatsMenu);
}

void GiveWeaponMenu()
{
    static int group = 0;
    static const char* names[std::size(kWeaponGroups)];
    for (size_t i = 0; i < std::size(kWeaponGroups); i++) names[i] = kWeaponGroups[i].name;
    if (std::size(kWeaponGroups) > 1)
        menu::Choice("Category", &group, names, (int)std::size(kWeaponGroups), "Left/Right to change category.");
    const WeaponGroup& g = kWeaponGroups[group];
    for (int i = 0; i < g.count; i++)
        if (menu::Action(g.weapons[i].name, "Gives you this weapon with ammo.")) GiveWeapon(g.weapons[i]);
}

void WeaponsMenu()
{
    menu::Submenu("Give weapon", GiveWeaponMenu);
    menu::Toggle("Infinite ammo", &s.infAmmo, "Keeps your ammo topped up. You still reload.");
    menu::Toggle("Fast reload", &g_fastReload.on, "Reload much faster (a side-mission reward).");
    CheatRows(kWeaponCheats);
}

void PlacesMenu()
{
    for (const Place& p : kPlaces)
        if (menu::Action(p.name))
        {
            Teleport(p.x, p.y, p.z + 1.0f);
            menu::Notify("Teleported to %s", p.name);
        }
}

void TeleportMenu()
{
    void* ped = PlayerPed();
    if (ped)
    {
        void* veh = PlayerVehicle(ped);
        float* p = PosOf(veh ? veh : ped);
        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f, %.0f, %.0f", p[0], p[1], p[2]);
        menu::Info("You are at", buf);
    }
    static char wpHelp[80];
    snprintf(wpHelp, sizeof(wpHelp), "Mark a spot on the pause-menu map first. Hotkey: %s.", KeyName(g_keys.waypoint).c_str());
    if (menu::Action("Teleport to waypoint", wpHelp)) Post(TeleportToWaypoint);
    static char ncHelp[96];
    snprintf(ncHelp, sizeof(ncHelp), "Fly where the camera looks: WASD, Space/Ctrl up/down, Shift fast. Hotkey: %s.",
             KeyName(g_keys.noclip).c_str());
    menu::Toggle("No-clip (fly)", &s.noclip, ncHelp);
    menu::Float("No-clip speed", &s.noclipSpeed, 0.2f, 5.0f, 0.2f, "%.1fx");
    menu::Submenu("Places", PlacesMenu);
    for (int i = 0; i < 3; i++)
    {
        char label[48];
        snprintf(label, sizeof(label), "Save position %d", i + 1);
        if (menu::Action(label) && ped)
        {
            void* veh = PlayerVehicle(ped);
            float* p = PosOf(veh ? veh : ped);
            for (int k = 0; k < 3; k++) g_slots[i][k] = p[k];
            g_slotUsed[i] = true;
            wchar_t key[16], val[96];
            swprintf_s(key, L"Slot%d", i + 1);
            swprintf_s(val, L"%.2f,%.2f,%.2f", p[0], p[1], p[2]);
            SettingsSet(L"Teleport", key, val);
            menu::Notify("Saved position %d", i + 1);
        }
        snprintf(label, sizeof(label), "Go to position %d", i + 1);
        if (menu::Action(label))
        {
            if (g_slotUsed[i]) Teleport(g_slots[i][0], g_slots[i][1], g_slots[i][2] + 0.5f);
            else menu::Notify("Position %d is empty", i + 1);
        }
    }
}

void PedsMenu()
{
    menu::Toggle("Riot mode", &g_threats.riot, "Everyone fights everyone. Turn off to calm the streets again.");
    menu::Toggle("Everyone attacks you", &g_threats.attack, "Every ped is hostile to you.");
    CheatRows(kPedCheats);
    if (menu::Action("Reset peds to normal", "Fixes a save that still has riot mode from the game's cheat."))
        ResetPeds();
}

void WorldMenu()
{
    if (menu::Choice("Weather", &s.weather, kWeather, (int)std::size(kWeather), "Left/Right to choose, Enter to apply."))
    {
        int id = s.weather;
        Post([id] { SetWeatherNow(id); });
        menu::Notify("Weather: %s", kWeather[s.weather]);
    }
    menu::Toggle("Freeze weather", &s.freezeWeather, "Keeps the weather chosen above.");
    menu::Int("Hour", &s.hour, 0, 23);
    menu::Int("Minute", &s.minute, 0, 59, 5);
    if (menu::Action("Set time")) SetTime(s.hour, s.minute);
    menu::Toggle("Freeze time", &s.freezeTime, "Holds the clock at the hour and minute above.");
    menu::Float("Game speed", &s.speed, 0.1f, 3.0f, 0.1f, "%.1fx");
    menu::Toggle("Use custom game speed", &s.customSpeed);
    CheatRows(kWorldCheats);
    menu::Submenu("Peds", PedsMenu);
}

void SettingsMenu()
{
    menu::Info("Open / close menu", KeyName(menu::OpenKey()));
    menu::Info("Teleport to waypoint", KeyName(g_keys.waypoint));
    menu::Info("No-clip on / off", KeyName(g_keys.noclip));
    menu::Info("Vehicle boost / stop / jump",
               KeyName(g_keys.boost) + " / " + KeyName(g_keys.stop) + " / " + KeyName(g_keys.jump));
    menu::Info("Change the keys in", "TrilogyTrainer." GAME_SHORT ".ini");
    menu::Info("Version", "1.1 for game build 1.112");
}

void MainMenu()
{
    menu::Submenu("Player", PlayerMenu);
    menu::Submenu("Vehicle", VehicleMenu);
    menu::Submenu("Weapons", WeaponsMenu);
    menu::Submenu("Teleport", TeleportMenu);
    menu::Submenu("World", WorldMenu);
    menu::Submenu("Settings", SettingsMenu);
}

void Draw()
{
    menu::Frame();
    if (s.speedometer)
        if (void* veh = PlayerVehicle(PlayerPed()))
        {
            float* v = Velocity(veh);
            float kmh = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) * 50.0f * 3.6f;
            char buf[48];
            snprintf(buf, sizeof(buf), "%.0f km/h  |  %.0f mph", kmh, kmh * 0.6214f);
            ImGuiIO& io = ImGui::GetIO();
            float scale = io.DisplaySize.y / 1080.0f, size = 30.0f * scale;
            ImVec2 sz = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0, buf);
            ImVec2 pos(io.DisplaySize.x - sz.x - 40 * scale, io.DisplaySize.y - size * 3.2f);
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            dl->AddText(ImGui::GetFont(), size, ImVec2(pos.x + 2, pos.y + 2), IM_COL32(0, 0, 0, 200), buf);
            dl->AddText(ImGui::GetFont(), size, pos, IM_COL32(255, 255, 255, 240), buf);
        }
    if (!s.showCoords) return;
    if (void* ped = PlayerPed())
    {
        void* veh = PlayerVehicle(ped);
        float* p = PosOf(veh ? veh : ped);
        char buf[96];
        snprintf(buf, sizeof(buf), "X %.1f   Y %.1f   Z %.1f", p[0], p[1], p[2]);
        ImGuiIO& io = ImGui::GetIO();
        float scale = io.DisplaySize.y / 1080.0f, size = 22.0f * scale;
        ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), size, ImVec2(20 * scale, io.DisplaySize.y - size * 2),
                                                IM_COL32(255, 255, 255, 230), buf);
    }
}

// ---------- startup ----------

using TimerUpdateFn = void (*)();
TimerUpdateFn g_origTimerUpdate = nullptr;

void HookedTimerUpdate()
{
    g_origTimerUpdate();
    gt::Tick();
}

void LoadSlots()
{
    for (int i = 0; i < 3; i++)
    {
        wchar_t key[16];
        swprintf_s(key, L"Slot%d", i + 1);
        std::wstring v = SettingsGet(L"Teleport", key, L"");
        g_slotUsed[i] = !v.empty() && swscanf_s(v.c_str(), L"%f,%f,%f", &g_slots[i][0], &g_slots[i][1], &g_slots[i][2]) == 3;
    }
}

int HotkeySetting(const wchar_t* name, int fallback)
{
    std::wstring def;
    if (fallback == VK_ADD) def = L"ADD";
    else if (fallback == VK_SUBTRACT) def = L"SUBTRACT";
    else if (fallback == VK_MULTIPLY) def = L"MULTIPLY";
    else
    {
        std::string n = KeyName(fallback);
        def.assign(n.begin(), n.end());
    }
    std::wstring v = SettingsGet(L"Hotkeys", name, def.c_str());
    SettingsSet(L"Hotkeys", name, v);
    return ParseKeyName(v, fallback);
}

void LoadHotkeys()
{
    g_keys.waypoint = HotkeySetting(L"TeleportToWaypoint", g_keys.waypoint);
    g_keys.noclip = HotkeySetting(L"Noclip", g_keys.noclip);
    g_keys.boost = HotkeySetting(L"VehicleBoost", g_keys.boost);
    g_keys.stop = HotkeySetting(L"VehicleStop", g_keys.stop);
    g_keys.jump = HotkeySetting(L"VehicleJump", g_keys.jump);
}

bool VerifyBuild()
{
#if defined(GAME_VC)
    return CheckBytes(addr::TimerUpdate, "48 89 5c 24 08 57 48 83 ec 40 8b 05 d8 67 04 04") &&
           CheckBytes(addr::VehicleCheat, "40 53 57 41 55 41 56 41 57 48 81 ec e0 00 00 00") &&
           CheckBytes(addr::SetGameClock, "8b 05 e2 eb 08 04 44 0f b6 ca 89 05 40 58 ff 03") &&
           CheckBytes(0x10AA116, "0f b6 05 0b db 19 04 48 8d 0d 6c d9 19 04");
#else
    return CheckBytes(addr::TimerUpdate, "48 89 5c 24 08 57 48 83 ec 50 8b 05 24 3d f9 03") &&
           CheckBytes(addr::TankCheat, "48 89 5c 24 18 55 57 41 56 48 8d 6c 24 b9 48 81") &&
           CheckBytes(addr::TankModelImm - 6, "c7 05 4b 01 ce 03 7a 00 00 00") &&
           CheckBytes(addr::SetGameClock, "8b 05 3e 60 f8 03 89 05 28 40 f8 03 33 c0 66 89") &&
           CheckBytes(addr::FixAutomobile, "48 89 5c 24 10 48 89 74 24 18 57 48 81 ec b0 00");
#endif
}

DWORD WINAPI Start(LPVOID)
{
    g_base = (uintptr_t)GetModuleHandleW(nullptr);
    Log("%s trainer starting, game base %p", GAME_TITLE, (void*)g_base);
    if (!VerifyBuild())
    {
        Log("This executable is not build 1.112. Trainer disabled.");
        return 0;
    }

    std::wstring keyName = SettingsGet(L"Menu", L"OpenKey", L"F11");
    SettingsSet(L"Menu", L"OpenKey", keyName);
    int key = ParseKeyName(keyName, VK_F11);
    menu::SetOpenKey(key);
    menu::Init(GAME_TITLE, MainMenu);
    LoadSlots();
    LoadHotkeys();
    gt::SetPerFrame(PerFrame);

    if (MH_Initialize() != MH_OK)
    {
        Log("MinHook failed to initialise.");
        return 0;
    }
    if (MH_CreateHook((void*)(g_base + addr::TimerUpdate), (void*)&HookedTimerUpdate, (void**)&g_origTimerUpdate) != MH_OK)
        Log("Couldn't hook the game tick.");
    render::SetDrawCallback(Draw);
    if (!render::InstallHooks()) Log("Couldn't hook rendering.");
    MH_EnableHook(MH_ALL_HOOKS);
    menu::Notify("%s trainer loaded. Press %ls to open.", GAME_TITLE, keyName.c_str());
    Log("Ready. Menu key 0x%02X.", key);
    return 0;
}
} // namespace classic

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        LogInit(module);
        core::SettingsInit(module);
        if (HANDLE t = CreateThread(nullptr, 0, classic::Start, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
