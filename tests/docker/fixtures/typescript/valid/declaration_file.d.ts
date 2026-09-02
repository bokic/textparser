// 10n: Declaration-file profile (.d.ts) - ambient contexts and legality.
export declare const version: string;
export declare function register(callback: () => void): void;
export declare class Handler {
    readonly options: Options;
    constructor(options: Options);
    on(event: "data", listener: (chunk: Buffer) => void): this;
    on(event: "error", listener: (err: Error) => void): this;
}
export declare interface Options {
    encoding: string;
    highWaterMark?: number;
}
export declare type Callback = () => void;
export declare namespace Config {
    const defaults: Options;
    function parse(text: string): Options;
}
export declare enum Mode {
    Read = 0,
    Write = 1,
}
export declare abstract class Stream {
    abstract read(): string;
    abstract write(chunk: string): void;
}
export declare const buffer: { alloc(size: number): Buffer };