#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "../prelude/prelude.hpp"

namespace mir::types {

const int INT_SIZE = 4;
const int PTR_SIZE = 4;

class Ty;

typedef std::shared_ptr<Ty> SharedTyPtr;
typedef int LabelId;

enum class TyKind { Int, Void, Array, Ptr, Fn, RestParam, Vector };

/// Base class for types
class Ty : public prelude::Displayable {
 public:
  virtual TyKind kind() const = 0;
  virtual bool is_value_type() const = 0;
  virtual std::optional<int> size() const = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual ~Ty(){};
};

/// Int type. `i32` or `int32` in some languages.
class IntTy final : public Ty {
 public:
  IntTy() {}

  virtual TyKind kind() const { return TyKind::Int; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return INT_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~IntTy() {}
};

/// Create a new IntTy behind a shared pointer
std::shared_ptr<IntTy> new_int_ty();

/// Void or unit type.
class VoidTy final : public Ty {
 public:
  VoidTy() {}

  virtual TyKind kind() const { return TyKind::Void; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return 0; };
  virtual void display(std::ostream& o) const;
  virtual ~VoidTy() {}
};

/// Array type. `item[len]`
class ArrayTy final : public Ty {
 public:
  ArrayTy(SharedTyPtr item, int len) : item(item), len(len) {}

  SharedTyPtr item;
  int len;

  virtual TyKind kind() const { return TyKind::Array; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const {
    auto size = item->size();
    if (size.has_value())
      return (*size) * len;
    else
      return std::nullopt;
  };
  virtual void display(std::ostream& o) const;
  virtual ~ArrayTy() {}
};

/// SIMD Vector type. `<item x len>`
class VectorTy final : public Ty {
 public:
  VectorTy(SharedTyPtr item, int len) : item(item), len(len) {
    if (item->kind() != TyKind::Int) {
      // TODO: Throw exception
    }
  }

  SharedTyPtr item;
  int len;

  virtual TyKind kind() const { return TyKind::Vector; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const {
    auto size = item->size();
    if (size.has_value())
      return (*size) * len;
    else
      return std::nullopt;
  };
  virtual void display(std::ostream& o) const;
  virtual ~VectorTy() {}
  friend std::shared_ptr<VectorTy> default_vector() {
    return std::make_shared<VectorTy>(new_int_ty(), 4);
  }
};

/// Pointer type. `item*`
///
/// When using `Array<T, n>` to construct a Ptr, instead of `Ptr<Array<T, n>>`
/// it wil automagically reduce to `Ptr<T>`.
class PtrTy final : public Ty {
 public:
  PtrTy(SharedTyPtr item) : item(item) {
    // reduce "to" type to array item if it's an array
    reduce_array();
  }

  SharedTyPtr item;
  virtual TyKind kind() const { return TyKind::Ptr; }
  virtual bool is_value_type() const { return false; }
  virtual std::optional<int> size() const { return PTR_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~PtrTy() {}

 private:
  void reduce_array();
};

/// Represents a function.
class FunctionTy final : public Ty {
 public:
  FunctionTy(SharedTyPtr return_val, std::vector<SharedTyPtr> params,
             bool is_extern = false)
      : ret(return_val), params(params), is_extern(is_extern) {}

  SharedTyPtr ret;
  std::vector<SharedTyPtr> params;

  bool is_extern;

  virtual TyKind kind() const { return TyKind::Fn; }
  virtual bool is_value_type() const { return false; }
  virtual std::optional<int> size() const { return std::nullopt; };
  virtual void display(std::ostream& o) const;
  virtual ~FunctionTy() {}
};

/// Rest param for variadic function. This type only appears in standard library
/// `putf(char*, ...rest)` function.
class RestParamTy final : public Ty {
 public:
  RestParamTy() {}

  virtual TyKind kind() const { return TyKind::RestParam; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return std::nullopt; };
  virtual void display(std::ostream& o) const;
  virtual ~RestParamTy() {}
};

/// Create a new VoidTy behind shared pointer
std::shared_ptr<VoidTy> new_void_ty();

/// Create a new ArrayTy behind shared pointer
std::shared_ptr<ArrayTy> new_array_ty(SharedTyPtr item, int len);

/// Create a new PtrTy behind shared pointer
std::shared_ptr<PtrTy> new_ptr_ty(SharedTyPtr item);

}  // namespace mir::types
