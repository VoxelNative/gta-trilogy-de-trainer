// GTA San Andreas - The Definitive Edition (1.112) in-game trainer.
// Every address below was reverse-engineered from SanAndreas.exe 1.0.112.48699928.
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
#include "sa_data.h"

using namespace core;

namespace sa
{
// ---------- addresses (RVAs) ----------
namespace addr
{
constexpr uintptr_t PlayerInFocus = 0x522BCF6; // uint8
constexpr uintptr_t Players = 0x53EB720;       // CPlayerInfo[2], 0x1C0 each
constexpr uintptr_t TimerUpdate = 0x1186920;   // CTimer::Update, once per frame
constexpr uintptr_t TimeScale = 0x523A2DC;     // float CTimer::ms_fTimeScale
constexpr uintptr_t CheatTable = 0x43064C8;    // void (*[])()
constexpr uintptr_t CheatActive = 0x51BD0BB;   // uint8 CCheat::m_aCheatsActive[]
constexpr uintptr_t VehicleCheat = 0x107E950;  // CVehicle* (int model): spawns in front of the player
constexpr uintptr_t SetGameClock = 0x112CE10;  // void (uint8 hours, uint8 minutes, uint8 day)
constexpr uintptr_t ClockHours = 0x521C917;    // uint8
constexpr uintptr_t ClockMinutes = 0x521C923;  // uint8
constexpr uintptr_t ClockDay = 0x521C922;      // uint8
constexpr uintptr_t WeatherOld = 0x5300FEC;    // int16
constexpr uintptr_t WeatherNew = 0x5300FF0;    // int16
constexpr uintptr_t WeatherInterp = 0x5300FF4; // float
constexpr uintptr_t WeatherForced = 0x5301008; // int16
constexpr uintptr_t TimeStep = 0x523A2D8;      // float CTimer::ms_fTimeStep (1.0 at 50 fps)
constexpr uintptr_t CameraMatrix = 0x53E23E8;  // CMatrix* TheCamera.m_matrix
constexpr uintptr_t RadarTrace = 0x542A7B0;    // tRadarTrace[250], 0x30 each
constexpr uintptr_t FindGroundZ = 0x1191AC0;   // float CWorld::FindGroundZFor3DCoord(x, y, z, bool*, CEntity**)
constexpr uintptr_t RequestModel = 0x1288560;  // void CStreaming::RequestModel(int model, int flags)
constexpr uintptr_t LoadAllRequested = 0x128A360; // void CStreaming::LoadAllRequestedModels(bool)
constexpr uintptr_t GiveWeapon = 0x1222990;    // void CPed::GiveWeapon(CPed*, int type, int ammo)
constexpr uintptr_t UpdateColours = 0x13DCAC0; // void (CVehicle*): pushes new paint to the renderer
constexpr uintptr_t StatsFloat = 0x522C740;    // float CStats::StatTypesFloat[] (ids 0-119)
constexpr uintptr_t StatsInt = 0x522C890;      // int CStats::StatTypesInt[] (ids 120+)
constexpr uintptr_t StatsChanged = 0x117EF60;  // void (bool), called by the game after every stat cheat
constexpr uintptr_t StatUpdated = 0x117A7E0;   // void (uint16 stat)
constexpr uintptr_t RebuildPlayer = 0x103ADD0; // void CClothes::RebuildPlayer(CPlayerPed*, bool)
} // namespace addr

namespace off
{
constexpr uintptr_t InfoStride = 0x1C0, InfoMoney = 0xE8, InfoMaxHealth = 0x183, InfoMaxArmor = 0x184;
constexpr uintptr_t PedHealth = 0x76C, PedArmor = 0x774, PedFlags = 0x634, PedVehicle = 0x7C8, PedData = 0x5F0;
constexpr uintptr_t PedWeapons = 0x7F0, WeaponStride = 0x28, WeaponTotal = 0xC;
constexpr uintptr_t WantedChaos = 0x0, WantedLevel = 0x2C;
constexpr uintptr_t EntMatrix = 0x18, EntPlacement = 0x8, MatrixPos = 0x30;
constexpr uintptr_t VehHealth = 0x79C, VtblFix = 0x1C8;
constexpr uintptr_t MoveSpeed = 0x7C, TurnSpeed = 0x88; // CPhysical velocity, in units per 1/50 s
constexpr uintptr_t VtblTeleport = 0x70;                // CEntity::Teleport(CVector*, bool)
constexpr uintptr_t VehColour = 0x67C;                  // uint8 primary, secondary, tertiary, quaternary
constexpr uintptr_t PedState = 0x5FC;
constexpr int InfoNoTired = 0x180, InfoFastReload = 0x181, InfoFireproof = 0x182; // script-command perks
constexpr int RadarTraceCount = 250, RadarTraceStride = 0x30;
} // namespace off

constexpr int kChaosForStars[7] = {0, 70, 200, 570, 1220, 2420, 4620};

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
    if (!ped || !(Field<uint32_t>(ped, off::PedFlags) & 0x100)) return nullptr;
    return Field<void*>(ped, off::PedVehicle);
}

