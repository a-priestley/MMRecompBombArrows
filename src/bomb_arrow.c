#include "../include/modding.h"
#include "../include/z64actor.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"

static EnArrow *sInitializingArrow = NULL;
static PlayState *sPlayStateAtArrowInit = NULL;

typedef struct {
  EnArrow *arrow;
  EnBom *bomb;
} BombArrowLink;

#define MAX_BOMB_ARROWS 16

static BombArrowLink sBombArrowLinks[MAX_BOMB_ARROWS];

static void BombArrow_Link(EnArrow *arrow, EnBom *bomb) {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if (sBombArrowLinks[i].arrow == NULL) {
      sBombArrowLinks[i].arrow = arrow;
      sBombArrowLinks[i].bomb = bomb;
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
      return bomb;
    }
  }

  return NULL;
}

static EnBom *BombArrow_FindBomb(EnArrow *arrow) {
  for (s32 i = 0; i < MAX_BOMB_ARROWS; i++) {
    if (sBombArrowLinks[i].arrow == arrow) {
      return sBombArrowLinks[i].bomb;
    }
  }

  return NULL;
}

static void BombArrow_DisableAttachedBombCollision(EnBom *bomb) {
  if (bomb == NULL) {
    return;
  }

  /*
   * Prevent the arrow's collider from immediately hitting its own bomb.
   */
  bomb->collider1.base.acFlags = AC_NONE;
}
static void BombArrow_UpdateAttachedBomb(EnArrow *arrow) {
  EnBom *bomb = BombArrow_FindBomb(arrow);

  if (bomb == NULL) {
    return;
  }
  Vec3f arrow_head_position = arrow->unk_234;
  Actor_SetScale(&bomb->actor, 0.0025f);
  Math_Vec3f_Copy(&bomb->actor.prevPos, &bomb->actor.world.pos);
  Math_Vec3f_Copy(&bomb->actor.world.pos, &arrow->actor.world.pos);
  Math_Vec3s_Copy(&bomb->actor.world.rot, &arrow->actor.world.rot);
}

static bool IsBombArrow(EnArrow *arrow) {
  if (arrow == NULL) {
    return false;
  }
  return arrow->actor.params == ARROW_TYPE_NORMAL;
}

static void InitializeBombArrow(EnArrow *arrow, PlayState *play) {
  if ((play == NULL) || !IsBombArrow(arrow)) {
    return;
  }
  EnBom *bomb = (EnBom *)Actor_SpawnAsChild(
      &play->actorCtx, &arrow->actor, play, ACTOR_EN_BOM,
      arrow->actor.world.pos.x, arrow->actor.world.pos.y,
      arrow->actor.world.pos.z, arrow->actor.world.rot.x,
      arrow->actor.world.rot.y, arrow->actor.world.rot.z, BOMB_TYPE_BODY);

  if (bomb != NULL) {
    Actor_SetScale(&bomb->actor, 0.0025f);
    BombArrow_DisableAttachedBombCollision(bomb);
    BombArrow_Link(arrow, bomb);
  }

  /*
   * Drop arrow damage to zero so that struck enemies are instead
   * damaged by the bomb explosion.
   */
  arrow->collider.elem.atDmgInfo.damage = 0;
}

static void TryDetonateBombArrow(EnArrow *arrow) {
  if (arrow == NULL) {
    return;
  }
  EnBom *bomb = BombArrow_Unlink(arrow);
  if (bomb != NULL) {
    /*
     * Drop fuse timer to zero, resulting in immediate detonation.
     */
    bomb->timer = 0;
  }
  Actor_Kill(&arrow->actor);
}

RECOMP_HOOK("EnArrow_Init")
void bomb_arrow_init_entry(Actor *thisx, PlayState *play) {
  sInitializingArrow = (EnArrow *)thisx;
  sPlayStateAtArrowInit = play;
}

RECOMP_HOOK_RETURN("EnArrow_Init")
void bomb_arrow_init_return() {
  EnArrow *arrow = sInitializingArrow;
  sInitializingArrow = NULL;

  PlayState *play = sPlayStateAtArrowInit;
  sPlayStateAtArrowInit = NULL;

  InitializeBombArrow(arrow, play);
}

RECOMP_HOOK("EnArrow_Update")
void bomb_arrow_update(Actor *thisx, PlayState *play) {
  BombArrow_UpdateAttachedBomb((EnArrow *)thisx);
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
