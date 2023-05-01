cmake_minimum_required(VERSION 3.13)

project(quickjs_napi)

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

add_compile_definitions("NAPI_VERSION=8")

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(IS_EMSCRIPTEN ON)
else()
  set(IS_EMSCRIPTEN OFF)
endif()

set(QJS_TARGET_NAME qjs_binding)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs")

set(QJS_BINDING_SRC
  "src/binding.c"
  "src/context.c"
  "src/runtime.c"
  "src/libc.c"
)

if(IS_EMSCRIPTEN)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/node_modules/emnapi")
  add_executable(${QJS_TARGET_NAME} ${QJS_BINDING_SRC})
  target_link_libraries(${QJS_TARGET_NAME} emnapi quickjs)
  target_link_options(${QJS_TARGET_NAME} PRIVATE
    "-sEXPORTED_FUNCTIONS=['_napi_register_wasm_v1','_malloc','_free']"
    "-sALLOW_MEMORY_GROWTH"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=quickjs"
    "-sSAFE_HEAP=1"
    "-sSTACK_SIZE=5MB"
    "--minify=0"
    "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/src/pre.js"
  )
else()
  include_directories(${CMAKE_JS_INC})
  add_library(${QJS_TARGET_NAME} SHARED ${QJS_BINDING_SRC} ${CMAKE_JS_SRC})
  set_target_properties(${QJS_TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
  target_link_libraries(${QJS_TARGET_NAME} PRIVATE quickjs ${CMAKE_JS_LIB})
endif()

target_include_directories(${QJS_TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs")
