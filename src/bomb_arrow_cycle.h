#ifndef BOMB_ARROW_CYCLE_H_
#define BOMB_ARROW_CYCLE_H_

#include <stdbool.h>

#include "../include/modding.h"
#include "../include/z64item.h"
#include "PR/ultratypes.h"

typedef struct {
  ItemId item;
  u8 slot;
  bool (*is_available)();
} CyclingArrowEntry;

RECOMP_IMPORT("mm_recomp_arrow_cycling",
              void AddArrowEntry(CyclingArrowEntry entry));

#endif  // BOMB_ARROW_CYCLE_H_
