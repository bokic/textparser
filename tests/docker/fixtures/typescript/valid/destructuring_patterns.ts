// 10j: Binding and assignment patterns, destructuring, rest, defaults.
const { a, b } = object;
const { x: renamed, y: alsoRenamed = 1 } = object;
const { nested: { deep } } = object;
const { ["computed"]: computedKey } = object;
const { ...rest } = object;
const { p, q, ...others } = object;

const [first, second] = array;
const [head, ...tail] = array;
const [a1, , a3] = array;
const [nested, [inner]] = array;
const [, , thirdWithDefault = 3] = array;
const [alphabet, ...grouped] = array;

let [swap1, swap2] = pair;
[swap1, swap2] = [swap2, swap1];

let target1;
let target2;
({ x: target1, y: target2 } = object);
[target1, target2] = array;

for (const { key, value } of entries) {}
for (const [id, label] of pairs) {}
for (let { x, y } of points) {}

try {
} catch ({ message }) {}

const { a: { b: deepest } = {} } = object;
const [{ firstItem }] = array;

function fn({ a = 1, b: { c } = {} }, [d, e = 2]) {}

class Destructured {
    constructor({ options, flags = [] }: Config) {}
    method([first, ...rest]: number[]): void {}
}

const [x1, x2 = 2, ...xRest] = [1];
const { y1, y2: renamed2 = "default", ...yRest } = {};

const [,,] = sparseArray;
const { z, ...zRest } = { z: 1 };

let assignmentTarget = 0;
[assignmentTarget, ...assignmentRest] = source;
({ prop: assignmentTarget } = source);