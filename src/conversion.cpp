#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <string.h>
#include "qjspp.h"
#include "helper_macro.h"
#include "conversion.h"

#ifdef __wasm__
#include <emnapi.h>
#endif

inline std::string NapiStringToUtf8Value(napi_env env, napi_value str) {
  size_t length;
  NAPI_CALL(env, napi_get_value_string_utf8(env, str, nullptr, 0, &length));

  std::string value;
  value.reserve(length + 1);
  value.resize(length);
  NAPI_CALL(env, napi_get_value_string_utf8(
      env, str, &value[0], value.capacity(), nullptr));
  return value;
}

typedef struct cfunc_promise_data {
  napi_env env;
  napi_deferred deferred;
} cfunc_promise_data;

typedef struct qjs_fn_data {
  bool disposed;
  JSContext* ctx;
  JSValue js_value;
} qjs_fn_data;

static napi_value qjs_fn(napi_env env, napi_callback_info info) {
  size_t argc = 1024;
  napi_value argv[1024];
  qjs_fn_data* data;
  NAPI_CALL(env, napi_get_cb_info((env), (info), &argc, argv, nullptr, reinterpret_cast<void**>(&data)));
  if (data->disposed) {
    NAPI_CALL(env, napi_throw_error(env, nullptr, "Invalid QuickJS Function"));
    return nullptr;
  }
  JSValue js_value = data->js_value;
  JSContext* ctx = data->ctx;
  JSRuntime* rt = JS_GetRuntime(ctx);
  if (JS_IsLiveObject(rt, js_value)) {
    std::vector<JSValue> args;
    args.reserve(argc);
    bool is_pending_exception;
    for (size_t i = 0; i < argc; ++i) {
      JSValue v = qjs_from_napi_value(env, ctx, argv[i]);
      napi_is_exception_pending(env, &is_pending_exception);
      if (is_pending_exception) {
        JS_FreeValue(ctx, v);
        for (size_t x = 0; x < i; ++x) {
          JS_FreeValue(ctx, args[x]);
        }
        return nullptr;
      }
      args.emplace_back(std::move(v));
    }
    JSValue r = JS_Call(ctx, js_value, JS_UNDEFINED, argc, argc > 0 ? args.data() : nullptr);
    for (size_t i = 0; i < argc; ++i) {
      JS_FreeValue(ctx, args[i]);
    }
    if (JS_IsException(r)) {
      JSValue err = JS_GetException(ctx);
      napi_value ret = qjs_to_napi_value(env, ctx, err);
      JS_FreeValue(ctx, err);
      NAPI_CALL(env, napi_throw(env, ret));
      return ret;
    }
    napi_value ret = qjs_to_napi_value(env, ctx, r);
    JS_FreeValue(ctx, r);
    return ret;
  } else {
    NAPI_CALL(env, napi_throw_error(env, nullptr, "Invalid QuickJS Function"));
    return nullptr;
  }
}

static void qjs_fn_finalizer(napi_env env, void* data, void* hint) {
  qjs_fn_data* p = static_cast<qjs_fn_data*>(data);
  if (!p->disposed) {
    JS_FreeValue(p->ctx, p->js_value);
  }
  delete p;
}

static napi_value qjs_fn_dispose(napi_env env, napi_callback_info info) {
  qjs_fn_data* data;
  NAPI_CALL(env, napi_get_cb_info((env), (info), nullptr, nullptr, nullptr, reinterpret_cast<void**>(&data)));
  if (data->disposed) {
    NAPI_CALL(env, napi_throw_error(env, nullptr, "Invalid QuickJS Function"));
    return nullptr;
  }
  qjs_fn_data* p = static_cast<qjs_fn_data*>(data);
  JS_FreeValue(p->ctx, p->js_value);
  p->disposed = true;
  return nullptr;
}

static JSValue promise_then_cb_fulfill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  delete data;
  napi_value value = qjs_to_napi_value(env, ctx, *argv);
  if (value) napi_resolve_deferred(env, deferred, value);
  return JS_DupValue(ctx, *argv);
}

