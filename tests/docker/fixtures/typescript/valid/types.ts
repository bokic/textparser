// 10b: TypeScript type grammar.
let a: number;
let b: string;
let c: boolean;
let d: any;
let e: unknown;
let f: never;
let g: void;
let h: object;
let i: symbol;
let j: bigint;
let k: null;
let l: undefined;
let m: unique symbol;

type Point2D = { x: number; y: number };
type Size = { width: number; height: number };
type Shape = Point2D & Size;
type Name = string | number;
type Maybe<T> = T | null;
type Callback = (value: number) => string;
type Registry = { [key: string]: number };
type Tuple = [number, string, boolean?];
type Fixed = [x: number, y: string];
type OptionalTuple = [number, ...string[]];
type EmptyArray = [];
type UnionArray = string[] | number[][];
type ReadonlyArray2 = ReadonlyArray<number>;
type FunctionType = () => void;
type RestParameters = (...args: string[]) => void;
type AssertFunction = (value: unknown) => asserts value;
type Guard = (value: unknown) => value is string;

type Indexed = { readonly [index: number]: string };
type Method = { get size(): number; set size(value: number); };
type Callable = { (x: number): number; label: string };
type GenericMethod = { map<U>(fn: (item: T) => U): U[] };
interface Named {
    readonly id: number;
    name?: string;
    [key: string]: unknown;
}
type Conditional<T> = T extends string ? "text" : "other";
type InferIf<T> = T extends Array<infer U> ? U : never;
type Keyof = keyof Point2D;
type Typeof = typeof a;
type Imported = import("./module").Thing;
type Mapped<T> = { [P in keyof T]?: T[P] };

type LiteralNumber = 42 | -1 | 0.5 | 1e3;
type LiteralString = "up" | "down";
type LiteralBoolean = true | false;
type LiteralTemplate = `plain-no-substitution`;
type TemplateType = `user:${string}`;
type TemplateMulti = `a${number}b${string}c`;
type TemplateUnion = `k-${"a" | "b" | "c"}`;
type TemplateNested = `${`inner-${string}`}`;

type Recursive = { value: number; next: Recursive | null };
type IntersectionParenthesis = (A & B) | C;
type UnionIntersection = A | (B & C);
type ArrayOfFunctions = ((x: number) => void)[];
type ConditionalNested<T> = T extends string ? (T extends "a" ? 1 : 2) : 3;