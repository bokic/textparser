#!/usr/bin/env node
// 10m/10n: Lexer completeness - unicode identifiers, escapes, full literals,
// hashbang, comments, and source-file profile contexts.
import { helper } from "./utils";

const café = 1;
const naïve = 2;
const π = 3.14;
const 变量 = "variable";
const 質問 = 42;

const escapedIdentifier = \u0061bc;
const escapedInName = ab\u0063;

const stringSingle = 'single \' quote';
const stringDouble = "double \" quote";
const stringEscapes = "line\nfeed\t tab\\ backslash \u1234 \x41";
const templateWithEscapes = `nested \` inside`;
const unterminatedNo = "complete";

const numberDecimal = 123.456;
const numberExponent = 1.5e10;
const numberExponentNeg = 1.5e-10;
const numberBigInt = 9007199254740991n;
const numberSeparator = 1_000_000;
const numberBinary = 0b0101_0101;
const numberOctal = 0o777;
const numberHex = 0xDEAD_BEEF;
const numberHexEscaped = 0xDEADBEEF;
const numberZero = 0;
const numberLeadingDot = .5;
const numberTrailingDot = 5.;
const numberNegativeZero = -00;
const numberPlus = +5e1;

// line comment
/* block comment */
/* multi
   line */
/** document and multi
  block **/

const re1 = /abc/;
const re2 = /[a-z]+/gi;
const re3 = /https?:\/\/example\.com/;
const re4 = /\d{2}-\d{2}-\d{4}/u;
const re5 = /[/\\]/msy;
const division = value / divisor;
const regexAfter = /after/;
const divisionAfter = a / b / c;

const templateSimple = `text`;
const templateExpression = `text ${value} more`;
const templateEmpty = ``;
const taggedString = tag`hello ${world}`;

// JSX profile files use .tsx; declaration files use .d.ts.
declare const ambientGlobal: number;
type AmbientType = string;

export { helper };

const unicodeSequence = "\u{1F600}";
const asciiControl = "\x00\x1F";
const lengthWithAstralPair = "𐐷";