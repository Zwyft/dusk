#include "dusk/save_state.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_save.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_actor_iter.h"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "m_Do/m_Do_controller_pad.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_horse.h"

#include <cstring>
#include <algorithm>

namespace dusk::save_state {
namespace {

// Helper to safely copy actor state
void captureActorBase(fopAc_ac_c* actor, ActorSnapshot& snap) {
    if (!actor) return;

    const char* procName = fopAcM_getProcNameString(actor);
    if (procName) {
        strncpy(snap.procName, procName, 7);
        snap.procName[7] = '\0';
    }
    snap.setID = actor->setID;
    snap.group = actor->group;
    snap.cullType = actor->cullType;
    snap.actor_status = actor->actor_status;
    snap.actor_condition = actor->actor_condition;
    snap.parentActorID = actor->parentActorID;

    snap.posX = actor->current.pos.x;
    snap.posY = actor->current.pos.y;
    snap.posZ = actor->current.pos.z;
    snap.oldPosX = actor->old.pos.x;
    snap.oldPosY = actor->old.pos.y;
    snap.oldPosZ = actor->old.pos.z;
    snap.angleX = actor->current.angle.x;
    snap.angleY = actor->current.angle.y;
    snap.angleZ = actor->current.angle.z;
    snap.shapeAngleX = actor->shape_angle.x;
    snap.shapeAngleY = actor->shape_angle.y;
    snap.shapeAngleZ = actor->shape_angle.z;
    snap.roomNo = actor->current.roomNo;
    snap.scaleX = actor->scale.x;
    snap.scaleY = actor->scale.y;
    snap.scaleZ = actor->scale.z;

    snap.speedX = actor->speed.x;
    snap.speedY = actor->speed.y;
    snap.speedZ = actor->speed.z;
    snap.speedF = actor->speedF;
    snap.gravity = actor->gravity;
    snap.maxFallSpeed = actor->maxFallSpeed;
    snap.homePosX = actor->home.pos.x;
    snap.homePosY = actor->home.pos.y;
    snap.homePosZ = actor->home.pos.z;
    snap.homeAngleX = actor->home.angle.x;
    snap.homeAngleY = actor->home.angle.y;
    snap.homeAngleZ = actor->home.angle.z;
    snap.homeRoomNo = actor->home.roomNo;

    snap.health = actor->health;
    snap.attentionFlags = actor->attention_info.flags;
    snap.attentionPosX = actor->attention_info.position.x;
    snap.attentionPosY = actor->attention_info.position.y;
    snap.attentionPosZ = actor->attention_info.position.z;

    snap.isEnemy = false;
    snap.isLink = false;
}

void captureEnemy(fopEn_enemy_c* enemy, ActorSnapshot& snap) {
    snap.isEnemy = true;
    snap.enemyFlags = enemy->mFlags;
    snap.enemyThrowMode = enemy->mThrowMode;
    snap.enemyAnmFrame = enemy->mAnmFrame;
    snap.enemyDownPosX = enemy->mDownPos.x;
    snap.enemyDownPosY = enemy->mDownPos.y;
    snap.enemyDownPosZ = enemy->mDownPos.z;
    snap.enemyHeadLockPosX = enemy->mHeadLockPos.x;
    snap.enemyHeadLockPosY = enemy->mHeadLockPos.y;
    snap.enemyHeadLockPosZ = enemy->mHeadLockPos.z;
}

void captureLink(daAlink_c* link, ActorSnapshot& snap) {
    snap.isLink = true;
    // Note: Most of Link's state is in dSv_info_c which is already saved
    // We just need runtime-only state here (currently minimal)
    (void)link;
}

// Iterate through all actors and capture their state
void* iterateActors(void* actor, void* userData) {
    auto* snapshots = static_cast<std::vector<ActorSnapshot>*>(userData);
    auto* ac = static_cast<fopAc_ac_c*>(actor);
    if (!ac) return nullptr;

    // Skip deleted or invalid actors
    if (ac->actor_condition & fopAcCnd_NOEXEC_e) return nullptr;

    ActorSnapshot snap = {};
    captureActorBase(ac, snap);

    // Check if it's an enemy
    if (ac->group == fopAc_ENEMY_e) {
        captureEnemy(static_cast<fopEn_enemy_c*>(ac), snap);
    }

    // Check if it's Link (player type)
    if (ac->actor_type == 0x10) {
        captureLink(static_cast<daAlink_c*>(ac), snap);
    }

    snapshots->push_back(snap);
    return nullptr; // Continue iteration
}

// Restore actor state from snapshot
void restoreActorBase(fopAc_ac_c* actor, const ActorSnapshot& snap) {
    if (!actor) return;

    actor->actor_status = snap.actor_status;
    actor->actor_condition = snap.actor_condition;
    actor->parentActorID = snap.parentActorID;

    actor->current.pos.x = snap.posX;
    actor->current.pos.y = snap.posY;
    actor->current.pos.z = snap.posZ;
    actor->old.pos.x = snap.oldPosX;
    actor->old.pos.y = snap.oldPosY;
    actor->old.pos.z = snap.oldPosZ;
    actor->current.angle.x = snap.angleX;
    actor->current.angle.y = snap.angleY;
    actor->current.angle.z = snap.angleZ;
    actor->shape_angle.x = snap.shapeAngleX;
    actor->shape_angle.y = snap.shapeAngleY;
    actor->shape_angle.z = snap.shapeAngleZ;
    actor->current.roomNo = snap.roomNo;
    actor->scale.x = snap.scaleX;
    actor->scale.y = snap.scaleY;
    actor->scale.z = snap.scaleZ;

    actor->speed.x = snap.speedX;
    actor->speed.y = snap.speedY;
    actor->speed.z = snap.speedZ;
    actor->speedF = snap.speedF;
    actor->gravity = snap.gravity;
    actor->maxFallSpeed = snap.maxFallSpeed;
    actor->home.pos.x = snap.homePosX;
    actor->home.pos.y = snap.homePosY;
    actor->home.pos.z = snap.homePosZ;
    actor->home.angle.x = snap.homeAngleX;
    actor->home.angle.y = snap.homeAngleY;
    actor->home.angle.z = snap.homeAngleZ;
    actor->home.roomNo = snap.homeRoomNo;

    actor->health = snap.health;
    actor->attention_info.flags = snap.attentionFlags;
    actor->attention_info.position.x = snap.attentionPosX;
    actor->attention_info.position.y = snap.attentionPosY;
    actor->attention_info.position.z = snap.attentionPosZ;
}

void restoreEnemy(fopEn_enemy_c* enemy, const ActorSnapshot& snap) {
    if (!snap.isEnemy) return;
    enemy->mFlags = snap.enemyFlags;
    enemy->mThrowMode = snap.enemyThrowMode;
    enemy->mAnmFrame = snap.enemyAnmFrame;
    enemy->mDownPos.x = snap.enemyDownPosX;
    enemy->mDownPos.y = snap.enemyDownPosY;
    enemy->mDownPos.z = snap.enemyDownPosZ;
    enemy->mHeadLockPos.x = snap.enemyHeadLockPosX;
    enemy->mHeadLockPos.y = snap.enemyHeadLockPosY;
    enemy->mHeadLockPos.z = snap.enemyHeadLockPosZ;
}

void restoreLink(daAlink_c* link, const ActorSnapshot& snap) {
    if (!snap.isLink) return;
    // Note: Most of Link's state is in dSv_info_c which is already restored
    (void)link;
    (void)snap;
}

// Find matching actor for restoration
struct RestoreContext {
    const ActorSnapshot* target;
    fopAc_ac_c* found;
    float bestDistSq;
};

void* findMatchingActor(void* actor, void* userData) {
    auto* ctx = static_cast<RestoreContext*>(userData);
    auto* ac = static_cast<fopAc_ac_c*>(actor);
    if (!ac || !ctx->target) return nullptr;

    // Match by process name and setID
    const char* procName = fopAcM_getProcNameString(ac);
    if (!procName || strncmp(procName, ctx->target->procName, 8) != 0) return nullptr;
    if (ac->setID != ctx->target->setID) return nullptr;

    // Calculate distance to target position
    float dx = ac->current.pos.x - ctx->target->posX;
    float dy = ac->current.pos.y - ctx->target->posY;
    float dz = ac->current.pos.z - ctx->target->posZ;
    float distSq = dx * dx + dy * dy + dz * dz;

    // Accept if within 100 units or this is the closest match
    if (distSq < 10000.0f && (!ctx->found || distSq < ctx->bestDistSq)) {
        ctx->found = ac;
        ctx->bestDistSq = distSq;
    }

    return nullptr; // Continue iteration
}

} // namespace

std::vector<uint8_t> captureState() {
    std::vector<ActorSnapshot> snapshots;
    snapshots.reserve(256);

    // Iterate through all actors
    fopAcIt_Judge(iterateActors, &snapshots);

    // Build the state data
    SaveStateData header = {};
    strncpy(header.stageName, dComIfGp_getStartStageName(), 7);
    header.roomNo = dComIfGp_getStartStageRoomNo();
    header.layer = dComIfGp_getStartStageLayer();
    header.startPoint = dComIfGp_getStartStagePoint();
    header.actorCount = static_cast<uint32_t>(snapshots.size());

    std::vector<uint8_t> data;
    data.reserve(sizeof(header) + snapshots.size() * sizeof(ActorSnapshot));

    // Copy header
    const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
    data.insert(data.end(), headerBytes, headerBytes + sizeof(header));

    // Copy snapshots
    for (const auto& snap : snapshots) {
        const uint8_t* snapBytes = reinterpret_cast<const uint8_t*>(&snap);
        data.insert(data.end(), snapBytes, snapBytes + sizeof(snap));
    }

    DuskLog.info("Save state captured: {} actors", snapshots.size());
    return data;
}

bool restoreState(const std::vector<uint8_t>& data) {
    if (!isValidState(data)) {
        DuskLog.warn("Invalid save state data");
        return false;
    }

    const SaveStateData* header = reinterpret_cast<const SaveStateData*>(data.data());
    const uint8_t* snapData = data.data() + sizeof(SaveStateData);

    DuskLog.info("Restoring save state: {} actors", header->actorCount);

    int restored = 0;
    int skipped = 0;

    for (uint32_t i = 0; i < header->actorCount; ++i) {
        const ActorSnapshot* snap = reinterpret_cast<const ActorSnapshot*>(snapData + i * sizeof(ActorSnapshot));

        RestoreContext ctx = {snap, nullptr, -1.0f};
        fopAcIt_Judge(findMatchingActor, &ctx);

        if (ctx.found) {
            restoreActorBase(ctx.found, *snap);

            if (ctx.found->group == fopAc_ENEMY_e) {
                restoreEnemy(static_cast<fopEn_enemy_c*>(ctx.found), *snap);
            }

            if (ctx.found->actor_type == 0x10) {
                restoreLink(static_cast<daAlink_c*>(ctx.found), *snap);
            }

            restored++;
        } else {
            skipped++;
            DuskLog.debug("Could not find matching actor for {} setID {}", snap->procName, snap->setID);
        }
    }

    DuskLog.info("Save state restored: {} actors restored, {} skipped", restored, skipped);
    return true;
}

std::string getStateInfo(const std::vector<uint8_t>& data) {
    if (!isValidState(data)) return "Invalid state";

    const SaveStateData* header = reinterpret_cast<const SaveStateData*>(data.data());
    return fmt::format("{} R{} - {} actors", header->stageName, (int)header->roomNo, header->actorCount);
}

bool isValidState(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(SaveStateData)) return false;

    const SaveStateData* header = reinterpret_cast<const SaveStateData*>(data.data());
    size_t expectedSize = sizeof(SaveStateData) + header->actorCount * sizeof(ActorSnapshot);

    return data.size() == expectedSize && header->actorCount < 10000;
}

} // namespace dusk::save_state
