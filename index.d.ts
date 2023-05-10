export declare interface MemoryUsage {
  mallocSize: number
  mallocLimit: number
  memoryUsedSize: number
  mallocCount: number
  memoryUsedCount: number
  atomCount: number
  atomSize: number
  strCount: number
  strSize: number
  objCount: number
  objSize: number
  propCount: number
  propSize: number
  shapeCount: number
  shapeSize: number
  jsFuncCount: number
  jsFuncSize: number
  jsFuncCodeSize: number
  jsFuncPc2lineCound: number
  jsFuncPc2lineSize: number
  cFuncCount: number
  arrayCount: number
  fastArrayCount: number
  fastArrayElements: number
  binaryObjectCount: number
  binaryObjectSize: number
}

export declare class Runtime {
  constructor()
  dispose(): void
  data(): number | bigint
  setMemoryLimit(value: number): void
  setGCThreshold(value: number): void
  setMaxStackSize(value: number): void
  runGC(): void
  computeMemoryUsage(): MemoryUsage
  setInterruptHandler(handler: (rt: Runtime) => number): void
  setCanBlock(value: boolean): void
  isJobPending(): boolean
  executePendingJob(): number
}

export declare class Context {
  constructor(rt: Runtime)
  dispose(): void
  data(): number | bigint
  addIntrinsicBaseObjects(): void
  addIntrinsicDate(): void
  addIntrinsicEval(): void
  addIntrinsicStringNormalize(): void
  addIntrinsicRegExp(): void
  addIntrinsicJSON(): void
  addIntrinsicProxy(): void
  addIntrinsicMapSet(): void
  addIntrinsicTypedArrays(): void
  addIntrinsicPromise(): void
  addIntrinsicBigInt(): void
  eval(source: string): void
  expose(name: string, value: any): void
}

export declare namespace std {
  export function initHandlers(rt: Runtime): void
  export function freeHandlers(rt: Runtime): void
  export function loop(ctx: Context): void
  export function dumpError(ctx: Context): void
  export function addHelpers(ctx: Context): void
  export function evalBinary(ctx: Context, buffer: ArrayBufferView, flags?: boolean | number): void
}
