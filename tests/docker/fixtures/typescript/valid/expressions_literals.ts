// 10a/10b/10h/10k: literals, primitive types, and expression operators.
const answer: number = 42;
const ratio: number = 3.14e-2;
const big: bigint = 10n;
const binary: number = 0b1010;
const hex: number = 0xdead_beef;
const oct: number = 0o755;
const text: string = "hello";
const single: string = 'world';
const flag: boolean = true;
const nothing: null = null;
const maybe: undefined = undefined;

let result = answer + ratio * (hex - oct) % big;
result = result ** 2;
result = answer / ratio;
result += 1;
result -= 1;
result *= 2;
result /= 4;
result %= 7;
result **= 2;
result <<= 1;
result >>= 1;
result >>>= 1;
result |= 8;
result &= 15;
result ^= 3;
let nullable = result ?? 0;
nullable ??= 5;
let truthy = result;
truthy &&= 10;
truthy ||= 20;

const negated: boolean = !flag;
const bitwiseNot: number = ~answer;
const unaryMinus: number = -answer;
const unaryPlus: number = +answer;
const prefixInc: number = ++counter_tmp;

const compared: boolean = answer < ratio && ratio <= hex || oct >= binary && hex != big;
const equality: boolean = answer === ratio || answer !== big;
const ternary: number = flag ? answer : ratio;

const isMember: boolean = name in record || value instanceof Klass;
const voided: void = void 0;
const typeofValue: string = typeof text;