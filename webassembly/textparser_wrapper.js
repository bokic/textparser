class TextParserWasm {
  constructor(wasmModule) {
    this.module = wasmModule;
  }

  static async init(moduleOrOptions = {}) {
    let createModule;
    if (typeof window !== 'undefined' && window.TextParserModule) {
      createModule = window.TextParserModule;
    } else if (typeof require !== 'undefined') {
      createModule = require('./textparser.js');
    } else {
      createModule = (await import('./textparser.js')).default;
    }

    const wasmModule = await createModule(moduleOrOptions);
    return new TextParserWasm(wasmModule);
  }

  loadLanguage(jsonDef) {
    const jsonStr = typeof jsonDef === 'string' ? jsonDef : JSON.stringify(jsonDef);
    const jsonStrLen = this.module.lengthBytesUTF8(jsonStr) + 1;
    const jsonPtr = this.module._malloc(jsonStrLen);
    this.module.stringToUTF8(jsonStr, jsonPtr, jsonStrLen);

    const outDefPtr = this.module._malloc(4); // 32-bit pointer
    try {
      const res = this.module._wasm_textparser_load_language_json(jsonPtr, outDefPtr);
      if (res !== 0) {
        throw new Error(`Failed to load language definition (error code: ${res})`);
      }
      const langDef = this.module.getValue(outDefPtr, 'i32');
      return new TextParserLanguage(this.module, langDef);
    } finally {
      this.module._free(jsonPtr);
      this.module._free(outDefPtr);
    }
  }

  parse(text, language) {
    const textLen = this.module.lengthBytesUTF8(text);
    const textPtr = this.module._malloc(textLen + 1);
    this.module.stringToUTF8(text, textPtr, textLen + 1);

    const outHandlePtr = this.module._malloc(4);
    try {
      const openRes = this.module._wasm_textparser_openmem(textPtr, textLen, outHandlePtr);
      if (openRes !== 0) {
        throw new Error(`Failed to open memory buffer (error code: ${openRes})`);
      }
      const handle = this.module.getValue(outHandlePtr, 'i32');

      const parseRes = this.module._wasm_textparser_parse(handle, language.ptr);
      if (parseRes !== 0) {
        const errPtr = this.module._wasm_textparser_parse_error(handle);
        const errPos = this.module._wasm_textparser_parse_error_position(handle);
        const errStr = errPtr ? this.module.UTF8ToString(errPtr) : 'Unknown parse error';
        this.module._wasm_textparser_close(handle);
        throw new Error(`Parse failed at position ${errPos}: ${errStr}`);
      }

      const rootPtrPtr = this.module._malloc(4);
      const firstToken = this.module._wasm_textparser_get_first_token(handle);
      this.module.setValue(rootPtrPtr, firstToken, 'i32');
      this.module._wasm_textparser_post_process(rootPtrPtr, language.ptr);
      const postProcessedRoot = this.module.getValue(rootPtrPtr, 'i32');
      this.module._free(rootPtrPtr);

      return new TextParserResult(this.module, handle, postProcessedRoot, language);
    } finally {
      this.module._free(textPtr);
      this.module._free(outHandlePtr);
    }
  }
}

class TextParserLanguage {
  constructor(module, ptr) {
    this.module = module;
    this.ptr = ptr;
  }

  free() {
    if (this.ptr) {
      this.module._wasm_textparser_free_language(this.ptr);
      this.ptr = 0;
    }
  }
}

class TextParserResult {
  constructor(module, handle, rootTokenPtr, language) {
    this.module = module;
    this.handle = handle;
    this.rootTokenPtr = rootTokenPtr;
    this.language = language;
  }

  getRootToken() {
    return this.rootTokenPtr ? new TextParserToken(this.module, this.handle, this.rootTokenPtr, this.language) : null;
  }

  close() {
    if (this.handle) {
      this.module._wasm_textparser_close(this.handle);
      this.handle = 0;
    }
  }

  toAST() {
    const root = this.getRootToken();
    return root ? root.toAST() : null;
  }
}

class TextParserToken {
  constructor(module, handle, ptr, language) {
    this.module = module;
    this.handle = handle;
    this.ptr = ptr;
    this.language = language;
  }

  get id() {
    return this.module._wasm_token_get_id(this.ptr);
  }

  get position() {
    return this.module._wasm_token_get_position(this.ptr);
  }

  get length() {
    return this.module._wasm_token_get_length(this.ptr);
  }

  get typeStr() {
    const typePtr = this.module._wasm_token_get_type_str(this.language.ptr, this.ptr);
    return typePtr ? this.module.UTF8ToString(typePtr) : null;
  }

  get error() {
    const errPtr = this.module._wasm_token_get_error(this.ptr);
    return errPtr ? this.module.UTF8ToString(errPtr) : null;
  }

  get text() {
    const textPtr = this.module._wasm_token_get_text(this.handle, this.ptr);
    if (!textPtr) return '';
    const str = this.module.UTF8ToString(textPtr);
    this.module._wasm_free_token_text(textPtr);
    return str;
  }

  get child() {
    const childPtr = this.module._wasm_token_get_child(this.ptr);
    return childPtr ? new TextParserToken(this.module, this.handle, childPtr, this.language) : null;
  }

  get next() {
    const nextPtr = this.module._wasm_token_get_next(this.ptr);
    return nextPtr ? new TextParserToken(this.module, this.handle, nextPtr, this.language) : null;
  }

  get prev() {
    const prevPtr = this.module._wasm_token_get_prev(this.ptr);
    return prevPtr ? new TextParserToken(this.module, this.handle, prevPtr, this.language) : null;
  }

  get parent() {
    const parentPtr = this.module._wasm_token_get_parent(this.ptr);
    return parentPtr ? new TextParserToken(this.module, this.handle, parentPtr, this.language) : null;
  }

  getChildren() {
    const children = [];
    let curr = this.child;
    while (curr) {
      children.push(curr);
      curr = curr.next;
    }
    return children;
  }

  toAST() {
    const ast = {
      id: this.id,
      type: this.typeStr,
      position: this.position,
      length: this.length,
      text: this.text,
      children: this.getChildren().map(c => c.toAST())
    };
    if (this.error) ast.error = this.error;
    return ast;
  }
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = { TextParserWasm, TextParserLanguage, TextParserResult, TextParserToken };
} else if (typeof window !== 'undefined') {
  window.TextParserWasmWrapper = { TextParserWasm, TextParserLanguage, TextParserResult, TextParserToken };
}
