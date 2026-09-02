# Fixture corpus parity: textparser vs tree-sitter-typescript

textparser statuses/diagnostics come from the committed golden CSTs (`tests/docker/fixtures/typescript/golden/*.json`); tree-sitter reports whether its root tree contains an ERROR node. A fixture is taken to be *rejected* by textparser when its status is not `OK` or it carries at least one diagnostic; tree-sitter rejects when it has an ERROR node.

| fixture | tp status | tp diagnostics | tp rejects | ts rejects | parity |
|---|---|---|---|---|---|
| declaration_file.d.ts (valid) | OK | 0 | False | False | OK |
| declarations_classes.ts (valid) | OK | 0 | False | True | MISMATCH |
| destructuring_patterns.ts (valid) | OK | 0 | False | False | OK |
| expressions_calls_templates.ts (valid) | OK | 0 | False | False | OK |
| expressions_literals.ts (valid) | OK | 0 | False | False | OK |
| functions_arrows.ts (valid) | OK | 0 | False | False | OK |
| jsx.tsx (valid) | OK | 0 | False | False | OK |
| lexer_syntax_profiles.ts (valid) | OK | 0 | False | False | OK |
| literals_objects_arrays.ts (valid) | OK | 0 | False | False | OK |
| modules_imports.ts (valid) | OK | 0 | False | True | MISMATCH |
| statements_control_flow.ts (valid) | OK | 0 | False | False | OK |
| types.ts (valid) | OK | 0 | False | False | OK |
| recovery_class_member.ts (invalid) | OK | 2 | True | True | OK |
| recovery_statement.ts (invalid) | OK | 2 | True | True | OK |
| recovery_switch_clause.ts (invalid) | OK | 3 | True | True | OK |
| recovery_type_member.ts (invalid) | OK | 2 | True | True | OK |
| terminal_no_resume.ts (invalid) | NO | 1 | True | True | OK |

## Distinct CST node kinds per parser (whole corpus)

