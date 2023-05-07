export declare class Runtime {
  constructor()
  dispose(): void
  data(): number | bigint
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
