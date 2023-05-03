#ifndef SRC_CONVERSION_H_
#define SRC_CONVERSION_H_

#include <quickjs.h>
#include <quickjs-libc.h>
#include "context.h"

napi_value qjs_to_napi_value(napi_env env, JSContext* ctx, JSValue val);

#endif