| textparser kind | count | | tree-sitter kind | count |
|---|---|---|---|---|
| Identifier | 983 | | identifier | 801 |
| Sequence | 612 | | tok:; | 453 |
| Semicolon | 440 | | tok:= | 328 |
| Assign | 325 | | tok:: | 257 |
| PostfixType | 312 | | variable_declarator | 243 |
| IntersectionType | 309 | | lexical_declaration | 242 |
| ConditionalType | 293 | | tok:} | 226 |
| UnionType | 293 | | tok:const | 225 |
| Colon | 248 | | tok:{ | 212 |
| VariableDeclaration | 241 | | tok:) | 194 |
| VariableDeclarationList | 241 | | predefined_type | 191 |
| VariableStatement | 240 | | property_identifier | 189 |
| ConstKeyword | 223 | | tok:( | 186 |
| Repeat | 192 | | type_annotation | 185 |
| TypeContext | 188 | | number | 156 |
| LBrace | 186 | | tok:, | 151 |
| RBrace | 186 | | tok: | 138 |
| LParen | 183 | | type_identifier | 130 |
| RParen | 183 | | formal_parameters | 103 |
| Comma | 150 | | tok:number | 90 |
| NumericLiteral | 150 | | statement_block | 80 |
| TypeAnnotation | 139 | | tok:] | 78 |
| NumberKeyword | 88 | | tok:[ | 77 |
| BindingParameterList | 80 | | string | 72 |
| LBracket | 77 | | required_parameter | 68 |
| RBracket | 77 | | tok: (string_fragment)  | 64 |
| BlockStatement | 73 | | tok:> | 60 |
| StringLiteral | 72 | | binary_expression | 57 |
| JSXIdentifier | 69 | | tok:string | 56 |
| BindingParameter | 57 | | tok:< | 53 |
| QualifiedName | 56 | | expression_statement | 51 |
| PostfixExpressionSuffix | 55 | | tok:type | 49 |
| StringKeyword | 55 | | export_statement | 48 |
| BindingParameterListElements | 54 | | tok:export | 48 |
| ArrayElement | 51 | | arguments | 46 |
| StatementList | 51 | | type_alias_declaration | 46 |
| TypeKeyword | 48 | | call_expression | 41 |
| ExportKeyword | 47 | | tok:` | 40 |
| ExpressionStatement | 46 | | pair | 38 |
| JSXElementName | 45 | | member_expression | 36 |
| TypeAliasDeclaration | 45 | | method_definition | 36 |
| TypeReference | 45 | | object | 32 |
| Arguments | 44 | | tok:. | 32 |
| ObjectPropertyAssignment | 41 | | string_fragment | 30 |
| BasePrimaryExpression | 37 | | literal_type | 29 |
| ObjectLiteralBody | 35 | | return_statement | 29 |
| ExportedDeclaration | 34 | | tok:function | 29 |
| ClassElement | 32 | | tok:return | 29 |
| Dot | 32 | | array | 25 |
| JSXOpen | 31 | | tok:... | 24 |
| BindingElement | 30 | | tok:=> | 24 |
| FunctionKeyword | 29 | | tok:let | 24 |
| ReturnKeyword | 29 | | object_pattern | 23 |
| ReturnStatement | 29 | | property_signature | 23 |
| ArrayLiteralExpression | 28 | | parenthesized_expression | 22 |
| ObjectBindingElements | 28 | | tok:void | 22 |
| ObjectBindingProperty | 28 | | shorthand_property_identifier_pattern | 21 |
| ArrayBindingElements | 27 | | comment | 20 |
| Argument | 25 | | jsx_attribute | 20 |
| FunctionDeclaration | 25 | | jsx_closing_element | 20 |
| Arrow | 24 | | jsx_element | 20 |
| Ellipsis | 24 | | jsx_opening_element | 20 |
| LetKeyword | 24 | | tok:</ | 20 |
| JSXAttribute | 22 | | function_declaration | 19 |
| LessThan | 22 | | jsx_expression | 19 |
| ObjectType | 21 | | array_pattern | 18 |
| PropertySignature | 21 | | tok:* | 18 |
| VoidKeyword | 21 | | ( | 17 |
| GreaterThan | 20 | | class_body | 17 |
| JSXCloseStart | 20 | | tok:/ | 17 |
| JSXClosingTagEnd | 20 | | tok:class | 17 |
| JSXTagEnd | 20 | | tok:declare | 17 |
| MethodDeclaration | 20 | | tok:program | 17 |
| ObjectBindingPattern | 20 | | tok:import | 16 |
| JSXExpressionEnd | 19 | | tok:| | 16 |
| JSXExpressionStart | 19 | | union_type | 16 |
| JSXText | 19 | | ambient_declaration | 15 |
| Multiply | 18 | | augmented_assignment_expression | 15 |
| Capture | 17 | | public_field_definition | 15 |
| DeclareKeyword | 17 | | tok:? | 15 |
| JSXExpressionContainer | 17 | | accessibility_modifier | 14 |
| JSXPairedElement | 17 | | class_declaration | 14 |
| BitOr | 16 | | object_type | 14 |
| ImportKeyword | 16 | | template_string | 14 |
| ParameterList | 16 | | tok:${ | 14 |
| Question | 16 | | unary_expression | 14 |
| ArrayBindingPattern | 15 | | ERROR | 13 |
| ClassBody | 15 | | arrow_function | 13 |
| ClassKeyword | 15 | | jsx_text | 13 |
| ArrowBody | 13 | | rest_pattern | 13 |
| ClassDeclaration | 13 | | tok:from | 13 |
| FromKeyword | 13 | | tok:as | 12 |
| ObjectMethodDeclaration | 13 | | tok:readonly | 12 |
| TemplateHead | 13 | | array_type | 11 |
| TemplateTail | 13 | | assignment_expression | 11 |
| AsKeyword | 12 | | function_type | 11 |
| PropertyDeclaration | 12 | | import_statement | 11 |
| ReadonlyKeyword | 12 | | jsx_self_closing_element | 11 |
| SourceFile | 12 | | method_signature | 11 |
| ThisKeyword | 12 | | pair_pattern | 11 |
| TypeParameterDeclaration | 12 | | tok:/> | 11 |
| TypeSuffix | 12 | | tok:?. | 11 |
| BooleanKeyword | 11 | | tok:boolean | 11 |
| FunctionType | 11 | | type_arguments | 11 |
| JSXSelfClosingElement | 11 | | interface_body | 10 |
| JSXSelfClosingEnd | 11 | | interface_declaration | 10 |
| OptionalChain | 11 | | spread_element | 10 |
| TypeList | 11 | | this | 10 |
| AsyncKeyword | 10 | | tok:interface | 10 |
| ImportDeclaration | 10 | | import_clause | 9 |
| TypeArguments | 10 | | null | 9 |
| InterfaceKeyword | 9 | | true | 9 |
| NullKeyword | 9 | | import_specifier | 8 |
| TrueKeyword | 9 | | new_expression | 8 |
| TupleElement | 9 | | optional_chain | 8 |
| EnumMember | 8 | | template_substitution | 8 |
| ForKeyword | 8 | | tok:! | 8 |
| ForStatement | 8 | | tok:+ | 8 |
| ImportSpecifier | 8 | | tok:async | 8 |
| InterfaceDeclaration | 8 | | tok:extends | 8 |
| LogicalNot | 8 | | tok:for | 8 |
| NewExpression | 8 | | tok:new | 8 |
| NewKeyword | 8 | | tok:yield | 8 |
| ParenthesizedArrowFunction | 8 | | type_parameter | 8 |
| Plus | 8 | | type_parameters | 8 |
| TemplateLiteral | 8 | | yield_expression | 8 |
| TypeParameter | 8 | | assignment_pattern | 7 |
| TypeParameters | 8 | | computed_property_name | 7 |
| YieldExpression | 8 | | enum_assignment | 7 |
| YieldKeyword | 8 | | tok:unknown | 7 |
| ExtendsKeyword | 7 | | index_signature | 6 |
| NoSubstitutionTemplateLiteral | 7 | | named_imports | 6 |
| ObjectPropertyName | 7 | | non_null_expression | 6 |
| TypeParametersContext | 7 | | regex | 6 |
| UnknownKeyword | 7 | | regex_pattern | 6 |
| AbstractKeyword | 6 | | template_literal_type | 6 |
| ConstructorDeclaration | 6 | | template_type | 6 |
| ConstructorKeyword | 6 | | tok:&& | 6 |
| DefaultKeyword | 6 | | tok:' | 6 |
| LogicalAnd | 6 | | tok:abstract | 6 |
| NamedImports | 6 | | tok:default | 6 |
| NamespaceDeclaration | 6 | | tok:get | 6 |
| ObjectAccessorDeclaration | 6 | | tok:in | 6 |
| ProtectedKeyword | 6 | | tok:protected | 6 |
| RegularExpressionLiteral | 6 | | tok:set | 6 |
| <no-cst> | 5 | | await_expression | 5 |
| AwaitKeyword | 5 | | break_statement | 5 |
| DeclaredVariableStatement | 5 | | escape_sequence | 5 |
| ExportSpecifier | 5 | | export_clause | 5 |
| ForBinding | 5 | | export_specifier | 5 |
| GetKeyword | 5 | | for_in_statement | 5 |
| InKeyword | 5 | | function_signature | 5 |
| IndexSignature | 5 | | generic_type | 5 |
| Minus | 5 | | optional_parameter | 5 |
| NamedExportDeclaration | 5 | | switch_case | 5 |
| NamedExports | 5 | | tok:- | 5 |
| NamespaceKeyword | 5 | | tok:await | 5 |
| ObjectBindingRestElement | 5 | | tok:break | 5 |
| SetKeyword | 5 | | tok:case | 5 |
| Slash | 5 | | tok:namespace | 5 |
| TemplateLiteralType | 5 | | tok:typeof | 5 |
| TupleType | 5 | | tuple_type | 5 |
| TypeofKeyword | 5 | | conditional_type | 4 |
| AccessorKeyword | 4 | | empty_statement | 4 |
| ArrayBindingRestElement | 4 | | if_statement | 4 |
| CaseClause | 4 | | internal_module | 4 |
| DefaultExportDeclaration | 4 | | parenthesized_type | 4 |
| IfKeyword | 4 | | private_property_identifier | 4 |
| IfStatement | 4 | | subscript_expression | 4 |
| LogicalOr | 4 | | tok:?? | 4 |
| MethodSignature | 4 | | tok:if | 4 |
| NullishCoalesce | 4 | | tok:of | 4 |
| ObjectSpreadAssignment | 4 | | tok:private | 4 |
| OfKeyword | 4 | | tok:public | 4 |
| ParenthesizedType | 4 | | tok:|| | 4 |
| PrivateIdentifier | 4 | | undefined | 4 |
| PrivateKeyword | 4 | | update_expression | 4 |
| PublicKeyword | 4 | | abstract_method_signature | 3 |
| UndefinedKeyword | 4 | | continue_statement | 3 |
| BigIntKeyword | 3 | | else_clause | 3 |
| BitAnd | 3 | | enum_body | 3 |
| BreakKeyword | 3 | | enum_declaration | 3 |
| BreakStatement | 3 | | for_statement | 3 |
| CaseKeyword | 3 | | function_expression | 3 |
| ElseKeyword | 3 | | intersection_type | 3 |
| EnumDeclaration | 3 | | jsx_namespace_name | 3 |
| EnumKeyword | 3 | | regex_flags | 3 |
| ImportEqualsDeclaration | 3 | | shorthand_property_identifier | 3 |
| Increment | 3 | | statement_identifier | 3 |
| JSXFragment | 3 | | switch_body | 3 |
| ObjectKeyword | 3 | | switch_statement | 3 |
| OverrideKeyword | 3 | | ternary_expression | 3 |
| StaticKeyword | 3 | | tok:& | 3 |
| SymbolKeyword | 3 | | tok:++ | 3 |
| TypeOperatorOrPostfixType | 3 | | tok:continue | 3 |
| TypeQuery | 3 | | tok:else | 3 |
| WhileKeyword | 3 | | tok:enum | 3 |
| AsyncArrowFunction | 2 | | tok:static | 3 |
| BigIntLiteral | 2 | | tok:switch | 3 |
| CatchClause | 2 | | tok:while | 3 |
| CatchKeyword | 2 | | tok:with | 3 |
| Exponent | 2 | | type_query | 3 |
| ExportAllDeclaration | 2 | | abstract_class_declaration | 2 |
| FalseKeyword | 2 | | as_expression | 2 |
| GenericArrowFunction | 2 | | catch_clause | 2 |
| ImportClause | 2 | | class_heritage | 2 |
| InstanceofKeyword | 2 | | extends_clause | 2 |
| KeyofKeyword | 2 | | false | 2 |
| ModuleKeyword | 2 | | generator_function_declaration | 2 |
| NamespaceImport | 2 | | import | 2 |
| NeverKeyword | 2 | | import_alias | 2 |
| Remainder | 2 | | index_type_query | 2 |
| RestBindingParameter | 2 | | module | 2 |
| StrictEqual | 2 | | namespace_import | 2 |
| StrictNotEqual | 2 | | object_assignment_pattern | 2 |
| ThrowKeyword | 2 | | override_modifier | 2 |
| ThrowStatement | 2 | | sequence_expression | 2 |
| TryKeyword | 2 | | switch_default | 2 |
| TryStatement | 2 | | this_type | 2 |
| TypePredicate | 2 | | throw_statement | 2 |
| WhileStatement | 2 | | tok:!== | 2 |
| AnyKeyword | 1 | | tok:% | 2 |
| AssertsKeyword | 1 | | tok:** | 2 |
| BitAndAssign | 1 | | tok:=== | 2 |
| BitNot | 1 | | tok:any | 2 |
| BitOrAssign | 1 | | tok:catch | 2 |
| BitXorAssign | 1 | | tok:instanceof | 2 |
| CallSignature | 1 | | tok:keyof | 2 |
| ClassStaticBlock | 1 | | tok:module | 2 |
| ContinueKeyword | 1 | | tok:never | 2 |
| ContinueStatement | 1 | | tok:object | 2 |
| Decrement | 1 | | tok:override | 2 |
| DefaultClassDeclaration | 1 | | tok:symbol | 2 |
| DefaultFunctionDeclaration | 1 | | tok:throw | 2 |
| DefaultInterfaceDeclaration | 1 | | tok:try | 2 |
| DeferKeyword | 1 | | tok:unique symbol | 2 |
| DeleteKeyword | 1 | | try_statement | 2 |
| DivideAssign | 1 | | while_statement | 2 |
| DoKeyword | 1 | | MISSING | 1 |
| DoStatement | 1 | | asserts | 1 |
| DynamicImportExpression | 1 | | call_signature | 1 |
| ExponentAssign | 1 | | class | 1 |
| ExportAsNamespaceDeclaration | 1 | | class_static_block | 1 |
| ExportedImportEqualsDeclaration | 1 | | constraint | 1 |
| FinallyClause | 1 | | do_statement | 1 |
| FinallyKeyword | 1 | | extends_type_clause | 1 |
| GreaterEqual | 1 | | finally_clause | 1 |
| Hashbang | 1 | | hash_bang_line | 1 |
| IdentifierArrowFunction | 1 | | import_require_clause | 1 |
| ImportMetaExpression | 1 | | infer_type | 1 |
| ImportType | 1 | | instantiation_expression | 1 |
| InferKeyword | 1 | | labeled_statement | 1 |
| InferType | 1 | | lookup_type | 1 |
| IsKeyword | 1 | | mapped_type_clause | 1 |
| LabeledStatement | 1 | | meta_property | 1 |
| LeftShift | 1 | | namespace_export | 1 |
| LeftShiftAssign | 1 | | nested_identifier | 1 |
| LessEqual | 1 | | opting_type_annotation | 1 |
| LiteralType | 1 | | optional_type | 1 |
| LogicalAndAssign | 1 | | rest_type | 1 |
| LogicalOrAssign | 1 | | satisfies_expression | 1 |
| MappedType | 1 | | tok:  | 1 |
| MinusAssign | 1 | | tok: (escape_sequence)  | 1 |
| MultiplyAssign | 1 | | tok: (escape_sequence) (escape_sequence)  | 1 |
| NotEqual | 1 | | tok: (string_fragment) (escape_sequence) (string_fragment)  | 1 |
| NullishCoalesceAssign | 1 | | tok: (string_fragment) (escape_sequence) (string_fragment) (escape_sequence) (string_fragment) (escape_sequence) (string_fragment) (escape_sequence) (string_fragment) (escape_sequence)  | 1 |
| PlusAssign | 1 | | tok:!= | 1 |
| RemainderAssign | 1 | | tok:%= | 1 |
| RequireKeyword | 1 | | tok:&&= | 1 |
| RightShift | 1 | | tok:&= | 1 |
| RightShiftAssign | 1 | | tok:**= | 1 |
| SatisfiesKeyword | 1 | | tok:*= | 1 |
| SwitchKeyword | 1 | | tok:+= | 1 |
| SwitchStatement | 1 | | tok:-- | 1 |
| TemplateMiddle | 1 | | tok:-= | 1 |
| UniqueKeyword | 1 | | tok:/= | 1 |
| UnsignedRightShift | 1 | | tok:<< | 1 |
| UnsignedRightShiftAssign | 1 | | tok:<<= | 1 |
| VarKeyword | 1 | | tok:<= | 1 |
| WithKeyword | 1 | | tok:>= | 1 |
| WithStatement | 1 | | tok:>> | 1 |
| | | | tok:>>= | 1 |
| | | | tok:>>> | 1 |
| | | | tok:>>>= | 1 |
| | | | tok:?: | 1 |
| | | | tok:??= | 1 |
| | | | tok:^= | 1 |
| | | | tok:accessor | 1 |
| | | | tok:asserts | 1 |
| | | | tok:delete | 1 |
| | | | tok:do | 1 |
| | | | tok:finally | 1 |
| | | | tok:infer | 1 |
| | | | tok:is | 1 |
| | | | tok:meta | 1 |
| | | | tok:require | 1 |
| | | | tok:satisfies | 1 |
| | | | tok:var | 1 |
| | | | tok:|= | 1 |
| | | | tok:||= | 1 |
| | | | tok:~ | 1 |
| | | | type_assertion | 1 |
| | | | type_predicate | 1 |
| | | | variable_declaration | 1 |
| | | | with_statement | 1 |

Total distinct kinds: textparser=281, tree-sitter=305