void* Wanted(void* ped)
{
    void* data = ped ? Field<void*>(ped, off::PedData) : nullptr;
    return data ? *reinterpret_cast<void**>(data) : nullptr;
}

float* PosOf(void* entity)
{
    void* m = Field<void*>(entity, off::EntMatrix);
    return m ? &Field<float>(m, off::MatrixPos) : &Field<float>(entity, off::EntPlacement);
}

float MaxHealth() { return (float)std::max(100, (int)PlayerInfo()[off::InfoMaxHealth]); }
float MaxArmor() { return (float)std::max(100, (int)PlayerInfo()[off::InfoMaxArmor]); }
int& Money() { return *reinterpret_cast<int*>(PlayerInfo() + off::InfoMoney); }

void SetWanted(void* ped, int stars)
{
    if (void* w = Wanted(ped))
    {
        stars = std::clamp(stars, 0, 6);
        Field<int>(w, off::WantedChaos) = kChaosForStars[stars];
        Field<int>(w, off::WantedLevel) = stars;
    }
}

float TimeStep() { return std::clamp(At<float>(addr::TimeStep), 0.01f, 3.0f); }

float* Velocity(void* e) { return &Field<float>(e, off::MoveSpeed); }

// Zeroes linear and angular velocity.
void Stop(void* e)
{
    float* v = Velocity(e);
    for (int i = 0; i < 6; i++) v[i] = 0.0f;
}

float* ForwardOf(void* e)
{
    void* m = Field<void*>(e, off::EntMatrix);
    return m ? &Field<float>(m, 0x10) : nullptr;
}

// Where the camera looks (falls back to the entity's own facing).
void CameraForward(void* fallback, float out[3])
{
    void* m = At<void*>(addr::CameraMatrix);
    const float* f = m ? &Field<float>(m, 0x10) : ForwardOf(fallback);
    if (!f) { out[0] = 0; out[1] = 1; out[2] = 0; return; }
    for (int i = 0; i < 3; i++) out[i] = f[i];
}

// The game's own teleport: moves the entity between world sectors and resets its physics.
void TeleportEntity(void* e, float x, float y, float z)
{
    float v[3] = {x, y, z};
    void** vtbl = *reinterpret_cast<void***>(e);
    reinterpret_cast<void (*)(void*, float*, bool)>(vtbl[off::VtblTeleport / 8])(e, v, false);
}

float GroundZ(float x, float y, bool* found)
{
    *found = false;
    return Fn<float (*)(float, float, float, bool*, void*, void*)>(addr::FindGroundZ)(x, y, 1000.0f, found, nullptr,
                                                                                    nullptr);
}