static JSValue promise_then_cb_reject(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  delete data;
  napi_value reason = qjs_to_napi_value(env, ctx, *argv);
  if (reason) napi_reject_deferred(env, deferred, reason);
  JS_Throw(ctx, JS_DupValue(ctx, *argv));
  return JS_UNDEFINED;
}

static napi_value qjs_to_napi_value_internal(napi_env env, const qjspp::Value& val, std::unordered_map<JSObject*, napi_value>& seen) {
  JSContext* ctx = val.ctx();
  napi_value ret = nullptr;
  if (val.IsUndefined()) {
    NAPI_CALL(env, napi_get_undefined(env, &ret));
  } else if (val.IsNull()) {
    NAPI_CALL(env, napi_get_null(env, &ret));
  } else if (val.IsBoolean()) {
    NAPI_CALL(env, napi_get_boolean(env, (bool)JS_VALUE_GET_BOOL(val.data()), &ret));
  } else if (val.IsNumber()) {
    napi_value global;
    if (JS_VALUE_IS_NAN(val)) {
      NAPI_CALL(env, napi_get_global(env, &global));
      NAPI_CALL(env, napi_get_named_property(env, global, "NaN", &ret));
    } else if (JS_VALUE_GET_TAG(val.data()) == JS_TAG_FLOAT64) {
      NAPI_CALL(env, napi_create_double(env, JS_VALUE_GET_FLOAT64(val.data()), &ret));
    } else {
      int64_t tmp;
      JS_ToInt64(ctx, &tmp, val);
      NAPI_CALL(env, napi_create_int64(env, tmp, &ret));
    }
  } else if (val.IsString()) {
    qjspp::CString str = val.ToCString();
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &ret));
  } else if (val.IsBigInt()) {
    int64_t tmp;
    JS_ToBigInt64(ctx, &tmp, val);
    NAPI_CALL(env, napi_create_bigint_int64(env, tmp, &ret));
  } else if (val.IsSymbol()) {
    qjspp::Atom atom = val.ToAtom();
    qjspp::CString cstr = atom.ToCString();
    napi_value desc;
    NAPI_CALL(env, napi_create_string_utf8(env, cstr, NAPI_AUTO_LENGTH, &desc));
    NAPI_CALL(env, napi_create_symbol(env, desc, &ret));
  } else if (val.IsObject()) {
    qjspp::Value global(qjspp::Global(ctx));
    if (val.IsFunction()) {
#ifdef __wasm__
        if (!emnapi_is_support_weakref()) {
          return nullptr;
        }
#endif
      qjs_fn_data* data = new qjs_fn_data;
      data->disposed = false;
      data->ctx = ctx;
      data->js_value = JS_DupValue(ctx, val.data());
      napi_create_function(env, nullptr, 0, qjs_fn, data, &ret);
      napi_value dispose_fn;
      napi_create_function(env, nullptr, 0, qjs_fn_dispose, data, &dispose_fn);
      napi_set_named_property(env, ret, "dispose", dispose_fn);
      napi_add_finalizer(env, ret, data, qjs_fn_finalizer, nullptr, nullptr);
      return ret;
    }

    qjspp::Value promise_ctor(ctx, JS_GetPropertyStr(ctx, global, "Promise"));
    if (!promise_ctor.IsUndefined() && val.InstanceOf(promise_ctor)) {
      qjspp::Value then(ctx, JS_GetPropertyStr(ctx, val, "then"));
      JSValue deferred_ref = JS_NewObject(ctx);
      napi_deferred deferred;
      cfunc_promise_data* data = new cfunc_promise_data;
      data->env = env;
      napi_status r = napi_create_promise(env, &deferred, &ret);
      if (r != napi_ok) {
        delete data;
        NAPI_CALL(env, r);
        return nullptr;
      }
      data->deferred = deferred;
      JS_SetOpaque(deferred_ref, (void*)data);
      JSValue cfunc_data[] = { deferred_ref };
      qjspp::Value onfulfill(ctx, JS_NewCFunctionData(ctx, promise_then_cb_fulfill, 1, 0, 1, cfunc_data));
      qjspp::Value onreject(ctx, JS_NewCFunctionData(ctx, promise_then_cb_reject, 1, 0, 1, cfunc_data));
      JSValue argv[2] = { onfulfill, onreject };
      qjspp::Value return_val(ctx, JS_Call(ctx, then, val, 2, argv));
      JS_FreeValue(ctx, deferred_ref);
      return ret;
    }

    qjspp::Value date_ctor(ctx, JS_GetPropertyStr(ctx, global, "Date"));
    if (!date_ctor.IsUndefined() && val.InstanceOf(date_ctor)) {
      qjspp::Value get_time(ctx, JS_GetPropertyStr(ctx, val, "getTime"));
      qjspp::Value return_val(ctx, JS_Call(ctx, get_time, val, 0, nullptr));
      double time = JS_VALUE_GET_FLOAT64(return_val.data());
      NAPI_CALL(env, napi_create_date(env, time, &ret));
      return ret;
    }

    qjspp::Value regexp_ctor(ctx, JS_GetPropertyStr(ctx, global, "RegExp"));
    if (!regexp_ctor.IsUndefined() && val.InstanceOf(regexp_ctor)) {
      qjspp::CString cstr = val.ToCString();
      const char* last_slash = strrchr(cstr, '/');
      const char* mode_str = last_slash + 1;
      napi_value args[2];
      NAPI_CALL(env, napi_create_string_utf8(env, cstr.data() + 1, (size_t)last_slash - (size_t)cstr.data() - 1, args));
      NAPI_CALL(env, napi_create_string_utf8(env, mode_str, NAPI_AUTO_LENGTH, args + 1));
      napi_value g, regexp;
      NAPI_CALL(env, napi_get_global(env, &g));
      NAPI_CALL(env, napi_get_named_property(env, g, "RegExp", &regexp));
      NAPI_CALL(env, napi_new_instance(env, regexp, 2, args, &ret));
      return ret;
    }

    if (val.IsError()) {
      napi_value msg = nullptr;
      napi_value stack = nullptr;
      qjspp::Value message_value(ctx, JS_GetPropertyStr(ctx, val, "message"));
      qjspp::CString message_value_str = message_value.ToCString();
      const char* msgraw = message_value_str.data();
      qjspp::Value stack_value(ctx, JS_GetPropertyStr(ctx, val, "stack"));
      qjspp::CString stack_value_str = stack_value.ToCString();

      qjspp::Value type_error_ctor(ctx, JS_GetPropertyStr(ctx, global, "TypeError"));
      if (!type_error_ctor.IsUndefined() && val.InstanceOf(type_error_ctor)) {
        NAPI_CALL(env, napi_create_string_utf8(env, msgraw, NAPI_AUTO_LENGTH, &msg));
        NAPI_CALL(env, napi_create_type_error(env, nullptr, msg, &ret));
        napi_value old_stack;
        napi_get_named_property(env, ret, "stack", &old_stack);
        std::string stack_str = NapiStringToUtf8Value(env, old_stack);
        size_t line = stack_str.find_first_of('\n');
        stack_str.insert(line + 1, stack_value_str.data());
        NAPI_CALL(env, napi_create_string_utf8(env, stack_str.c_str(), NAPI_AUTO_LENGTH, &stack));
        NAPI_CALL(env, napi_set_named_property(env, ret, "stack", stack));
        return ret;
      }

      qjspp::Value range_error_ctor(ctx, JS_GetPropertyStr(ctx, global, "RangeError"));
      if (!range_error_ctor.IsUndefined() && val.InstanceOf(range_error_ctor)) {
        NAPI_CALL(env, napi_create_string_utf8(env, msgraw, NAPI_AUTO_LENGTH, &msg));
        NAPI_CALL(env, napi_create_range_error(env, nullptr, msg, &ret));
        napi_value old_stack;
        napi_get_named_property(env, ret, "stack", &old_stack);
        std::string stack_str = NapiStringToUtf8Value(env, old_stack);
        size_t line = stack_str.find_first_of('\n');
        stack_str.insert(line + 1, stack_value_str.data());
        NAPI_CALL(env, napi_create_string_utf8(env, stack_str.c_str(), NAPI_AUTO_LENGTH, &stack));
        NAPI_CALL(env, napi_set_named_property(env, ret, "stack", stack));
        return ret;
      }

#ifdef NAPI_EXPERIMENTAL
      qjspp::Value syntax_error_ctor(ctx, JS_GetPropertyStr(ctx, global, "SyntaxError"));
      if (!syntax_error_ctor.IsUndefined() && val.InstanceOf(syntax_error_ctor)) {
        NAPI_CALL(env, napi_create_string_utf8(env, msgraw, NAPI_AUTO_LENGTH, &msg));
        NAPI_CALL(env, node_api_create_syntax_error(env, nullptr, msg, &ret));
        napi_value old_stack;
        napi_get_named_property(env, ret, "stack", &old_stack);
        std::string stack_str = NapiStringToUtf8Value(env, old_stack);
        size_t line = stack_str.find_first_of('\n');
        stack_str.insert(line + 1, stack_value_str.data());
        NAPI_CALL(env, napi_create_string_utf8(env, stack_str.c_str(), NAPI_AUTO_LENGTH, &stack));
        NAPI_CALL(env, napi_set_named_property(env, ret, "stack", stack));
        return ret;
      }
#endif

      NAPI_CALL(env, napi_create_string_utf8(env, msgraw, NAPI_AUTO_LENGTH, &msg));
      NAPI_CALL(env, napi_create_error(env, nullptr, msg, &ret));
      napi_value old_stack;
      napi_get_named_property(env, ret, "stack", &old_stack);
      std::string stack_str = NapiStringToUtf8Value(env, old_stack);
      size_t line = stack_str.find_first_of('\n');
      stack_str.insert(line + 1, stack_value_str.data());
      NAPI_CALL(env, napi_create_string_utf8(env, stack_str.c_str(), NAPI_AUTO_LENGTH, &stack));
      NAPI_CALL(env, napi_set_named_property(env, ret, "stack", stack));
      return ret;
    }

    JSObject* val_obj = JS_VALUE_GET_OBJ(val.data());
    std::unordered_map<JSObject*, napi_value>::iterator it = seen.find(val_obj);
    if (it != seen.end()) {
      return it->second;
    }
    if (val.IsArray()) {
      NAPI_CALL(env, napi_create_array(env, &ret));
      seen[val_obj] = ret;

      qjspp::Value keys_len(ctx, JS_GetPropertyStr(ctx, val, "length"));
      uint32_t len = (uint32_t)JS_VALUE_GET_INT(keys_len.data());

      for (uint32_t i = 0; i < len; ++i) {
        qjspp::Value v(ctx, JS_GetPropertyUint32(ctx, val, i));
        NAPI_CALL(env, napi_set_element(env, ret, i, qjs_to_napi_value_internal(env, v, seen)));
      }

      return ret;
    }

    NAPI_CALL(env, napi_create_object(env, &ret));
    seen[val_obj] = ret;

    qjspp::Value object_ctor(ctx, JS_GetPropertyStr(ctx, global, "Object"));
    if (object_ctor.IsUndefined()) {
      NAPI_CALL(env, napi_throw_type_error(env, nullptr, "Object is not defined, try to use JS_AddIntrinsicBaseObjects"));
      return nullptr;
    }
    qjspp::Value keys(ctx, JS_GetPropertyStr(ctx, object_ctor, "keys"));
    JSValue value = val.data();
    qjspp::Value keys_arr(ctx, JS_Call(ctx, keys, object_ctor, 1, &value));
    qjspp::Value keys_len(ctx, JS_GetPropertyStr(ctx, keys_arr, "length"));
    uint32_t len = (uint32_t)JS_VALUE_GET_INT(keys_len.data());
    for (uint32_t i = 0; i < len; ++i) {
      qjspp::Value k(ctx, JS_GetPropertyUint32(ctx, keys_arr, i));
      qjspp::CString kstr = k.ToCString();
      qjspp::Atom atom = k.ToAtom();
      qjspp::Value v(ctx, JS_GetProperty(ctx, val, atom));
      NAPI_CALL(env, napi_set_named_property(env, ret, kstr, qjs_to_napi_value_internal(env, v, seen)));
    }

    return ret;
  } else {
    NAPI_CALL(env, napi_throw_type_error(env, nullptr, "unsupported type"));
    return nullptr;
  }
  return ret;
}

