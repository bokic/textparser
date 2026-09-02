// 10i: Functions and arrows, including async, generator, and ambiguity forms.
function declared(a: number, b?: string, ...rest: boolean[]): void {}
function typed(): number {
    return 0;
}
function withDefault(value: number = 10): number {
    return value;
}

const arrow1 = (x: number): number => x * 2;
const arrow2 = (x) => x + 1;
const arrow3 = (x: number, y: number) => x + y;
const arrow4 = () => ({ value: 1 });
const arrow5 = async (x: number) => await fetch(`/api/${x}`);
const arrow6 = async () => await fetch("/");
const arrow7 = (async) => async;
const arrow8 = async => async + 1;
const arrow9 = <T,>(value: T): T[] => [value];
const arrow10 = <T extends object>(value: T) => value;

function* generator(): Generator<number> {
    yield 1;
    yield* inner();
    return 0;
}

async function asyncFunction(): Promise<void> {
    await task();
}

async function* asyncGenerator(): AsyncGenerator<number> {
    yield await fetchOne();
    yield* nestedGenerator();
}

const functionExpression = function () {};
const namedFunctionExpression = function named() {};
const immediatelyInvoked = (function () {})();
const classExpression = class Named {};

function restOverload(a: number): string;
function restOverload(...args: number[]): string {
    return "";
}

function defaultUndefined(a: number = void 0) {}
function optionalParams(a?: number, b?: string) {}

declare function ambientWithThis(this: Window, x: number): void;

const objectMethod = {
    plain() {},
    async asyncMethod() {},
    *generatorMethod() {
        yield 1;
    },
    async *asyncGeneratorMethod() {
        yield 1;
    },
    arrowProperty: () => {},
};

function outer() {
    function inner(): number {
        return 0;
    }
    return inner;
}

const callback = (err: Error | null) => {
    if (err) throw err;
};

function parameterDestructuring({ a, b }: { a: number; b: number }) {}
function arrayParam([first, second]: [number, number]) {}
function withRestDestructuring({ head, ...tail }) {}