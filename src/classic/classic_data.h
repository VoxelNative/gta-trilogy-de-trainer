#pragma once
// Static data for the Vice City and GTA III trainers (selected with GAME_VC / GAME_III).

namespace classic
{
struct VehicleModel
{
    int id;
    const char* name;
};

struct VehicleGroup
{
    const char* name;
    const VehicleModel* models;
    int count;
};

struct CheatInfo
{
    unsigned rva; // handler in the game exe
    const char* name;
};

struct Place
{
    const char* name;
    float x, y, z;
};

// Weapon type -> model to stream in first (0 = none needed).
struct WeaponModel
{
    int type, model, ammo;
    const char* name;
};

struct WeaponGroup
{
    const char* name;
    const WeaponModel* weapons;
    int count;
};

#define CL_GROUP(name, arr) {name, arr, (int)(sizeof(arr) / sizeof(arr[0]))}

#if defined(GAME_VC)
// ------------------------------------------------------------------ Vice City
inline const VehicleModel kCars[] = {
    {141, "Infernus"}, {145, "Cheetah"}, {159, "Banshee"}, {132, "Stinger"}, {210, "Comet"}, {211, "Deluxo"},
    {207, "Phoenix"}, {206, "Sabre Turbo"}, {224, "Hotring Racer"}, {232, "Hotring Racer 2"}, {233, "Hotring Racer 3"},
    {234, "Bloodring Banger"}, {235, "Bloodring Banger 2"}, {236, "Vice Squad Cheetah"}, {174, "Sentinel XS"},
    {130, "Landstalker"}, {131, "Idaho"}, {134, "Perennial"}, {135, "Sentinel"}, {136, "Rio"}, {140, "Manana"},
    {142, "Voodoo"}, {148, "Moonbeam"}, {149, "Esperanto"}, {151, "Washington"}, {152, "Bobcat"},
    {154, "BF Injection"}, {164, "Cuban Hermes"}, {169, "Stallion"}, {175, "Admiral"}, {196, "Glendale"},
    {197, "Oceanic"}, {200, "Patriot"}, {204, "Hermes"}, {205, "Sabre"}, {208, "Walton"}, {209, "Regina"},
    {219, "Rancher"}, {221, "Virgo"}, {222, "Greenwood"}, {225, "Sandking"}, {226, "Blista Compact"},
    {230, "Mesa"}, {139, "Stretch"}, {201, "Love Fist"}, {172, "Romero's Hearse"}, {187, "Caddy"},
};
inline const VehicleModel kService[] = {
    {156, "Police"}, {147, "FBI Washington"}, {220, "FBI Rancher"}, {157, "Enforcer"}, {162, "Rhino"},
    {163, "Barracks OL"}, {137, "Firetruck"}, {146, "Ambulance"}, {150, "Taxi"}, {168, "Cabbie"},
    {188, "Zebra Cab"}, {216, "Kaufman Cab"}, {161, "Bus"}, {167, "Coach"}, {158, "Securicar"},
};
inline const VehicleModel kWork[] = {
    {133, "Linerunner"}, {138, "Trashmaster"}, {143, "Pony"}, {144, "Mule"}, {153, "Mr Whoopee"},
    {170, "Rumpo"}, {173, "Packer"}, {179, "Gang Burrito"}, {185, "Flatbed"}, {186, "Yankee"},
    {189, "Top Fun"}, {212, "Burrito"}, {213, "Spand Express"}, {215, "Baggage Handler"}, {228, "Boxville"},
    {229, "Benson"},
};
inline const VehicleModel kBikes[] = {
    {166, "Angel"}, {178, "Pizzaboy"}, {191, "PCJ 600"}, {192, "Faggio"}, {193, "Freeway"}, {198, "Sanchez"},
};
inline const VehicleModel kAir[] = {
    {155, "Hunter"}, {177, "Sea Sparrow"}, {199, "Sparrow"}, {217, "Maverick"}, {218, "VCN Maverick"},
    {227, "Police Maverick"}, {190, "Skimmer"},
};
inline const VehicleModel kBoats[] = {
    {160, "Predator"}, {176, "Squalo"}, {182, "Speeder"}, {183, "Reefer"}, {184, "Tropic"},
    {202, "Coast Guard"}, {203, "Dinghy"}, {214, "Marquis"}, {223, "Cuban Jetmax"},
};
inline const VehicleModel kRc[] = {{171, "RC Bandit"}, {194, "RC Baron"}, {195, "RC Raider"}, {231, "RC Goblin"}};

inline const VehicleGroup kVehicleGroups[] = {
    CL_GROUP("Cars", kCars),          CL_GROUP("Emergency & public", kService), CL_GROUP("Vans & trucks", kWork),
    CL_GROUP("Bikes", kBikes),        CL_GROUP("Helicopters & planes", kAir),   CL_GROUP("Boats", kBoats),
    CL_GROUP("RC vehicles", kRc),
};

inline const char* const kWeather[] = {"Sunny", "Cloudy", "Rainy", "Foggy", "Extra sunny", "Hurricane", "Extra colours"};

inline const CheatInfo kPlayerCheats[] = {
    {0x10A9020, "Full health (+ repair vehicle)"}, {0x10AA0A0, "Full armor"}, {0x10AA1D0, "Wanted level +2"},
    {0x10AA290, "Clear wanted level"}, {0x10AAB80, "Women follow you"}, {0x10AA7D0, "Suicide"},
};
inline const CheatInfo kWeaponCheats[] = {
    {0x10A87D0, "Weapon set 1 (thug)"}, {0x10A8AB0, "Weapon set 2 (professional)"}, {0x10A8D60, "Weapon set 3 (nutter)"},
};
inline const CheatInfo kVehicleCheats[] = {
    {0x10AA6F0, "Flying cars"}, {0x10AA730, "Better handling"}, {0x10AAB70, "Flying boats"},
    {0x10AA890, "Cars drive on water"}, {0x10AA6D0, "Invisible cars"}, {0x10AA8A0, "Bigger wheels"},
    {0x10AA830, "All traffic lights green"}, {0x10AA840, "Aggressive traffic"}, {0x10AA850, "Pink traffic"},
    {0x10AA870, "Black traffic"}, {0x10A9BA0, "Blow up all cars"},
};
inline const CheatInfo kPedCheats[] = {
    {0x10AA030, "Peds have weapons"},
    {0x10AA770, "Women carry guns"},
};
inline const CheatInfo kWorldCheats[] = {
    {0x10AA6C0, "Faster clock"}, {0x10AA050, "Faster gameplay"}, {0x10AA070, "Slower gameplay"},
};

inline const WeaponModel kMelee[] = {
    {1, 259, 1, "Brass knuckles"}, {2, 260, 1, "Screwdriver"}, {3, 261, 1, "Golf club"}, {4, 262, 1, "Nightstick"},
    {5, 263, 1, "Knife"}, {6, 264, 1, "Baseball bat"}, {7, 265, 1, "Hammer"}, {8, 266, 1, "Meat cleaver"},
    {9, 267, 1, "Machete"}, {10, 268, 1, "Katana"}, {11, 269, 1, "Chainsaw"},
};
inline const WeaponModel kPistols[] = {{17, 274, 500, "Pistol"}, {18, 275, 500, "Python"}};
inline const WeaponModel kShotguns[] = {{19, 277, 300, "Shotgun"}, {20, 278, 300, "SPAS-12"}, {21, 279, 300, "Stubby shotgun"}};
inline const WeaponModel kSmgs[] = {
    {22, 281, 1000, "Tec-9"}, {23, 282, 1000, "Uzi"}, {24, 283, 1000, "Silenced Ingram"}, {25, 284, 1000, "MP5"},
};
inline const WeaponModel kRifles[] = {
    {26, 280, 1000, "M4"}, {27, 276, 1000, "Ruger"}, {28, 285, 200, "Sniper rifle"}, {29, 286, 200, "Laser-scope sniper"},
};
inline const WeaponModel kHeavy[] = {
    {30, 287, 50, "Rocket launcher"}, {31, 288, 1000, "Flamethrower"}, {32, 289, 1000, "M60"}, {33, 290, 2000, "Minigun"},
};
inline const WeaponModel kThrown[] = {
    {12, 270, 25, "Grenades"}, {14, 271, 25, "Tear gas"}, {15, 272, 25, "Molotov cocktails"}, {36, 292, 100, "Camera"},
};
inline const WeaponGroup kWeaponGroups[] = {
    CL_GROUP("Melee", kMelee),   CL_GROUP("Pistols", kPistols), CL_GROUP("Shotguns", kShotguns),
    CL_GROUP("SMGs", kSmgs),     CL_GROUP("Rifles", kRifles),   CL_GROUP("Heavy weapons", kHeavy),
    CL_GROUP("Thrown & other", kThrown),
};
constexpr int kMaxColour = 94;

inline const Place kPlaces[] = {
    {"Ocean View Hotel", 228.0f, -1277.0f, 12.0f},
    {"Vercetti Mansion", -378.0f, -537.0f, 17.3f},
    {"Malibu Club", 489.0f, -82.0f, 11.5f},
    {"Escobar International", -1455.0f, -810.0f, 14.9f},
};
#define GAME_TITLE "VICE CITY"
#define GAME_SHORT "VC"

#elif defined(GAME_III)
// ------------------------------------------------------------------ GTA III
inline const VehicleModel kCars[] = {
    {101, "Infernus"}, {105, "Cheetah"}, {119, "Banshee"}, {92, "Stinger"}, {111, "Kuruma"}, {102, "Blista"},
    {90, "Landstalker"}, {91, "Idaho"}, {94, "Perennial"}, {95, "Sentinel"}, {96, "Patriot"}, {99, "Stretch"},
    {100, "Manana"}, {108, "Moonbeam"}, {109, "Esperanto"}, {112, "Bobcat"}, {114, "BF Injection"},
    {115, "Hearse"}, {129, "Stallion"}, {134, "Mafia Sentinel"}, {135, "Yardie Lobo"}, {136, "Yakuza Stinger"},
    {137, "Diablo Stallion"}, {138, "Cartel Cruiser"}, {133, "Mr Wongs"}, {132, "Bellyup"},
};
inline const VehicleModel kService[] = {
    {116, "Police"}, {107, "FBI Car"}, {117, "Enforcer"}, {122, "Rhino"}, {123, "Barracks OL"},
    {97, "Firetruck"}, {106, "Ambulance"}, {110, "Taxi"}, {128, "Cabbie"}, {148, "Borgnine Taxi"},
    {121, "Bus"}, {127, "Coach"}, {118, "Securicar"},
};
inline const VehicleModel kWork[] = {
    {93, "Linerunner"}, {98, "Trashmaster"}, {103, "Pony"}, {104, "Mule"}, {113, "Mr Whoopee"}, {130, "Rumpo"},
    {139, "Hoods Rumpo XL"}, {145, "Flatbed"}, {146, "Yankee"}, {147, "Escape"}, {149, "Toyz Van"},
};
inline const VehicleModel kSpecial[] = {{126, "Dodo"}};
inline const VehicleModel kBoats[] = {{120, "Predator"}, {142, "Speeder"}, {143, "Reefer"}, {144, "Panlantic"}, {150, "Ghost"}};

inline const VehicleGroup kVehicleGroups[] = {
    CL_GROUP("Cars", kCars), CL_GROUP("Emergency & public", kService), CL_GROUP("Vans & trucks", kWork),
    CL_GROUP("Dodo", kSpecial), CL_GROUP("Boats", kBoats),
};

inline const char* const kWeather[] = {"Sunny", "Cloudy", "Rainy", "Foggy"};

inline const CheatInfo kPlayerCheats[] = {
    {0x1087610, "Full health (+ repair vehicle)"}, {0x1088050, "Full armor"}, {0x1087FC0, "+$250,000"},
    {0x10880F0, "Wanted level +2"}, {0x10881C0, "Clear wanted level"}, {0x1087C30, "Change outfit"},
};
inline const CheatInfo kWeaponCheats[] = {{0x1087490, "All weapons"}};
inline const CheatInfo kVehicleCheats[] = {
    {0x1088670, "Flying cars"}, {0x1088680, "Better handling"}, {0x1088660, "Invisible cars"},
    {0x1087BB0, "Blow up all cars"},
};
inline const CheatInfo kPedCheats[] = {
    {0x1087F40, "Peds have weapons"},
};
inline const CheatInfo kWorldCheats[] = {
    {0x1088650, "Fast-changing weather"}, {0x1087F50, "Faster gameplay"}, {0x1087F70, "Slower gameplay"},
};

inline const WeaponModel kAllWeapons[] = {
    {1, 0, 1, "Baseball bat"}, {2, 0, 500, "Pistol"}, {3, 0, 1000, "Uzi"}, {4, 0, 300, "Shotgun"},
    {5, 0, 1000, "AK-47"}, {6, 0, 1000, "M16"}, {7, 0, 200, "Sniper rifle"}, {8, 0, 50, "Rocket launcher"},
    {9, 0, 1000, "Flamethrower"}, {10, 0, 25, "Molotov cocktails"}, {11, 0, 25, "Grenades"},
};
inline const WeaponGroup kWeaponGroups[] = {CL_GROUP("All weapons", kAllWeapons)};
constexpr int kMaxColour = 94;

inline const Place kPlaces[] = {
    {"Portland safehouse", 885.0f, -308.0f, 8.7f},
};
#define GAME_TITLE "GTA III"
#define GAME_SHORT "III"
#else
#error Define GAME_VC or GAME_III
#endif
}