napi_value qjs_to_napi_value(napi_env env, JSContext* ctx, JSValue val) {
  std::unordered_map<JSObject*, napi_value> seen;
  qjspp::Value value = qjspp::Value::New(ctx, val);
  return qjs_to_napi_value_internal(env, value, seen);
}

#define NAPI_CALL_JSVALUE(env, the_call)                                \
  NAPI_CALL_BASE(env, the_call, JS_UNDEFINED)

static JSValue qjs_host_fn(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  qjspp::Value env_value(ctx, JS_GetPropertyStr(ctx, *func_data, "env"));
  qjspp::Value ref_value(ctx, JS_GetPropertyStr(ctx, *func_data, "ref"));
  int64_t e, r;
  JS_ToInt64(ctx, &e, env_value);
  JS_ToInt64(ctx, &r, ref_value);
  napi_env env = (napi_env)e;
  napi_ref ref = (napi_ref)r;

  std::vector<napi_value> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    qjspp::Value v = qjspp::Value::New(ctx, *(argv + i));
    args.emplace_back(qjs_to_napi_value(env, ctx, v));
  }
  napi_value recv, f, ret;
  NAPI_CALL_JSVALUE(env, napi_get_undefined(env, &recv));
  NAPI_CALL_JSVALUE(env, napi_get_reference_value(env, ref, &f));
  NAPI_CALL_JSVALUE(env, napi_call_function(env, recv, f, argc, argc > 0 ? args.data() : nullptr, &ret));

  JSValue v = qjs_from_napi_value(env, ctx, ret);
  bool is_pending_exception;
  napi_is_exception_pending(env, &is_pending_exception);
  if (is_pending_exception) {
    JS_FreeValue(ctx, v);
    napi_value err, msg;
    napi_get_and_clear_last_exception(env, &err);
    napi_get_named_property(env, err, "message", &msg);
    std::string msg_str = NapiStringToUtf8Value(env, msg);
    return JS_ThrowInternalError(ctx, "%s", msg_str.c_str());
  }

  return v;
}

