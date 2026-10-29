#pragma once
#include <cstdint>

namespace Offsets {
    constexpr uintptr_t GameFacade               = 0xAC1E768;
    constexpr uintptr_t GameFacade_SF            = 0xB8;   // Il2CppClass → static_fields*
    constexpr uintptr_t GameFacade_MatchGame     = 0x8;    // static_fields + 0x8 = CurrentMatchGame
    constexpr uintptr_t GameFacade_P2            = 0xB8;   // kept for compat
    constexpr uintptr_t BaseGame_Match           = 0x90;   // MatchGame.m_Match
    constexpr uintptr_t BaseGame_Timer           = 0x20;
    constexpr uintptr_t Timer_Delta              = 0x2C;
    constexpr uintptr_t MatchIsRunning           = 0xCC;
    constexpr uintptr_t Match_PlayerDict         = 0x128;  // JMAGGLCNGIG.NOLEDCGOJKC — dump.cs confirmed
    constexpr uintptr_t Match_LocalPlayer        = 0xD8;   // JMAGGLCNGIG.MHJCLOPOBAA — dump.cs 0xD8
    constexpr uintptr_t Dict_Entries             = 0x18;
    constexpr uintptr_t Dict_Count               = 0x20;
    constexpr uintptr_t Dict_ArrayHeader         = 0x20;
    constexpr uintptr_t Dict_EntryStride         = 0x18;
    constexpr uintptr_t Dict_EntryValue          = 0x10;
    
    constexpr uintptr_t Player_Rotation          = 0x614; 
    constexpr uintptr_t Player_HeadTF            = 0x6A0;
    constexpr uintptr_t Player_FootTF            = 0x6E8;
    constexpr uintptr_t Player_Camera            = 0x690;
    constexpr uintptr_t Player_IsFiring          = 0x848;
    constexpr uintptr_t Player_HP                = 0x70;
    constexpr uintptr_t Player_IsDead            = 0x7C;
    constexpr uintptr_t Player_Avatar            = 0x770;
    constexpr uintptr_t Player_HedColider        = 0x738;
    constexpr uintptr_t Player_ColWrite          = 0x80;
    constexpr uintptr_t Player_DeathInfo         = 0x24A8;
    constexpr uintptr_t Player_IsBot             = 0x4A0;
    constexpr uintptr_t Player_Name              = 0x490;
    constexpr uintptr_t Player_AimInfo           = 0xE50;
    constexpr uintptr_t Player_Velocity          = 0xF0;
    constexpr uintptr_t Player_IsGrounded        = 0x11C;
    constexpr uintptr_t Player_FOVOffset         = 0x84;
    constexpr uintptr_t Avatar_Uma               = 0x138;
    constexpr uintptr_t Uma_Visible              = 0x101;
    constexpr uintptr_t Uma_Data                 = 0x28;
    constexpr uintptr_t UmaData_Skip             = 0x91;
    constexpr uintptr_t Camera_Follow            = 0x30;  // CameraControllerBase.AFMGCAELKBD (UnityEngine.Camera)
    constexpr uintptr_t Camera_IntPtr            = 0x10;
    constexpr uintptr_t Camera_Matrix            = 0x100;
    constexpr uintptr_t HP_List                  = 0x10;
    constexpr uintptr_t HP_Entry                 = 0x20;
    constexpr uintptr_t HP_Value                 = 0x18;
    constexpr uintptr_t Player_Weapon            = 0x600;
    constexpr uintptr_t Weapon_Data              = 0x80;
    constexpr uintptr_t Weapon_Recoil            = 0x18;
    constexpr uintptr_t AimInfo_Direction        = 0x40;
    constexpr uintptr_t AimInfo_StartPosition    = 0x4C;
    
    constexpr uintptr_t Bone_Head                = 0x6A0;
    constexpr uintptr_t Bone_Hip                 = 0x6A8;
    constexpr uintptr_t Bone_Breast              = 0x6B0;
    constexpr uintptr_t Bone_Root                = 0x6C8;
    constexpr uintptr_t Bone_LeftShoulder        = 0x708;
    constexpr uintptr_t Bone_RightShoulder       = 0x710;
    constexpr uintptr_t Bone_LeftAnkle           = 0x6D8;
    
    constexpr uintptr_t Bone_RightAnkle          = 0x6E0;
    constexpr uintptr_t Bone_LeftToe             = 0x6E8;
    constexpr uintptr_t Bone_RightToe            = 0x6F0;
    constexpr uintptr_t Bone_RightForeArm        = 0x728;
    constexpr uintptr_t Bone_LeftForeArm         = 0x730;
    constexpr uintptr_t Bone_RightHand           = 0x718;
    constexpr uintptr_t Bone_LeftHand            = 0x720;
}