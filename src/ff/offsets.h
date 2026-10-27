#pragma once
#include <cstdint>

// Free Fire global — libil2cpp.so relative offsets
// These shift with each game update — re-derive with Il2CppDumper after update
namespace FF {

// il2cpp world pointers (base = libil2cpp.so load address from /proc/pid/maps)
constexpr uint64_t OBJ_MANAGER      = 0x0550A1C8; // GameObjectManagerGlobal
constexpr uint64_t SCENE_OBJ_LIST   = 0x18;        // ->sceneObjects
constexpr uint64_t TAGGED_OBJ_LIST  = 0x28;        // ->taggedObjects

// Character / entity offsets from Transform
constexpr uint64_t COMP_LIST        = 0x30;         // transform.componentArray
constexpr uint64_t CHAR_CONTROLLER  = 0x08;         // component[1] = CharacterController

// CharacterController fields
constexpr uint64_t CC_HEALTH        = 0xE0;
constexpr uint64_t CC_MAX_HEALTH    = 0xE4;
constexpr uint64_t CC_IS_DEAD       = 0xEC;
constexpr uint64_t CC_TEAM_ID       = 0x148;
constexpr uint64_t CC_PLAYER_NAME   = 0x1A0;       // MonoString ptr

// Transform bone positions
constexpr uint64_t TRANSFORM_POS    = 0x90;        // Vector3 world position (3 floats)
constexpr uint64_t TRANSFORM_HEAD   = 0xB0;        // head bone position

// il2cpp TypeInfo needed to filter character objects
constexpr uint64_t CHAR_TYPEINFO    = 0x0544B7D0;

// Screen dimensions — set at runtime
extern int SCREEN_W;
extern int SCREEN_H;

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };

} // namespace FF
