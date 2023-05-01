export declare class Runtime {
  constructor()
  dispose(): void
  data(): number | bigint
}

export declare class Context {
  constructor(rt: Runtime)
  dispose(): void
  data(): number | bigint
  eval(source: string): void
}

export declare namespace std {
  export function initHandlers(rt: Runtime): void
  export function loop(ctx: Context): void
}
