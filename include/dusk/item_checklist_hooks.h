#pragma once

#include <cstdint>

namespace dusk::item_checklist {

void on_item_slot_changed(uint8_t itemId);
void on_item_first_bit_changed(uint8_t itemId, bool collected);

}  // namespace dusk::item_checklist
