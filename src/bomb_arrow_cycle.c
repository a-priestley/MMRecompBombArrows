#include <stdbool.h>

#include "../include/modding.h"
#include "../include/recomputils.h"
#include "../include/z64save.h"
#include "bomb_arrow_cycle.h"

static bool bomb_arrow_is_available() {
  return INV_CONTENT(ITEM_BOW) == ITEM_BOW &&
         INV_CONTENT(ITEM_BOMB) == ITEM_BOMB;
}

static CyclingArrowEntry sBombArrowCyclingEntry = {
    ITEM_BOW,
    SLOT_BOMB,
    bomb_arrow_is_available,
};

RECOMP_CALLBACK("*", recomp_on_init) void on_init() {
  if (recomp_is_dependency_met("mm_recomp_arrow_cycling") ==
      DEPENDENCY_STATUS_FOUND) {
    AddArrowEntry(sBombArrowCyclingEntry);
  }
}
