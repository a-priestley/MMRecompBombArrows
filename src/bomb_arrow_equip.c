#include "../include/modding.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

typedef struct {
  bool valid;
  PlayState *play;
  s32 equipSlot;
  s32 entryMainState;
  u8 currentItem;
  u8 currentSlot;
  u8 targetItem;
  u8 targetSlot;
} BombArrowEquipContext;

static BombArrowEquipContext sBombArrowEquipContext;

static s32 BombArrow_GetEquipSlotFromPauseTarget(s32 equipTargetCBtn) {
  switch (equipTargetCBtn) {
  case PAUSE_EQUIP_C_LEFT:
    return EQUIP_SLOT_C_LEFT;

  case PAUSE_EQUIP_C_DOWN:
    return EQUIP_SLOT_C_DOWN;

  case PAUSE_EQUIP_C_RIGHT:
    return EQUIP_SLOT_C_RIGHT;

  default:
    return -1;
  }
}
static bool BombArrow_IsEquip(u8 item, u8 slot) {
  return (item == ITEM_BOW) && (slot == SLOT_BOMB);
}

static bool BombArrow_IsNormalBowEquip(u8 item, u8 slot) {
  return (item == ITEM_BOW) && (slot == SLOT_BOW);
}

static bool BombArrow_IsBombEquip(u8 item, u8 slot) {
  return (item == ITEM_BOMB) && (slot == SLOT_BOMB);
}

static void BombArrow_ResolveEquip(u8 currentItem, u8 currentSlot,
                                   u8 targetItem, u8 targetSlot,
                                   u8 *resolvedItem, u8 *resolvedSlot) {
  *resolvedItem = targetItem;
  *resolvedSlot = targetSlot;

  /*
   * Bomb arrows already assigned: assigning bow or bombs replaces them with the
   * normal item.
   */
  if (BombArrow_IsEquip(currentItem, currentSlot)) {
    if (targetItem == ITEM_BOW) {
      *resolvedItem = ITEM_BOW;
      *resolvedSlot = SLOT_BOW;
      return;
    }

    if (targetItem == ITEM_BOMB) {
      *resolvedItem = ITEM_BOMB;
      *resolvedSlot = SLOT_BOMB;
      return;
    }
  }

  /*
   * Bow + bombs on the same C-button becomes bomb arrows.
   */
  if (BombArrow_IsNormalBowEquip(currentItem, currentSlot) &&
      BombArrow_IsBombEquip(targetItem, targetSlot)) {
    *resolvedItem = ITEM_BOW;
    *resolvedSlot = SLOT_BOMB;
    return;
  }

  /*
   * Bombs + bow on the same C-button becomes bomb arrows.
   */
  if (BombArrow_IsBombEquip(currentItem, currentSlot) &&
      BombArrow_IsNormalBowEquip(targetItem, targetSlot)) {
    *resolvedItem = ITEM_BOW;
    *resolvedSlot = SLOT_BOMB;
    return;
  }
}

static bool BombArrow_IsEquipSlotBombArrow(s32 equipSlot) {
  return ((BUTTON_ITEM_EQUIP(0, equipSlot) & 0xFF) == ITEM_BOW) &&
         ((C_SLOT_EQUIP(0, equipSlot) & 0xFF) == SLOT_BOMB);
}

bool BombArrow_IsActiveBombArrowEquip(PlayState *play) {
  if (play == NULL) {
    return false;
  }

  Player *player = GET_PLAYER(play);

  if (player == NULL) {
    return false;
  }

  switch (player->heldItemButton) {
  case EQUIP_SLOT_C_LEFT:
  case EQUIP_SLOT_C_DOWN:
  case EQUIP_SLOT_C_RIGHT:
    return BombArrow_IsEquipSlotBombArrow(player->heldItemButton);

  default:
    return false;
  }
}

RECOMP_HOOK("KaleidoScope_UpdateItemEquip")
void kaleido_scope_update_item_equip(PlayState *play) {
  sBombArrowEquipContext.valid = false;

  if (play == NULL) {
    return;
  }

  PauseContext *pauseCtx = &play->pauseCtx;
  s32 equipSlot =
      BombArrow_GetEquipSlotFromPauseTarget(pauseCtx->equipTargetCBtn);

  if (equipSlot < 0) {
    return;
  }

  sBombArrowEquipContext.valid = true;
  sBombArrowEquipContext.play = play;
  sBombArrowEquipContext.equipSlot = equipSlot;
  sBombArrowEquipContext.entryMainState = pauseCtx->mainState;

  sBombArrowEquipContext.currentItem = BUTTON_ITEM_EQUIP(0, equipSlot) & 0xFF;
  sBombArrowEquipContext.currentSlot = C_SLOT_EQUIP(0, equipSlot) & 0xFF;

  sBombArrowEquipContext.targetItem = pauseCtx->equipTargetItem & 0xFF;
  sBombArrowEquipContext.targetSlot = pauseCtx->equipTargetSlot & 0xFF;
}

RECOMP_HOOK_RETURN("KaleidoScope_UpdateItemEquip")
void kaleido_scope_update_item_equip_return() {
  BombArrowEquipContext ctx = sBombArrowEquipContext;
  sBombArrowEquipContext.valid = false;

  if (!ctx.valid || (ctx.play == NULL)) {
    return;
  }

  PauseContext *pauseCtx = &ctx.play->pauseCtx;

  /*
   * KaleidoScope_UpdateItemEquip is an animation update. Only override the
   * final result after the original function has actually equipped the item.
   */
  if (pauseCtx->mainState != PAUSE_MAIN_STATE_IDLE) {
    return;
  }

  if (ctx.entryMainState == PAUSE_MAIN_STATE_IDLE) {
    return;
  }

  u8 resolvedItem;
  u8 resolvedSlot;

  BombArrow_ResolveEquip(ctx.currentItem, ctx.currentSlot, ctx.targetItem,
                         ctx.targetSlot, &resolvedItem, &resolvedSlot);

  if (((BUTTON_ITEM_EQUIP(0, ctx.equipSlot) & 0xFF) == resolvedItem) &&
      ((C_SLOT_EQUIP(0, ctx.equipSlot) & 0xFF) == resolvedSlot)) {
    return;
  }

  BUTTON_ITEM_EQUIP(0, ctx.equipSlot) = resolvedItem;
  C_SLOT_EQUIP(0, ctx.equipSlot) = resolvedSlot;
  Interface_LoadItemIconImpl(ctx.play, ctx.equipSlot);
}
