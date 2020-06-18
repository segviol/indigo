#pragma once;
#include "mir.hpp"

#include <memory>
#include <typeinfo>

using namespace std;

void mir::types::PtrTy::reduce_array() {
  if (this->item->kind() == TyKind::Array) {
    auto arr = dynamic_pointer_cast<ArrayTy>(this->item);
    this->item = arr->item;
  }
}
