#pragma once

#include "f_op/f_op_actor.h"
#include <cstdint>
#include <string>
#include <vector>

namespace dusk::save_state {

// Serialized state for a single actor
struct ActorSnapshot {
    char procName[8];
    u16 setID;
    u8 group;
    u8 cullType;
    u32 actor_status;
    u32 actor_condition;
    fpc_ProcID parentActorID;

    // Transform
    float posX, posY, posZ;
    float oldPosX, oldPosY, oldPosZ;
    s16 angleX, angleY, angleZ;
    s16 shapeAngleX, shapeAngleY, shapeAngleZ;
    s8 roomNo;
    float scaleX, scaleY, scaleZ;

    // Physics
    float speedX, speedY, speedZ;
    float speedF;
    float gravity;
    float maxFallSpeed;
    float homePosX, homePosY, homePosZ;
    s16 homeAngleX, homeAngleY, homeAngleZ;
    s8 homeRoomNo;

    // State
    s16 health;
    u32 attentionFlags;
    float attentionPosX, attentionPosY, attentionPosZ;

    // Enemy-specific (if applicable)
    bool isEnemy;
    u16 enemyFlags;
    u8 enemyThrowMode;
    float enemyAnmFrame;
    float enemyDownPosX, enemyDownPosY, enemyDownPosZ;
    float enemyHeadLockPosX, enemyHeadLockPosY, enemyHeadLockPosZ;

    // Link-specific
    bool isLink;
    u8 linkTransformState;
    u8 linkItemSlot[24];
    u16 linkRupees;
    s16 linkHearts;
    s16 linkMaxHearts;
    u8 linkMagic;
    u8 linkMaxMagic;
    u8 linkWalletLevel;
    u8 linkBombBag[3];
    u8 linkArrowCount;
    u8 linkSlingshotCount;
    u8 linkBottle[4];
    u8 linkBottleContent[4];
};

struct SaveStateData {
    char stageName[8];
    int8_t roomNo;
    int8_t layer;
    int16_t startPoint;
    uint32_t actorCount;
    // Followed by actorCount * ActorSnapshot
};

// Capture current game state into a snapshot
std::vector<uint8_t> captureState();

// Restore game state from a snapshot (returns true on success)
bool restoreState(const std::vector<uint8_t>& data);

// Get human-readable info about a saved state
std::string getStateInfo(const std::vector<uint8_t>& data);

// Validate state data
bool isValidState(const std::vector<uint8_t>& data);

} // namespace dusk::save_state
