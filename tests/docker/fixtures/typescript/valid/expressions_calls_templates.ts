// 10g/10k: Core expression structure, calls, member access, new, optional chains,
// generic calls, templates, and remaining assertions.
const callResult = process(1, 2, 3);
const memberAccess = object.property;
const computedAccess = object["property"];
const chained = object.deeply.nested.value;
const called = object.method(argument);
const indexResult = array[0];

const newInstance = new Constructor();
const newWithArgs = new Constructor(1, 2);
const newExpression = new (getClass())(param);

const optionalCall = fn?.();
const optionalMember = obj?.prop;
const optionalChain = obj?.deep?.path ?? fallback;
const optionalIndex = arr?.[0];
const optionalCallChain = obj.method?.();

const nonNull = value!;
const doubleBang = value!!;
const assertedMember = obj!.prop;
const nonNullCall = fn()!.result;

const genericCall = map<number, string>(item);
const genericMethod = obj.call<"literal">();
const genericNew = new Factory<number>();
const instantiationExpression = identity<string>;
const instantiationCall = process<number>(input);

const taggedResult = tag`template ${value} end`;
const rawTagged = String.raw`path\\name`;
const template = `value: ${expression}`;
const nested = `outer ${`inner ${deepest}`} end`;
const multiline = `line1
line2`;
const escaped = `tab:\tquote:\``;

const assertion = <string>value;
const typeAssertion = value as string;
const constAssertion = value as const;
const satisfiesAssertion = value satisfies Interface;

const awaitExpression = await promise;

const inOperator = key in object;
const instanceOf = obj instanceof Class;
const typeofCheck = typeof value;
const deleteOperator = delete object.prop;
const voidOperator = void 0;

const spreadInCall = process(...args);
const spreadInNew = new Ctor(...args);
const spreadInArray = [...values, last];

const commaExpression = (first, second, third);
const parenthesized = (expression);
const groupingNested = ((value));

const chainedCalls = a.b().c().d;
const mixedChain = obj?.method()!.value ?? fallback;

const optionalChainMethods = {
    a: obj?.b?.(),
    b: obj?.c(),
    c: obj?.[key],
};

const newWithComplexArgs = new Worker("name", { options: { retry: 3 } });

const bitwiseShift = value << 1 >> 2 >>> 3;
const relational = value < other ? 1 : value > other ? 2 : 0;
const equalityConst = a === b && c !== d;
const addSub = a + b - c;
const mulDivMod = a * b / c % d;
const logicalAnd = a && b;
const logicalOr = a || b;
const nullish = a ?? b;
const exponent = a ** b;
const inNested = (a in b) && c;