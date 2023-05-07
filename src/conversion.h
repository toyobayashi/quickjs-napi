#ifndef SRC_CONVERSION_H_
#define SRC_CONVERSION_H_

#include <quickjs.h>
#include <quickjs-libc.h>
#include <js_native_api_types.h>

#ifdef __cplusplus
extern "C" {
#endif

napi_value qjs_to_napi_value(napi_env env, JSContext* ctx, JSValue val);
JSValue qjs_from_napi_value(napi_env env, JSContext* ctx, napi_value val);

#ifdef __cplusplus
}
#endif

#endif
