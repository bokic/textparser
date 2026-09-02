// 10h: Object and array literals, computed names, methods, accessors, spread.
const emptyObject = {};
const basicObject = { a: 1, b: "two", c: true, d: null };
const shorthand = { short, property, method() { return this; } };
const shorthandWithValue = { a } = source;
const computedName = { [dynamicKey]: value, [`prefix-${suffix}`]: other };

function accessorFactory() {
    const methodObject = {
        plain() {},
        computed() {},
        async asyncMethod() {},
        *generator() {
            yield 1;
        },
        async *asyncGen() {
            yield 1;
        },
        get accessor() {
            return 1;
        },
        set accessor(value) {},
        get [dynamicGetter]() {
            return 2;
        },
        set [dynamicSetter](value) {},
    };
    return methodObject;
}

function accessorOnlyFactory() {
    const accessorOnly = {
        get value() { return stored; },
        set value(v) { stored = v; },
    };
    return accessorOnly;
}
const spreadObject = { ...source, extra: 1, ...nested };
const spreadFirst = { ...base, override: true };
const nestedObject = { outer: { inner: { deepest: 1 } } };
const unicodeKeys = { \u0061: 1, \u{62}: 2 };
const quotedKeys = { "key with space": 1, 'single': 2 };
const numericKeys = { 0: "zero", 1.5: "one-five", 1e2: "hundred" };
const mixedObject = {
    ...spread,
    [computed]: value,
    shorthand,
    explicit: "value",
    nested: { deep: true },
};

const emptyArray = [];
const basicArray = [1, 2, 3];
const sparseArray = [1, , 3];
const trailingComma = [1, 2, 3,];
const holesOnly = [,,];
const nestedArrays = [[1, [2, [3]]]];
const mixedArray = [1, "two", true, null, undefined, { object: 1 }, [array]];
const arrayWithSpread = [prefix, ...middle, suffix];
const newlinesInArray = [
    first,
    second,
];
const elisions = [1, , , , 5];

const objectNestedArrays = { items: [1, 2], matrix: [[1], [2]] };
const arrayNestedObjects = [{ a: 1 }, { b: 2 }];

const objectWithMethods = {
    name: "x",
    toString() {},
    valueOf() { return this; },
    [Symbol.iterator]() { return iterator; },
};

const emptyObjectWithImportant = { };
const objectInExpression = fn({ config: 1, flags: [true, false] });