JSValue qjs_from_napi_value_internal(napi_env env, JSContext* ctx, napi_value val, std::unordered_map<napi_value, JSValue>& seen) {
  napi_valuetype type;
  NAPI_CALL_JSVALUE(env, napi_typeof(env, val, &type));
  napi_value global;
  NAPI_CALL_JSVALUE(env, napi_get_global(env, &global));
  if (type == napi_undefined) {
    return JS_UNDEFINED;
  }
  if (type == napi_null) {
    return JS_NULL;
  }
  if (type == napi_boolean) {
    bool ret;
    NAPI_CALL_JSVALUE(env, napi_get_value_bool(env, val, &ret));
    return JS_NewBool(ctx, ret ? 1 : 0);
  }
  if (type == napi_number) {
    napi_value nan;
    NAPI_CALL_JSVALUE(env, napi_get_named_property(env, global, "NaN", &nan));
    bool isnan;
    NAPI_CALL_JSVALUE(env, napi_strict_equals(env, val, nan, &isnan));
    if (isnan) return JS_NAN;
    double ret;
    NAPI_CALL_JSVALUE(env, napi_get_value_double(env, val, &ret));
    return JS_NewFloat64(ctx, ret);
  }
  if (type == napi_string) {
    std::string str = NapiStringToUtf8Value(env, val);
    return JS_NewString(ctx, str.c_str());
  }
  if (type == napi_bigint) {
    int64_t ret;
    bool lossless;
    NAPI_CALL_JSVALUE(env, napi_get_value_bigint_int64(env, val, &ret, &lossless));
    return JS_NewBigInt64(ctx, ret);
  }
  if (type == napi_symbol) {
    NAPI_CALL_JSVALUE(env, napi_throw_error(env, nullptr, "Can not convert Symbol"));
    return JS_UNDEFINED;
  }
  if (type == napi_function) {
    napi_value func_len;
    NAPI_CALL_JSVALUE(env, napi_get_named_property(env, val, "length", &func_len));
    int len;
    NAPI_CALL_JSVALUE(env, napi_get_value_int32(env, func_len, &len));

    napi_ref func_ref;
    NAPI_CALL_JSVALUE(env, napi_create_reference(env, val, 1, &func_ref));
    JSValue fn_data = JS_NewObject(ctx);
    qjspp::Value env_value(ctx, JS_NewInt64(ctx, (int64_t)env));
    qjspp::Value ref_value(ctx, JS_NewInt64(ctx, (int64_t)func_ref));
    JS_SetPropertyStr(ctx, fn_data, "env", env_value);
    JS_SetPropertyStr(ctx, fn_data, "ref", ref_value);
    JSValue fn = JS_NewCFunctionData(ctx, qjs_host_fn, 0, 0, 1, &fn_data);
    JS_FreeValue(ctx, fn_data);
    return fn;
  }

  bool eq;
  for (const auto& pair : seen) {
    napi_strict_equals(env, pair.first, val, &eq);
    if (eq) {
      return pair.second;
    }
  }

  bool is_array;
  NAPI_CALL_JSVALUE(env, napi_is_array(env, val, &is_array));
  if (is_array) {
    JSValue ret = JS_NewArray(ctx);
    seen[val] = ret;

    uint32_t len;
    napi_get_array_length(env, val, &len);

    std::vector<qjspp::Value> elements;
    elements.reserve(len);
    bool is_pending_exception;

    for (uint32_t i = 0; i < len; ++i) {
      napi_value v;
      napi_get_element(env, val, i, &v);
      qjspp::Value value(ctx, qjs_from_napi_value_internal(env, ctx, v, seen));
      napi_is_exception_pending(env, &is_pending_exception);
      if (is_pending_exception) {
        return JS_UNDEFINED;
      }
      elements.emplace_back(std::move(value));
    }
    for (uint32_t i = 0; i < len; ++i) {
      JS_SetPropertyUint32(ctx, ret, i, elements[i]);
    }

    return ret;
  }

  JSValue ret = JS_NewObject(ctx);
  seen[val] = ret;
  napi_value obj_ctor, keys_fn, key_arr;
  napi_get_named_property(env, global, "Object", &obj_ctor);
  napi_get_named_property(env, obj_ctor, "keys", &keys_fn);
  napi_call_function(env, obj_ctor, keys_fn, 1, &val, &key_arr);
  uint32_t len;
  napi_get_array_length(env, key_arr, &len);
  std::vector<std::pair<qjspp::Value, qjspp::Value>> kvs;
  kvs.reserve(len);

  bool is_pending_exception;

  for (uint32_t i = 0; i < len; ++i) {
    napi_value k, v;
    napi_get_element(env, key_arr, i, &k);
    napi_get_property(env, val, k, &v);
    std::string str = NapiStringToUtf8Value(env, k);
    qjspp::Value key(ctx, JS_NewString(ctx, str.c_str()));
    qjspp::Value value = qjspp::Value::New(ctx, qjs_from_napi_value_internal(env, ctx, v, seen));
    napi_is_exception_pending(env, &is_pending_exception);
    if (is_pending_exception) {
      return JS_UNDEFINED;
    }
    kvs.emplace_back(std::make_pair(std::move(key), std::move(value)));
  }

  for (uint32_t i = 0; i < len; ++i) {
    const auto& pair = kvs[i];
    JS_SetProperty(ctx, ret, pair.first.ToAtom(), pair.second);
  }

  return ret;
}

JSValue qjs_from_napi_value(napi_env env, JSContext* ctx, napi_value val) {
  std::unordered_map<napi_value, JSValue> seen;
  return qjs_from_napi_value_internal(env, ctx, val, seen);
}
