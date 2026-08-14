const fs = require('fs');
const path = require('path');
const { TextParserWasm } = require('./textparser_wrapper.js');

async function testWasm() {
  console.log('Initializing WebAssembly module...');
  const textparser = await TextParserWasm.init();
  console.log('Module initialized.');

  const sampleJsonDef = {
    name: 'SimpleCFML',
    caseSensitivity: false,
    defaultFileExtensions: ['cfm', 'cfc'],
    startTokens: ['Tag'],
    tokens: {
      Tag: {
        type: 'StartStop',
        startRegex: '<cf[a-zA-Z0-9_]+',
        endRegex: '>',
        otherTextInside: true
      }
    }
  };

  console.log('Loading language definition...');
  const lang = textparser.loadLanguage(sampleJsonDef);
  console.log('Language loaded successfully.');

  const code = '<cfset foo = "bar">';
  console.log(`Parsing code: "${code}"`);
  const result = textparser.parse(code, lang);
  const ast = result.toAST();
  console.log('AST Output:', JSON.stringify(ast, null, 2));

  result.close();
  lang.free();
  console.log('Test completed successfully.');
}

testWasm().catch(err => {
  console.error('Test failed:', err);
  process.exit(1);
});
