# TextParser WebAssembly

This directory provides WebAssembly bindings for `libtextparser`, allowing client-side web applications and Node.js applications to parse text using `textparser` language definitions directly in JavaScript/TypeScript.

## Directory Contents

- [textparser_wasm.c](file:///home/boris/projects/textparser/ports/webassembly/textparser_wasm.c): C WebAssembly interface exporting textparser functionality (`EMSCRIPTEN_KEEPALIVE`).
- [textparser_wrapper.js](file:///home/boris/projects/textparser/ports/webassembly/textparser_wrapper.js): High-level JavaScript wrapper class (`TextParserWasm`, `TextParserLanguage`, `TextParserResult`, `TextParserToken`).
- [build.sh](file:///home/boris/projects/textparser/ports/webassembly/build.sh): Automated Emscripten build script to compile dependencies (`PCRE2` and `json-c` for WASM) and generate `textparser.js` and `textparser.wasm`.
- [test_wasm.js](file:///home/boris/projects/textparser/ports/webassembly/test_wasm.js): Node.js unit test for verifying the WebAssembly build and API wrapper.

## Build Instructions

Ensure [Emscripten](https://emscripten.org/) (`emcc`, `emcmake`) is installed and available in your environment path:

```bash
cd ports/webassembly
./build.sh
```

This generates:
- `ports/webassembly/textparser.js`: Emscripten module loader script.
- `ports/webassembly/textparser.wasm`: Binary WebAssembly compiled parser library.

## Usage in Web Applications & Node.js

```javascript
const { TextParserWasm } = require('./textparser_wrapper.js');

async function run() {
  // Initialize the WebAssembly module
  const textparser = await TextParserWasm.init();

  // Load language definition from JS object or JSON string
  const lang = textparser.loadLanguage({
    name: 'SimpleCFML',
    caseSensitivity: false,
    defaultFileExtensions: ['cfm'],
    startTokens: ['Tag'],
    tokens: {
      Tag: {
        type: 'StartStop',
        startRegex: '<cf[a-zA-Z0-9_]+',
        endRegex: '>',
        otherTextInside: true
      }
    }
  });

  // Parse source text
  const result = textparser.parse('<cfset name = "world">', lang);
  
  // Get AST structure
  const ast = result.toAST();
  console.log(JSON.stringify(ast, null, 2));

  // Clean up WebAssembly allocations
  result.close();
  lang.free();
}

run();
```

## Running Tests

```bash
node ports/webassembly/test_wasm.js
```
