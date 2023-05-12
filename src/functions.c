#include <stdlib.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include "helper_macro.h"
#include "functions.h"

static napi_value qjs_detect_module(napi_env env, napi_callback_info info) {
  NAPI_GET_CB_INFO_ARGS(env, info, 1, "Require source input")
  NAPI_CHECK_VALUE_TYPE(env, argv[0], napi_string, "Source is not string");

  size_t len = 0;
  NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &len));
  char* buf = (char*)malloc(len + 1);
  napi_status r = napi_get_value_string_utf8(env, argv[0], buf, len + 1, &len);
  if (r != napi_ok) {
    free(buf);
    NAPI_CALL(env, r);
  }
  *(buf + len) = '\0';

  bool b = JS_DetectModule(buf, len);
  free(buf);
  napi_value ret;
  NAPI_CALL(env, napi_get_boolean(env, b, &ret));
  return ret;
}

void register_qjs_functions(napi_env env, napi_value exports) {
  NAPI_SET_FUNCTION_VOID(env, exports, "detectModule", qjs_detect_module);
}