// The marker placed on the pause-menu map: an in-use radar blip with the remaster's waypoint flag.
bool FindWaypoint(float& x, float& y)
{
    for (int i = 0; i < off::RadarTraceCount; i++)
    {
        uint8_t* t = &At<uint8_t>(addr::RadarTrace + i * off::RadarTraceStride);
        if ((t[0x29] & 2) && (t[0x2A] & 0x40))
        {
            x = *reinterpret_cast<float*>(t + 8);
            y = *reinterpret_cast<float*>(t + 12);
            return true;
        }
    }
    return false;
}

float GetStat(int id)
{
    return id < 120 ? At<float>(addr::StatsFloat + id * 4) : (float)At<int>(addr::StatsInt + (id - 120) * 4);
}

void SetStat(int id, float value)
{
    if (id < 120) At<float>(addr::StatsFloat + id * 4) = value;
    else At<int>(addr::StatsInt + (id - 120) * 4) = (int)value;
    Fn<void (*)(bool)>(addr::StatsChanged)(false);
    Fn<void (*)(uint16_t)>(addr::StatUpdated)((uint16_t)id);
}

using CheatFn = void (*)();
CheatFn CheatFunction(int i) { return reinterpret_cast<CheatFn*>(g_base + addr::CheatTable)[i]; }

// The flag a cheat flips, if it's a toggle: either the game's m_aCheatsActive slot (cheats with no
// handler), or the byte its handler tests with "cmp byte [rip+X], 0" in its first instructions.
uint8_t* CheatFlag(int i)
{
    CheatFn fn = CheatFunction(i);
    if (!fn) return &At<uint8_t>(addr::CheatActive + i);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(fn);
    for (int k = 0; k < 12; k++)
        if (p[k] == 0x80 && p[k + 1] == 0x3D && p[k + 6] == 0x00)
            return const_cast<uint8_t*>(p + k + 7 + *reinterpret_cast<const int32_t*>(p + k + 2));
    return nullptr;
}

void RunCheat(int i)
{
    if (CheatFn fn = CheatFunction(i)) fn();
    else At<uint8_t>(addr::CheatActive + i) ^= 1;
}

// ---------- trainer state ----------

struct State
{
    bool god = false, armor = false, neverWanted = false, infAmmo = false;
    bool vehGod = false, vehAutoRepair = false, vehKeys = true, speedometer = false;
    bool freezeTime = false, freezeWeather = false, customSpeed = false, showCoords = false;
    bool noclip = false;
    int wanted = 0, hour = 12, minute = 0, weather = 1, colour1 = 0, colour2 = 0;
    float speed = 1.0f, noclipSpeed = 1.0f, boost = 1.0f;
} s;

struct Hotkeys
{
    int waypoint = VK_F5, noclip = VK_F6, boost = VK_ADD, stop = VK_SUBTRACT, jump = VK_MULTIPLY;
} g_keys;

float g_slots[3][3];
bool g_slotUsed[3];

// A player perk flag (rewards for side missions). Turning one off restores whatever the game had,
// so a perk you earned yourself isn't taken away.
struct Perk
{
    int offset;
    bool on = false;
    bool applied = false;
    uint8_t saved = 0;

    void Apply()
    {
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

Perk g_sprint{off::InfoNoTired}, g_fastReload{off::InfoFastReload}, g_fireproof{off::InfoFireproof};

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
        float f[3];
        CameraForward(e, f);
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
    TeleportEntity(e, x, y, 300.0f);
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
        TeleportEntity(e, g_wp.x, g_wp.y, found ? z + 1.0f : 20.0f);
        g_wp.active = false;
        menu::Notify(found ? "Teleported to the waypoint" : "Teleported (couldn't find the ground there)");
        return;
    }
    float* p = PosOf(e);
    p[0] = g_wp.x;
    p[1] = g_wp.y;
    p[2] = 300.0f;
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
    if (KeyDown(g_keys.boost))
        if (float* f = ForwardOf(veh))
            for (int i = 0; i < 3; i++) v[i] += f[i] * 0.025f * s.boost * TimeStep();
    if (KeyPressed(g_keys.stop)) Stop(veh);
    if (KeyPressed(g_keys.jump)) v[2] += 0.25f;
}

