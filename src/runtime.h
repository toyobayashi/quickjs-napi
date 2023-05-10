#ifndef SRC_RUNTIME_H_
#define SRC_RUNTIME_H_

#include <quickjs.h>
#include <js_native_api_types.h>

typedef struct qjs_runtime_wrap_s {
  JSRuntime* value;
  napi_env env;
  napi_ref interrupt_handler;
  napi_ref this_arg;
} qjs_runtime_wrap_s;

void register_qjs_runtime_class(napi_env env, napi_value exports);

#endif
