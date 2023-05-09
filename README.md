# QuickJS Node-API binding

Browser support is powered by [emnapi](https://github.com/toyobayashi/emnapi).

**I created this repo for learning QuickJS and I want to try it in WeChat miniprogram. If you are looking for quickjs wasm, recommend you use [quickjs-emscripten](https://github.com/justjake/quickjs-emscripten).**

```bash
git clone --recurse-submodules https://github.com/toyobayashi/quickjs-napi.git
```

## WebAssembly

Install Emscripten, CMake and ninja.

```bash
npm install --ignore-scripts
npm run configure:emcc
npm run build:emcc
```

## Native

Windows requires Visual Studio 2022 17.5+

```bash
npm install
```
