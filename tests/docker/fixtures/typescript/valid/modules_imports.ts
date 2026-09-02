// 10e: Imports, exports, namespaces, and modules.
import { helper, CONSTANT as renamed } from "./util";
import defaultExport from "./default";
import * as allExports from "./all";
import type { OnlyType } from "./types";
import { type InlineType, value } from "./mixed";
import "./side-effect";
import { "quoted name" as local } from "./quoted";
import defer * as deferredNamespace from "./deferred";

export { helper };
export { CONSTANT as exportedConstant };
export { default as renamedDefault } from "./default";
export * from "./everything";
export * as all from "./all";
export { type OnlyType as RenamedType } from "./types";
export import reexport = ImportAlias;

export const topLevelValue: number = 1;
export let mutableValue: string = "x";
export var oldStyleValue: boolean = true;
export type ExportedType = { field: number };
export interface ExportedInterface {}
export default function defaultFunc() {}
export default class DefaultClass {}
export default 42;

namespace Internal {
    export const value = 1;
    export function inner(): number {
        return value;
    }
}

module LegacyModule {
    export interface Legacy {}
}

namespace Outer {
    export namespace Inner {
        export type Nested = string;
    }
}

import { helper as h } from "./util";
import ImportAlias = require("./alias");
import ImportAndExport = Out.ImportAndExport;

export as namespace MyGlobal;

declare module "external-module" {
    export interface Patch {
        added: boolean;
    }
}

export interface NamedImportType {
    use: typeof DeferredImport;
}

const imported = import("./dynamic");
const metaUrl: string = import.meta.url;

export default interface DefaultInterface {
    ok: true;
}