void PerFrame()
{
    void* ped = PlayerPed();
    if (!ped) return;
    g_sprint.Apply();
    g_fastReload.Apply();
    g_fireproof.Apply();
    NoclipFrame(ped);
    WaypointFrame(ped);
    HotkeyFrame(ped);
    if (s.god && Field<float>(ped, off::PedHealth) < MaxHealth()) Field<float>(ped, off::PedHealth) = MaxHealth();
    if (s.armor && Field<float>(ped, off::PedArmor) < MaxArmor()) Field<float>(ped, off::PedArmor) = MaxArmor();
    if (s.neverWanted) SetWanted(ped, 0);
    if (s.infAmmo)
        for (int i = 0; i < 13; i++)
        {
            uint8_t* slot = (uint8_t*)ped + off::PedWeapons + i * off::WeaponStride;
            int& total = *reinterpret_cast<int*>(slot + off::WeaponTotal);
            if (*reinterpret_cast<int*>(slot) > 0 && total > 0 && total < 9000) total = 9999;
        }
    if (void* veh = PlayerVehicle(ped))
    {
        float& hp = Field<float>(veh, off::VehHealth);
        if (s.vehAutoRepair && hp < 1000.0f)
        {
            void** vtbl = *reinterpret_cast<void***>(veh);
            reinterpret_cast<void (*)(void*)>(vtbl[off::VtblFix / 8])(veh);
            hp = 1000.0f;
        }
        else if (s.vehGod && hp < 1000.0f)
            hp = 1000.0f;
    }
    if (s.freezeTime)
    {
        At<uint8_t>(addr::ClockHours) = (uint8_t)s.hour;
        At<uint8_t>(addr::ClockMinutes) = (uint8_t)s.minute;
    }
    if (s.freezeWeather)
    {
        At<int16_t>(addr::WeatherOld) = At<int16_t>(addr::WeatherNew) = At<int16_t>(addr::WeatherForced) = (int16_t)s.weather;
        At<float>(addr::WeatherInterp) = 0.0f;
    }
    if (s.customSpeed) At<float>(addr::TimeScale) = s.speed;
}

// ---------- actions (always run on the game thread) ----------

void Post(std::function<void()> fn) { gt::Post(std::move(fn)); }

