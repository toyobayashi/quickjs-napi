#include <unordered_map>
#include <string.h>
#include "qjspp.h"
#include "helper_macro.h"
#include "conversion.h"

typedef struct cfunc_promise_data {
  napi_env env;
  napi_deferred deferred;
} cfunc_promise_data;

static JSValue promise_then_cb_fulfill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  free(data);
  JS_FreeValue(ctx, *func_data);
  napi_value value = qjs_to_napi_value(env, ctx, *argv);
  if (value) napi_resolve_deferred(env, deferred, value);
  return JS_DupValue(ctx, *argv);
}

static JSValue promise_then_cb_reject(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
  cfunc_promise_data* data = (cfunc_promise_data*)JS_GetOpaque(*func_data, 1);
  napi_env env = data->env;
  napi_deferred deferred = data->deferred;
  free(data);
  JS_FreeValue(ctx, *func_data);
  napi_value reason = qjs_to_napi_value(env, ctx, *argv);
  if (reason) napi_reject_deferred(env, deferred, reason);
  JS_Throw(ctx, JS_DupValue(ctx, *argv));
  return JS_UNDEFINED;
}

static napi_value qjs_to_napi_value_internal(napi_env env, JSContext* ctx, JSValue val, std::unordered_map<JSObject*, napi_value>& seen) {
  napi_value ret = nullptr;
  if (JS_IsUndefined(val)) {
    NAPI_CALL(env, napi_get_undefined(env, &ret));
  } else if (JS_IsNull(val)) {
    NAPI_CALL(env, napi_get_null(env, &ret));
  } else if (JS_IsBool(val)) {
    NAPI_CALL(env, napi_get_boolean(env, (bool)JS_VALUE_GET_BOOL(val), &ret));
  } else if (JS_IsNumber(val)) {
    napi_value global;
    if (JS_VALUE_IS_NAN(val)) {
      NAPI_CALL(env, napi_get_global(env, &global));
      NAPI_CALL(env, napi_get_named_property(env, global, "NaN", &ret));
    } else if (JS_VALUE_GET_TAG(val) == JS_TAG_FLOAT64) {
      NAPI_CALL(env, napi_create_double(env, JS_VALUE_GET_FLOAT64(val), &ret));
    } else {
      int64_t tmp;
      JS_ToInt64(ctx, &tmp, val);
      NAPI_CALL(env, napi_create_int64(env, tmp, &ret));
    }
  } else if (JS_IsString(val)) {
    qjspp::CString str(ctx, val);
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &ret));
  } else if (JS_IsBigInt(ctx, val)) {
    int64_t tmp;
    JS_ToBigInt64(ctx, &tmp, val);
    NAPI_CALL(env, napi_create_bigint_int64(env, tmp, &ret));
  } else if (JS_IsSymbol(val)) {
    qjspp::Atom atom(ctx, val);
    qjspp::CString cstr = atom.ToCString();
    napi_value desc;
    NAPI_CALL(env, napi_create_string_utf8(env, cstr, NAPI_AUTO_LENGTH, &desc));
    NAPI_CALL(env, napi_create_symbol(env, desc, &ret));
  } else if (JS_IsObject(val)) {
    qjspp::Value global(qjspp::Global(ctx));
    qjspp::Value promise_ctor(ctx, JS_GetPropertyStr(ctx, global, "Promise"));
    if (!JS_IsUndefined(promise_ctor) && JS_IsInstanceOf(ctx, val, promise_ctor)) {
      qjspp::Value then(ctx, JS_GetPropertyStr(ctx, val, "then"));
      JSValue deferred_ref = JS_NewObject(ctx);
      napi_deferred deferred;
      cfunc_promise_data* data = (cfunc_promise_data*)malloc(sizeof(cfunc_promise_data));
      data->env = env;
      napi_status r = napi_create_promise(env, &deferred, &ret);
      if (r != napi_ok) {
        free(data);
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
      return ret;
    }

    qjspp::Value date_ctor(ctx, JS_GetPropertyStr(ctx, global, "Date"));
    if (!JS_IsUndefined(date_ctor) && JS_IsInstanceOf(ctx, val, date_ctor)) {
      qjspp::Value get_time(ctx, JS_GetPropertyStr(ctx, val, "getTime"));
      qjspp::Value return_val(ctx, JS_Call(ctx, get_time, val, 0, nullptr));
      double time = JS_VALUE_GET_FLOAT64(return_val.data());
      NAPI_CALL(env, napi_create_date(env, time, &ret));
      return ret;
    }

    qjspp::Value regexp_ctor(ctx, JS_GetPropertyStr(ctx, global, "RegExp"));
    if (!JS_IsUndefined(regexp_ctor) && JS_IsInstanceOf(ctx, val, regexp_ctor)) {
      qjspp::CString cstr(ctx, val);
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

    if (JS_IsArray(ctx, val)) {
      if (seen.find(JS_VALUE_GET_OBJ(val)) != seen.end()) {
        return seen[JS_VALUE_GET_OBJ(val)];
      }
      NAPI_CALL(env, napi_create_array(env, &ret));
      seen[JS_VALUE_GET_OBJ(val)] = ret;

      qjspp::Value keys_len(ctx, JS_GetPropertyStr(ctx, val, "length"));
      uint32_t len = (uint32_t)JS_VALUE_GET_INT(keys_len.data());

      for (uint32_t i = 0; i < len; ++i) {
        qjspp::Value v(ctx, JS_GetPropertyUint32(ctx, val, i));
        NAPI_CALL(env, napi_set_element(env, ret, i, qjs_to_napi_value_internal(env, ctx, v, seen)));
      }

      return ret;
    }

    if (seen.find(JS_VALUE_GET_OBJ(val)) != seen.end()) {
      return seen[JS_VALUE_GET_OBJ(val)];
    }
    NAPI_CALL(env, napi_create_object(env, &ret));
    seen[JS_VALUE_GET_OBJ(val)] = ret;

    qjspp::Value object_ctor(ctx, JS_GetPropertyStr(ctx, global, "Object"));
    qjspp::Value keys(ctx, JS_GetPropertyStr(ctx, object_ctor, "keys"));
    qjspp::Value keys_arr(ctx, JS_Call(ctx, keys, object_ctor, 1, &val));
    qjspp::Value keys_len(ctx, JS_GetPropertyStr(ctx, keys_arr, "length"));
    uint32_t len = (uint32_t)JS_VALUE_GET_INT(keys_len.data());
    for (uint32_t i = 0; i < len; ++i) {
      qjspp::Value k(ctx, JS_GetPropertyUint32(ctx, keys_arr, i));
      qjspp::CString kstr(ctx, k);
      qjspp::Atom atom(ctx, k);
      qjspp::Value v(ctx, JS_GetProperty(ctx, val, atom));
      NAPI_CALL(env, napi_set_named_property(env, ret, kstr, qjs_to_napi_value_internal(env, ctx, v, seen)));
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
  return qjs_to_napi_value_internal(env, ctx, val, seen);
}
