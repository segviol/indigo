#pragma once;
#include "mir.hpp"

#include <memory>
#include <typeinfo>

void mir::types::PtrTy::reduce_array() {
  if (this->item->kind() == TyKind::Array) {
    auto arr = std::dynamic_pointer_cast<ArrayTy>(this->item);
    this->item = arr->item;
  }
  // else noop
}
