#include <utility>
#include <assert.h>
#include <quickjs.h>
#include <quickjs-libc.h>

namespace qjspp {

class Value;
class CString;
class Atom;

class CString final {
 private:
  JSContext* ctx_;
  const char* data_;
 public:
  CString(JSContext* ctx, const char* val) noexcept: ctx_(ctx), data_(val) { assert(ctx != nullptr); }
  ~CString() noexcept {
    if (data_) {
      JS_FreeCString(ctx_, data_);
      ctx_ = nullptr;
      data_ = nullptr;
    }
  }
  CString(CString&& other) noexcept: ctx_(std::exchange(other.ctx_, nullptr)), data_(std::exchange(other.data_, nullptr)) {}
  CString& operator=(CString&& other) noexcept {
    if (this == &other) return *this;
    if (data_) JS_FreeCString(ctx_, data_);
    ctx_ = std::exchange(other.ctx_, nullptr);
    data_ = std::exchange(other.data_, nullptr);
    return *this;
  }
  JSContext* ctx() const noexcept { return ctx_; }
  const char* data() const noexcept { return data_; }
  operator const char*() const noexcept { return data_; }
};

class Atom final {
 private:
  JSContext* ctx_;
  JSAtom data_;
 public:
  Atom(JSContext* ctx, JSAtom val) noexcept: ctx_(ctx), data_(val) { assert(ctx != nullptr); }
  ~Atom() noexcept { if (ctx_) { JS_FreeAtom(ctx_, data_); ctx_ = nullptr; } }
  Atom(Atom&& other) noexcept: ctx_(std::exchange(other.ctx_, nullptr)), data_(other.data_) {}
  JSContext* ctx() const noexcept { return ctx_; }
  JSAtom data() const noexcept { return data_; }
  operator JSAtom() const noexcept { return data_; }
  Value ToString() const noexcept;
  CString ToCString() const noexcept;
};

class Value {
 protected:
  JSContext* ctx_;
  JSValue data_;
 public:
  static Value New(JSContext* ctx, JSValue val) noexcept { return Value(ctx, JS_DupValue(ctx, val)); }
  Value(JSContext* ctx, JSValue val) noexcept: ctx_(ctx), data_(std::move(val)) { assert(ctx != nullptr); }
  virtual ~Value() noexcept { if (ctx_) { JS_FreeValue(ctx_, data_); ctx_ = nullptr; } }
  Value(const Value& other) noexcept: ctx_(other.ctx_), data_(JS_DupValue(other.ctx_, other.data_)) {};
  Value& operator=(const Value& other) noexcept {
    if (this == &other) return *this;
    if (ctx_) JS_FreeValue(ctx_, data_);
    ctx_ = other.ctx_;
    data_ = JS_DupValue(other.ctx_, other.data_);
    return *this;
  }
  Value(Value&& other) noexcept: ctx_(std::exchange(other.ctx_, nullptr)), data_(std::move(other.data_)) {}
  Value& operator=(Value&& other) noexcept {
    if (this == &other) return *this;
    if (ctx_) JS_FreeValue(ctx_, data_);
    ctx_ = std::exchange(other.ctx_, nullptr);
    data_ = std::move(other.data_);
    return *this;
  }
  JSContext* ctx() const noexcept { return ctx_; }
  JSValue data() const noexcept { return data_; }
  operator JSValue() const noexcept { return data_; }

  bool IsUndefined() const noexcept { return JS_IsUndefined(data_); }
  bool IsNull() const noexcept { return JS_IsNull(data_); }
  bool IsBoolean() const noexcept { return JS_IsBool(data_); }
  bool IsNumber() const noexcept { return JS_IsNumber(data_); }
  bool IsString() const noexcept { return JS_IsString(data_); }
  bool IsSymbol() const noexcept { return JS_IsSymbol(data_); }
  bool IsBigInt() const noexcept { return JS_IsBigInt(ctx_, data_); }
  bool IsArray() const noexcept { return JS_IsArray(ctx_, data_); }
  bool IsObject() const noexcept { return JS_IsObject(data_); }
  bool IsFunction() const noexcept { return JS_IsFunction(ctx_, data_); }
  bool IsError() const noexcept { return JS_IsError(ctx_, data_); }
  bool IsException() const noexcept { return JS_IsException(data_); }

  bool InstanceOf(JSValue val) const noexcept { return JS_IsInstanceOf(ctx_, data_, val); }
  bool InstanceOf(const Value& val) const noexcept { return JS_IsInstanceOf(ctx_, data_, val); }

  CString ToCString() const noexcept { return CString(ctx_, JS_ToCString(ctx_, data_)); }
  Atom ToAtom() const noexcept { return Atom(ctx_, JS_ValueToAtom(ctx_, data_)); }
};

inline Value Atom::ToString() const noexcept { return Value(ctx_, JS_AtomToString(ctx_, data_)); }
inline CString Atom::ToCString() const noexcept { return CString(ctx_, JS_AtomToCString(ctx_, data_)); }

inline Value Global(JSContext* ctx) {
  return Value(ctx, JS_GetGlobalObject(ctx));
}

} // namespace qjspp
