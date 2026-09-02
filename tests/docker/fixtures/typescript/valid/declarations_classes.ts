// 10d/10l: Declarations and classes, including modern class/member edge cases.
export class Shape {
    static readonly origin = "O";
    readonly kind: string = "shape";
    private _size: number = 0;

    constructor(private readonly name: string, public x = 0, protected y?: number) {
        this._size = x;
    }

    get size(): number {
        return this._size;
    }

    set size(value: number) {
        this._size = value;
    }

    area(): number {
        return this.x * this.y;
    }

    static describe(): string {
        return "shape";
    }

    protected helper(value: number): number {
        return value;
    }
}

export abstract class Base {
    abstract run(): void;
    constructor(public readonly id: number) {}
    protected abstract name: string;
}

export class Derived extends Base {
    protected name: string = "derived";
    override run(): void {}
    declare field: number;
    declare readonly declaredOnly: string;
    accessor auto: number = 1;
    override readonly accessor autoAccessor = 2;
}

export class WithIndexSignature {
    [key: string]: unknown;
    [index: number]: unknown;
}

export class WithOverloads {
    method(x: string): string;
    method(x: number): number;
    method(x: string | number): string | number {
        return x;
    }
}

export class WithStaticBlock {
    static {
        const loaded = initialize();
        register(loaded);
    }
}

export class WithAccessors {
    get value(): number {
        return 1;
    }
    set value(v: number) {}
}

export class WithParameterProperties {
    constructor(
        public a: number,
        protected b: string,
        private c: boolean,
        readonly d: symbol,
        public readonly e: bigint,
        private readonly f: null,
        protected g?: unknown
    ) {}
}

export class WithPrivateMembers {
    #privateField: number = 0;
    #privateMethod(): number {
        return this.#privateField;
    }
    use() {
        return this.#privateMethod();
    }
}

export class Ambient {
    foo: number;
    constructor(bar: string);
    method(): void;
}

const instance = new Shape("s", 1, 2);
const abstractType: typeof Base = Derived;

export interface Empty {}
export interface Extending extends Base, Named {}

enum Direction {
    Up,
    Down = 2,
    Left = Direction.Right * 2,
    Right = 4,
}

const enum ConstEnum {
    A = 1,
    B = A * 2,
}

declare function globalFn(x: number): string;
declare const globalVar: number;
declare let mutableGlobal: string;