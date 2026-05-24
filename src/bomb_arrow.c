#include "../include/modding.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"

bool BombArrow_IsActiveBombArrowEquip(PlayState *play);

static EnArrow *sCurrentArrow = NULL;
static PlayState *sCurrentPlayState = NULL;

typedef struct {
  EnArrow *arrow;
  EnBom *bomb;
  bool loosed;
} BombArrowLink;

#define MAX_BOMB_ARROWS 16

static BombArrowLink sBombArrowLinks[MAX_BOMB_ARROWS];

static void BombArrow_Link(EnArrow *arrow, EnBom *bomb) {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if (sBombArrowLinks[i].arrow == NULL) {
      sBombArrowLinks[i].arrow = arrow;
      sBombArrowLinks[i].bomb = bomb;
      sBombArrowLinks[i].loosed = false;
      return;
    }
  }
}

static EnBom *BombArrow_Unlink(EnArrow *arrow) {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if (sBombArrowLinks[i].arrow == arrow) {
      EnBom *bomb = sBombArrowLinks[i].bomb;
      sBombArrowLinks[i].arrow = NULL;
      sBombArrowLinks[i].bomb = NULL;
      sBombArrowLinks[i].loosed = false;
      return bomb;
    }
  }

  return NULL;
}

static BombArrowLink *BombArrow_FindLinkByArrow(EnArrow *arrow) {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if (sBombArrowLinks[i].arrow == arrow) {
      return &sBombArrowLinks[i];
    }
  }

  return NULL;
}

static BombArrowLink *BombArrow_FindNockedLink() {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if ((sBombArrowLinks[i].arrow != NULL) &&
        (sBombArrowLinks[i].bomb != NULL) && !sBombArrowLinks[i].loosed) {
      return &sBombArrowLinks[i];
    }
  }

  return NULL;
}

static bool BombArrow_IsLoosed(EnArrow *arrow, PlayState *play) {
  if (arrow == NULL) {
    return false;
  }
  return Actor_HasNoParent(&arrow->actor, play);
}

static void BombArrow_UpdateAttachedBomb(EnArrow *arrow, PlayState *play) {
  BombArrowLink *link = BombArrow_FindLinkByArrow(arrow);

  if ((link == NULL) || (link->bomb == NULL)) {
    return;
  }

  EnBom *bomb = link->bomb;

  if (!link->loosed) {
    if (!BombArrow_IsLoosed(arrow, play)) {
      return;
    }

    Inventory_ChangeAmmo(ITEM_BOMB, -1);

    /*
     * A linked, unreleased bomb reaching timer 0 means it detonated while
     * nocked. Expend an arrow also. The explosion destroys it.
     * TODO: timer never seems to resolve to <= 0 here. 8 seems to be where it
     * hits once and only once. Will this be consistent?
     */
    if (bomb->timer <= 8) {
      Inventory_ChangeAmmo(ITEM_BOW, -1);
    }

    link->loosed = true;
  }

  Actor_SetScale(&bomb->actor, 0.0025f);

  /*
   * Local-space offset from the center of the arrow shaft.
   */
  static Vec3f loosedBombOffset = {0, 0, 4};
  Vec3f bombPos;
  MtxF bombMf;

  Matrix_Push();

  Matrix_SetTranslateRotateYXZ(
      arrow->actor.world.pos.x, arrow->actor.world.pos.y,
      arrow->actor.world.pos.z, &arrow->actor.shape.rot);

  Matrix_MultVec3f(&loosedBombOffset, &bombPos);
  Matrix_Get(&bombMf);

  Matrix_Pop();

  Math_Vec3f_Copy(&bomb->actor.world.pos, &bombPos);
  Math_Vec3f_Copy(&bomb->actor.prevPos, &bombPos);

  Matrix_MtxFToYXZRot(&bombMf, &bomb->actor.world.rot, false);
  Math_Vec3s_Copy(&bomb->actor.shape.rot, &bomb->actor.world.rot);
}

static void BombArrow_UpdateNockedBombFromPlayerLimb(Player *player,
                                                     s32 limbIndex) {
  if (player == NULL) {
    return;
  }

  if ((limbIndex != PLAYER_LIMB_RIGHT_HAND) ||
      (player->rightHandType != PLAYER_MODELTYPE_RH_BOW)) {
    return;
  }

  BombArrowLink *link = BombArrow_FindNockedLink();

  if ((link == NULL) || (link->bomb == NULL)) {
    return;
  }

  EnBom *bomb = link->bomb;

  /*
   * Local-space offset from the right hand limb.
   */
  static Vec3f sNockedBombOffset = {275, 1000, 275};

  Vec3f bombPos;
  MtxF bombMf;

  Matrix_MultVec3f(&sNockedBombOffset, &bombPos);
  Matrix_Get(&bombMf);

  Actor_SetScale(&bomb->actor, 0.0025f);

  Math_Vec3f_Copy(&bomb->actor.world.pos, &bombPos);
  Math_Vec3f_Copy(&bomb->actor.prevPos, &bombPos);

  Matrix_MtxFToYXZRot(&bombMf, &bomb->actor.world.rot, false);
  Math_Vec3s_Copy(&bomb->actor.shape.rot, &bomb->actor.world.rot);
}