void SpawnVehicle(int model, const char* name)
{
    Post([model, name] {
        if (!PlayerPed()) return;
        reinterpret_cast<void* (*)(int)>(g_base + addr::VehicleCheat)(model);
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
        Fn<void (*)(int, int)>(addr::RequestModel)(copy.model, 2);
        if (copy.type == 39) Fn<void (*)(int, int)>(addr::RequestModel)(364, 2); // satchels come with a detonator
        Fn<void (*)(bool)>(addr::LoadAllRequested)(false);
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

void ChangeStat(int id, int value)
{
    Post([id, value] {
        SetStat(id, (float)value);
        void* ped = PlayerPed();
        // Fat and muscle change CJ's body, which the game rebuilds (not while driving).
        if ((id == 21 || id == 23) && ped && Field<int>(ped, off::PedState) != 50)
            Fn<void (*)(void*, bool)>(addr::RebuildPlayer)(ped, false);
    });
}

void SetWeather(int id)
{
    Post([id] {
        At<int16_t>(addr::WeatherOld) = At<int16_t>(addr::WeatherNew) = At<int16_t>(addr::WeatherForced) = (int16_t)id;
        At<float>(addr::WeatherInterp) = 0.0f;
    });
}

void SetTime(int h, int m)
{
    Post([h, m] {
        reinterpret_cast<void (*)(uint8_t, uint8_t, uint8_t)>(g_base + addr::SetGameClock)(
            (uint8_t)h, (uint8_t)m, At<uint8_t>(addr::ClockDay));
    });
}

void Cheat(int index, const char* name)
{
    Post([index, name] {
        RunCheat(index);
        uint8_t* flag = CheatFlag(index);
        if (flag) menu::Notify("%s: %s", name, *flag ? "ON" : "OFF");
        else menu::Notify("%s", name);
    });
}

void RepairVehicle()
{
    Post([] {
        void* veh = PlayerVehicle(PlayerPed());
        if (!veh) { menu::Notify("You're not in a vehicle"); return; }
        void** vtbl = *reinterpret_cast<void***>(veh);
        reinterpret_cast<void (*)(void*)>(vtbl[off::VtblFix / 8])(veh);
        Field<float>(veh, off::VehHealth) = 1000.0f;
        menu::Notify("Vehicle repaired");
    });
}

void FlipVehicle()
{
    Post([] {
        void* veh = PlayerVehicle(PlayerPed());
        if (!veh) { menu::Notify("You're not in a vehicle"); return; }
        void* m = Field<void*>(veh, off::EntMatrix);
        if (!m) return;
        float* right = &Field<float>(m, 0x00);
        float* fwd = &Field<float>(m, 0x10);
        float* up = &Field<float>(m, 0x20);
        float fx = fwd[0], fy = fwd[1];
        float len = sqrtf(fx * fx + fy * fy);
        if (len < 0.01f) { fx = 0; fy = 1; len = 1; }
        fx /= len;
        fy /= len;
        fwd[0] = fx; fwd[1] = fy; fwd[2] = 0;
        up[0] = 0; up[1] = 0; up[2] = 1;
        right[0] = fy; right[1] = -fx; right[2] = 0; // forward x up
        Field<float>(m, off::MatrixPos + 8) += 1.0f;
        menu::Notify("Vehicle flipped upright");
    });
}

// ---------- menus ----------

void CheatRows(const CheatInfo* list, int count)
{
    for (int i = 0; i < count; i++)
    {
        uint8_t* flag = CheatFlag(list[i].index);
        bool hit = flag ? menu::ToggleState(list[i].name, *flag != 0) : menu::Action(list[i].name);
        if (hit) Cheat(list[i].index, list[i].name);
    }
}

void StatRows(const StatInfo* list, int count)
{
    for (int i = 0; i < count; i++)
    {
        int v = (int)GetStat(list[i].id);
        if (menu::Int(list[i].name, &v, 0, 1000, 50, "Left/Right to change (0 - 1000).")) ChangeStat(list[i].id, v);
    }
}

void BodyStatsMenu() { StatRows(kBodyStats, (int)std::size(kBodyStats)); }
void DrivingStatsMenu() { StatRows(kDrivingStats, (int)std::size(kDrivingStats)); }
void WeaponStatsMenu()
{
    StatRows(kWeaponStats, (int)std::size(kWeaponStats));
    if (menu::Action("Max all weapon skills (hitman)"))
        for (const StatInfo& st : kWeaponStats) ChangeStat(st.id, 1000);
}

void StatsMenu()
{
    menu::Submenu("Body & respect", BodyStatsMenu);
    menu::Submenu("Driving & flying skills", DrivingStatsMenu);
    menu::Submenu("Weapon skills", WeaponStatsMenu);
    CheatRows(kStatCheats, (int)std::size(kStatCheats));
}

void PlayerMenu()
{
    void* ped = PlayerPed();
    if (ped)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f / %.0f", Field<float>(ped, off::PedHealth), Field<float>(ped, off::PedArmor));
        menu::Info("Health / armor", buf);
        snprintf(buf, sizeof(buf), "$%d", Money());
        menu::Info("Money", buf);
    }
    menu::Toggle("God mode", &s.god, "Keeps your health full every frame.");
    menu::Toggle("Infinite armor", &s.armor);
    menu::Toggle("Infinite stamina", &g_sprint.on, "Sprint forever (the Paramedic mission reward).");
    menu::Toggle("Fireproof", &g_fireproof.on, "You can't be set on fire (the Firefighter reward).");
    if (menu::Toggle("Never wanted", &s.neverWanted, "Clears your wanted level every frame.") && !s.neverWanted)
        s.wanted = 0;
    if (menu::Int("Wanted level", &s.wanted, 0, 6, 1, "Left/Right to choose, Enter to apply."))
    {
        int stars = s.wanted;
        Post([stars] { SetWanted(PlayerPed(), stars); });
    }
    if (menu::Action("Refill health & armor"))
        Post([] {
            if (void* p = PlayerPed())
            {
                Field<float>(p, off::PedHealth) = MaxHealth();
                Field<float>(p, off::PedArmor) = MaxArmor();
            }
        });
    if (menu::Action("Add $10,000")) Post([] { Money() += 10000; });
    if (menu::Action("Add $1,000,000")) Post([] { Money() += 1000000; });
    if (menu::Action("Max money ($999,999,999)")) Post([] { Money() = 999999999; });
    CheatRows(kPlayerCheats, (int)std::size(kPlayerCheats));
    menu::Submenu("Stats & skills", StatsMenu);
    menu::Toggle("Show coordinates", &s.showCoords);
}

void VehicleCheatsMenu() { CheatRows(kVehicleCheats, (int)std::size(kVehicleCheats)); }

// The first row picks the category with Left/Right; the rows below are that category's vehicles.
void SpawnerMenu()
{
    static int group = 0;
    static const char* names[std::size(kVehicleGroups)];
    for (size_t i = 0; i < std::size(kVehicleGroups); i++) names[i] = kVehicleGroups[i].name;
    menu::Choice("Category", &group, names, (int)std::size(kVehicleGroups), "Left/Right to change category.");
    const VehicleGroup& g = kVehicleGroups[group];
    for (int i = 0; i < g.count; i++)
        if (menu::Action(g.models[i].name, "Spawns in front of you.")) SpawnVehicle(g.models[i].id, g.models[i].name);
}

void ColoursMenu()
{
    if (void* veh = PlayerVehicle(PlayerPed()))
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d / %d", Field<uint8_t>(veh, off::VehColour), Field<uint8_t>(veh, off::VehColour + 1));
        menu::Info("Current colours", buf);
    }
    if (menu::Int("Primary colour", &s.colour1, 0, 126, 1, "Left/Right to repaint.")) PaintVehicle(s.colour1, s.colour2);
    if (menu::Int("Secondary colour", &s.colour2, 0, 126, 1, "Left/Right to repaint.")) PaintVehicle(s.colour1, s.colour2);
    if (menu::Action("Random colours"))
    {
        s.colour1 = rand() % 127;
        s.colour2 = rand() % 127;
        PaintVehicle(s.colour1, s.colour2);
    }
    if (menu::Action("Black")) PaintVehicle(s.colour1 = 0, s.colour2 = 0);
    if (menu::Action("White")) PaintVehicle(s.colour1 = 1, s.colour2 = 1);
}

void VehicleMenu()
{
    menu::Submenu("Spawn vehicle", SpawnerMenu);
    void* veh = PlayerVehicle(PlayerPed());
    if (veh)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f / 1000", Field<float>(veh, off::VehHealth));
        menu::Info("Vehicle health", buf);
    }
    menu::Toggle("Vehicle god mode", &s.vehGod, "Keeps engine health at 1000: no fire, no explosion.");
    menu::Toggle("Auto-repair", &s.vehAutoRepair, "Fully repairs your vehicle (dents too) whenever it takes damage.");
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
    CheatRows(kWeaponCheats, (int)std::size(kWeaponCheats));
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

void TrafficMenu() { CheatRows(kTrafficCheats, (int)std::size(kTrafficCheats)); }
void PedsMenu() { CheatRows(kPedCheats, (int)std::size(kPedCheats)); }

void WorldMenu()
{
    if (menu::Choice("Weather", &s.weather, kWeather, (int)std::size(kWeather), "Left/Right to choose, Enter to apply."))
    {
        SetWeather(s.weather);
        menu::Notify("Weather: %s", kWeather[s.weather]);
    }
    menu::Toggle("Freeze weather", &s.freezeWeather, "Keeps the weather chosen above.");
    menu::Int("Hour", &s.hour, 0, 23);
    menu::Int("Minute", &s.minute, 0, 59, 5);
    if (menu::Action("Set time")) SetTime(s.hour, s.minute);
    menu::Toggle("Freeze time", &s.freezeTime, "Holds the clock at the hour and minute above.");
    menu::Float("Game speed", &s.speed, 0.1f, 3.0f, 0.1f, "%.1fx");
    menu::Toggle("Use custom game speed", &s.customSpeed);
    CheatRows(kWorldCheats, (int)std::size(kWorldCheats));
    menu::Submenu("Traffic", TrafficMenu);
    menu::Submenu("Peds & gangs", PedsMenu);
}

void SettingsMenu()
{
    menu::Info("Open / close menu", KeyName(menu::OpenKey()));
    menu::Info("Teleport to waypoint", KeyName(g_keys.waypoint));
    menu::Info("No-clip on / off", KeyName(g_keys.noclip));
    menu::Info("Vehicle boost / stop / jump",
               KeyName(g_keys.boost) + " / " + KeyName(g_keys.stop) + " / " + KeyName(g_keys.jump));
    menu::Info("Change the keys in", "TrilogyTrainer.SA.ini");
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
    if (s.showCoords)
        if (void* ped = PlayerPed())
        {
            void* veh = PlayerVehicle(ped);
            float* p = PosOf(veh ? veh : ped);
            char buf[96];
            snprintf(buf, sizeof(buf), "X %.1f   Y %.1f   Z %.1f", p[0], p[1], p[2]);
            ImGuiIO& io = ImGui::GetIO();
            float size = 22.0f * io.DisplaySize.y / 1080.0f;
            ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), size,
                                                    ImVec2(20 * io.DisplaySize.y / 1080.0f, io.DisplaySize.y - size * 2),
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

DWORD WINAPI Start(LPVOID)
{
    g_base = (uintptr_t)GetModuleHandleW(nullptr);
    Log("GTA SA trainer starting, game base %p", (void*)g_base);

    bool ok = CheckBytes(addr::TimerUpdate, "48 83 ec 58 48 8b 0d 9d b2 0b 04 48 85 c9 0f 84") &&
              CheckBytes(addr::VehicleCheat, "40 56 57 41 57 48 83 ec 40 0f b6 05 96 d3 1a 04") &&
              CheckBytes(addr::SetGameClock, "8b 05 42 d6 10 04 44 0f b6 15 f8 fa 0e 04 44 0f") &&
              CheckBytes(0x107E170, "0f b6 15 7f db 1a 04 4c 8d 05 a2 d5 36 04 48 69 ca c0 01 00 00");
    if (!ok)
    {
        Log("This SanAndreas.exe is not build 1.112. Trainer disabled.");
        return 0;
    }

    std::wstring keyName = SettingsGet(L"Menu", L"OpenKey", L"F11");
    SettingsSet(L"Menu", L"OpenKey", keyName); // writes the default so it's easy to find and edit
    int key = ParseKeyName(keyName, VK_F11);
    menu::SetOpenKey(key);
    menu::Init("SAN ANDREAS", MainMenu);
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
    menu::Notify("San Andreas trainer loaded. Press %ls to open.", keyName.c_str());
    Log("Ready. Menu key 0x%02X.", key);
    return 0;
}
} // namespace sa

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        LogInit(module);
        core::SettingsInit(module);
        if (HANDLE t = CreateThread(nullptr, 0, sa::Start, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
