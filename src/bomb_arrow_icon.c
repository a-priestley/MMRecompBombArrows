#include "interface_helper.h"

void CmpDma_LoadFile(uintptr_t segmentVrom, s32 id, void* dst, size_t size);
void Interface_DrawItemIconTexture(PlayState* play, TexturePtr texture,
                                   s16 button);
bool BombArrow_IsEquipSlotBombArrow(s32 equipSlot);

ITEM_ICON_TEXTURE_OVERRIDE_DECLARE
ITEM_ICON_TEXTURE_WIDTH_DECLARE
ITEM_ICON_TEXTURE_HEIGHT_DECLARE
ITEM_ICON_POSITIONS_X_DECLARE
ITEM_ICON_POSITIONS_Y_DECLARE
ITEM_ICON_RECT_SIZES_X_DECLARE
ITEM_ICON_RECT_SIZES_Y_DECLARE
ITEM_ICON_TEXTURE_SCALES_DECLARE

typedef struct {
  bool valid;
  PlayState* play;
  s16 button;
} BombArrowItemIconDrawContext;

static BombArrowItemIconDrawContext sBombArrowItemIconDrawContext;
static bool sBombArrowDrawingOverlay = false;
static bool sBombArrowBombIconLoaded = false;
static u32 sBombArrowBombIcon[ICON_ITEM_TEX_SIZE / sizeof(u32)];
static PlayState* sIconLoadPlayState = NULL;
static s32 sIconLoadEquipSlot = -1;

static bool BombArrow_InterfaceHelperReady(void) {
  return (pItemIconTextureOverride != NULL) &&
         (pItemIconTextureWidth != NULL) && (pItemIconTextureHeight != NULL) &&
         (pItemIconPositionsX != NULL) && (pItemIconPositionsY != NULL) &&
         (pItemIconRectSizesX != NULL) && (pItemIconRectSizesY != NULL) &&
         (pItemIconTextureScales != NULL);
}

static void BombArrow_EnsureBombOverlayIconLoaded(void) {
  if (sBombArrowBombIconLoaded) {
    return;
  }

  CmpDma_LoadFile(SEGMENT_ROM_START(icon_item_static_yar), ITEM_BOMB,
                  sBombArrowBombIcon, ICON_ITEM_TEX_SIZE);
  sBombArrowBombIconLoaded = true;
}

static bool BombArrow_IsCEquipSlot(s32 equipSlot) {
  return (equipSlot == EQUIP_SLOT_C_LEFT) || (equipSlot == EQUIP_SLOT_C_DOWN) ||
         (equipSlot == EQUIP_SLOT_C_RIGHT);
}

static void BombArrow_DrawBombIconOverlay(PlayState* play, s16 button) {
  if ((play == NULL) || sBombArrowDrawingOverlay ||
      !BombArrow_IsCEquipSlot(button) ||
      !BombArrow_IsEquipSlotBombArrow(button) ||
      !BombArrow_InterfaceHelperReady()) {
    return;
  }

  BombArrow_EnsureBombOverlayIconLoaded();

  const s16 overlayPixels = 11;
  const s16 overlayMargin = 1;
  const s16 overlayRectSize = overlayPixels << 2;
  const s16 overlayMarginSize = overlayMargin << 2;

  s16 oldX = (*pItemIconPositionsX)[button];
  s16 oldY = (*pItemIconPositionsY)[button];
  s16 oldRectX = (*pItemIconRectSizesX)[button];
  s16 oldRectY = (*pItemIconRectSizesY)[button];
  s16 oldScale = (*pItemIconTextureScales)[button];

  TexturePtr oldOverride = *pItemIconTextureOverride;
  int oldTextureWidth = *pItemIconTextureWidth;
  int oldTextureHeight = *pItemIconTextureHeight;

  (*pItemIconPositionsX)[button] =
      oldX + oldRectX - overlayRectSize - overlayMarginSize;
  (*pItemIconPositionsY)[button] =
      oldY + oldRectY - overlayRectSize - overlayMarginSize;
  (*pItemIconRectSizesX)[button] = overlayRectSize;
  (*pItemIconRectSizesY)[button] = overlayRectSize;
  (*pItemIconTextureScales)[button] = (32 << 10) / overlayPixels;

  *pItemIconTextureOverride = (TexturePtr)sBombArrowBombIcon;
  *pItemIconTextureWidth = 32;
  *pItemIconTextureHeight = 32;

  sBombArrowDrawingOverlay = true;
  Interface_DrawItemIconTexture(play, (TexturePtr)sBombArrowBombIcon, button);
  sBombArrowDrawingOverlay = false;

  *pItemIconTextureOverride = oldOverride;
  *pItemIconTextureWidth = oldTextureWidth;
  *pItemIconTextureHeight = oldTextureHeight;

  (*pItemIconPositionsX)[button] = oldX;
  (*pItemIconPositionsY)[button] = oldY;
  (*pItemIconRectSizesX)[button] = oldRectX;
  (*pItemIconRectSizesY)[button] = oldRectY;
  (*pItemIconTextureScales)[button] = oldScale;
}

RECOMP_HOOK("Interface_DrawItemIconTexture")
void bomb_arrow_item_icon_draw_entry(PlayState* play, TexturePtr texture,
                                     s16 button) {
  sBombArrowItemIconDrawContext.valid = false;

  if (sBombArrowDrawingOverlay) {
    return;
  }

  sBombArrowItemIconDrawContext.valid = true;
  sBombArrowItemIconDrawContext.play = play;
  sBombArrowItemIconDrawContext.button = button;
}

RECOMP_HOOK_RETURN("Interface_DrawItemIconTexture")
void bomb_arrow_item_icon_draw_return(void) {
  BombArrowItemIconDrawContext ctx = sBombArrowItemIconDrawContext;
  sBombArrowItemIconDrawContext.valid = false;

  if (!ctx.valid || sBombArrowDrawingOverlay) {
    return;
  }

  BombArrow_DrawBombIconOverlay(ctx.play, ctx.button);
}

RECOMP_CALLBACK("*", recomp_on_init)
void bomb_arrow_interface_helper_init(void) {
  ITEM_ICON_TEXTURE_OVERRIDE_REGISTER
  ITEM_ICON_TEXTURE_WIDTH_REGISTER
  ITEM_ICON_TEXTURE_HEIGHT_REGISTER
  ITEM_ICON_POSITIONS_X_REGISTER
  ITEM_ICON_POSITIONS_Y_REGISTER
  ITEM_ICON_RECT_SIZES_X_REGISTER
  ITEM_ICON_RECT_SIZES_Y_REGISTER
  ITEM_ICON_TEXTURE_SCALES_REGISTER
}
