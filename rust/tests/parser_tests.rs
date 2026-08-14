use textparser::TextParser;

const JSON_DEF: &str = r#"{
  "name": "json",
  "version": 1.0,
  "otherTextInside": false,
  "startTokens": ["Object", "Array"],
  "tokens": {
    "Object": {
      "type": "StartStop",
      "startRegex": "\\{",
      "endRegex": "\\}",
      "otherTextInside": true,
      "nestedTokens": ["Key", "String", "KeyValueSeparator", "Value", "ValueSeparator"]
    },
    "Array": {
      "type": "StartStop",
      "startRegex": "\\[",
      "endRegex": "\\]",
      "otherTextInside": true,
      "nestedTokens": ["String", "Number", "Object", "Array", "Bool", "Null", "ValueSeparator"]
    },
    "Value": {
      "type": "GroupOneChildOnly",
      "nestedTokens": ["String", "Number", "Object", "Array", "Bool", "Null"]
    },
    "Key": {
      "type": "StartStop",
      "startRegex": "\"",
      "endRegex": "\"",
      "otherTextInside": true,
      "nestedTokens": ["StringChar"]
    },
    "String": {
      "type": "StartStop",
      "startRegex": "\"",
      "endRegex": "\"",
      "otherTextInside": true,
      "nestedTokens": ["StringChar"]
    },
    "StringChar": {
      "type": "SimpleToken",
      "startRegex": "[^\"\\\\\\r\\n]+"
    },
    "Number": {
      "type": "SimpleToken",
      "startRegex": "-?[0-9]+(?:\\.[0-9]+)?"
    },
    "Bool": {
      "type": "SimpleToken",
      "startRegex": "true|false"
    },
    "Null": {
      "type": "SimpleToken",
      "startRegex": "null"
    },
    "KeyValueSeparator": {
      "type": "SimpleToken",
      "startRegex": ":"
    },
    "ValueSeparator": {
      "type": "SimpleToken",
      "startRegex": ","
    }
  }
}"#;

#[test]
fn test_parse_json_object() {
    let parser = TextParser::from_json_str(JSON_DEF).unwrap();
    let text = r#"{"key": "value", "num": 123, "flag": true}"#;
    let tokens = parser.parse(text).unwrap();

    assert_eq!(tokens.len(), 1);
    assert_eq!(tokens[0].id, "Object");
    assert_eq!(tokens[0].position, 0);
    assert_eq!(tokens[0].length, text.len());
    assert!(!tokens[0].children.is_empty());
}

#[test]
fn test_parse_json_array() {
    let parser = TextParser::from_json_str(JSON_DEF).unwrap();
    let text = r#"[1, 2, "three", false, null]"#;
    let tokens = parser.parse(text).unwrap();

    assert_eq!(tokens.len(), 1);
    assert_eq!(tokens[0].id, "Array");
    assert_eq!(tokens[0].length, text.len());
}

#[test]
fn test_parse_format() {
    let parser = TextParser::from_json_str(JSON_DEF).unwrap();
    let text = r#"{"a": 1}"#;
    let formatted = parser.parse_format(text).unwrap();

    assert_eq!(formatted.len(), text.len());
    assert_ne!(formatted, vec![0u8; text.len()]);
}

#[test]
fn test_empty_and_whitespace_input() {
    let parser = TextParser::from_json_str(JSON_DEF).unwrap();
    let tokens = parser.parse("   \n\t  ").unwrap();
    assert!(tokens.is_empty());
}

#[test]
fn test_invalid_definition_json() {
    let res = TextParser::from_json_str("{ invalid json ");
    assert!(res.is_err());
}

#[test]
fn test_sign_merge_and_post_process() {
    let calc_def = r#"{
      "name": "calc",
      "startTokens": ["Expr"],
      "mergeSignIntoNumber": {
        "signTokens": ["Sign"],
        "numberTokens": ["Number", "Expr"],
        "operandTokens": ["Number", "Var"]
      },
      "tokens": {
        "Expr": {
          "type": "Group",
          "otherTextInside": true,
          "deleteIfOnlyOneChild": true,
          "nestedTokens": ["Sign", "Number", "Var", "Op"]
        },
        "Sign": {
          "type": "SimpleToken",
          "startRegex": "[-+]"
        },
        "Number": {
          "type": "SimpleToken",
          "startRegex": "[0-9]+"
        },
        "Var": {
          "type": "SimpleToken",
          "startRegex": "[a-zA-Z]+"
        },
        "Op": {
          "type": "SimpleToken",
          "startRegex": "[*/]"
        }
      }
    }"#;

    let parser = TextParser::from_json_str(calc_def).unwrap();
    let tokens = parser.parse("-123").unwrap();
    assert_eq!(tokens.len(), 1);
    assert_eq!(tokens[0].id, "Number");
    assert_eq!(tokens[0].position, 0);
    assert_eq!(tokens[0].length, 4);
}