static void InitializeBombArrow(EnArrow *arrow, PlayState *play) {
  if ((play == NULL) || (arrow == NULL) ||
      arrow->actor.params != ARROW_TYPE_NORMAL || AMMO(ITEM_BOMB) <= 0 ||
      !BombArrow_IsActiveBombArrowEquip(play)) {
    return;
  }

  EnBom *bomb = (EnBom *)Actor_SpawnAsChild(
      &play->actorCtx, &arrow->actor, play, ACTOR_EN_BOM,
      arrow->actor.world.pos.x, arrow->actor.world.pos.y,
      arrow->actor.world.pos.z, arrow->actor.world.rot.x,
      arrow->actor.world.rot.y, arrow->actor.world.rot.z, BOMB_TYPE_BODY);

  if (bomb != NULL) {
    Actor_SetScale(&bomb->actor, 0.0025f);
    /*
     * Prevent the arrow's collider from immediately hitting its own bomb.
     */
    bomb->collider1.base.acFlags = AC_NONE;
    BombArrow_Link(arrow, bomb);
  }
}

static void TryDetonateBombArrow(EnArrow *arrow) {
  if (arrow == NULL) {
    return;
  }
  BombArrowLink *link = BombArrow_FindLinkByArrow(arrow);
  if (link == NULL) {
    return;
  }
  bool loosed = link->loosed;
  EnBom *bomb = BombArrow_Unlink(arrow);
  if (bomb == NULL) {
    return;
  }

  if (loosed) {
    /*
     * Arrow either collided with something,
     * or despawned likely because it flew too far.
     * Drop fuse timer to zero, resulting in immediate detonation.
     */
    bomb->timer = 0;
    /*
     * Forcibly despawn arrow to prevent bounce off / geometry embed animation,
     * since the explosion would destroy it.
     */
    Actor_Kill(&arrow->actor);
    loosed = false;
  } else {
    /*
     * Arrow actor destroyed likely because equipment has been swapped.
     * Despawn bomb.
     */
    Actor_Kill(&bomb->actor);
  }
}

RECOMP_HOOK("EnArrow_Init")
void bomb_arrow_init_entry(Actor *thisx, PlayState *play) {
  sCurrentArrow = (EnArrow *)thisx;
  sCurrentPlayState = play;
}

RECOMP_HOOK_RETURN("EnArrow_Init")
void bomb_arrow_init_return() {
  EnArrow *arrow = sCurrentArrow;
  sCurrentArrow = NULL;

  PlayState *play = sCurrentPlayState;
  sCurrentPlayState = NULL;

  InitializeBombArrow(arrow, play);
}

RECOMP_HOOK("Player_PostLimbDrawGameplay")
void bomb_arrow_post_limb_draw(PlayState *play, s32 limbIndex, Gfx **dList1,
                               Gfx **dList2, Vec3s *rot, Actor *actor) {
  BombArrow_UpdateNockedBombFromPlayerLimb((Player *)actor, limbIndex);
}

RECOMP_HOOK("EnArrow_Update")
void bomb_arrow_update(Actor *thisx, PlayState *play) {
  sCurrentArrow = (EnArrow *)thisx;
  sCurrentPlayState = play;
}

RECOMP_HOOK_RETURN("EnArrow_Update")
void bomb_arrow_update_return() {
  EnArrow *arrow = sCurrentArrow;
  sCurrentArrow = NULL;
  PlayState *play = sCurrentPlayState;
  sCurrentPlayState = NULL;
  BombArrow_UpdateAttachedBomb(arrow, play);
}

RECOMP_HOOK("EnArrow_Destroy")
void bomb_arrow_destroy(Actor *thisx, PlayState *play) {
  TryDetonateBombArrow((EnArrow *)thisx);
}

RECOMP_HOOK("func_8088B630")
void bomb_arrow_world_impact(EnArrow *arrow, PlayState *play) {
  TryDetonateBombArrow(arrow);
}

RECOMP_HOOK("func_8088A7D8")
void bomb_arrow_actor_impact(PlayState *play, EnArrow *arrow) {
  TryDetonateBombArrow(arrow);
}
