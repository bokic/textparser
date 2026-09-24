# Fixture corpus parity: textparser vs tree-sitter-cfml

`tp` = textparser (definitions/cfml_definition.json); `ts` = tree-sitter cfml/cfscript grammar at the rev pinned by the zed extension. A construct is *rejected* by textparser when its status is not `OK` or it carries diagnostics; tree-sitter rejects when its root tree has an ERROR node. `parity` is OK when both agree (both accept or both reject).

| construct | tp status | tp diag | tp rejects | ts rejects | parity |
|---|---|---|---|---|---|
| set_basic | OK | 0 | False | False | OK |
| set_expr | OK | 0 | False | False | OK |
| set_ternary | OK | 0 | False | False | OK |
| set_elvis | OK | 0 | False | False | OK |
| set_nullcoalesce | OK | 0 | False | False | OK |
| set_power | OK | 0 | False | False | OK |
| set_intdiv | OK | 0 | False | False | OK |
| set_mod_word | OK | 0 | False | False | OK |
| set_concat | OK | 0 | False | False | OK |
| set_compare_word | OK | 0 | False | False | OK |
| set_compare_symbols | OK | 0 | False | False | OK |
| set_contains | OK | 0 | False | False | OK |
| set_does_not_contain | OK | 0 | False | False | OK |
| set_is | OK | 0 | False | False | OK |
| set_negation | OK | 0 | False | False | OK |
| set_imp_eqv_xor | OK | 0 | False | False | OK |
| set_compound_assign | OK | 0 | False | False | OK |
| set_pow_leftassoc | OK | 0 | False | False | OK |
| set_pow_unary | OK | 0 | False | False | OK |
| set_intdiv_mod | OK | 0 | False | False | OK |
| set_concat_eq | OK | 0 | False | False | OK |
| set_not_compare | OK | 0 | False | False | OK |
| set_not_and | OK | 0 | False | False | OK |
| set_and_or | OK | 0 | False | False | OK |
| set_xor_or | OK | 0 | False | False | OK |
| set_imp_eqv | OK | 0 | False | False | OK |
| set_eq_lt_mixed | OK | 0 | False | False | OK |
| set_bang | OK | 0 | False | False | OK |
| set_is_not_word | OK | 0 | False | False | OK |
| set_triple_eq | OK | 0 | False | False | OK |
| set_safe_nav | OK | 0 | False | False | OK |
| set_struct_literal | OK | 0 | False | False | OK |
| set_array_literal | OK | 0 | False | False | OK |
| set_hash_in_string | OK | 0 | False | False | OK |
| output_basic | OK | 0 | False | False | OK |
| output_hash_expr | OK | 0 | False | False | OK |
| component_basic | NO | 1 | True | False | MISMATCH |
| component_extends | OK | 0 | False | False | OK |
| if_elseif_else | OK | 0 | False | False | OK |
| query_tag | ERROR | 1 | True | False | MISMATCH |
| loop_index | OK | 0 | False | False | OK |
| loop_array | OK | 0 | False | False | OK |
| loop_query | OK | 0 | False | False | OK |
| switch_tag | OK | 0 | False | False | OK |
| mail_tag | OK | 0 | False | False | OK |
| savecontent | OK | 0 | False | False | OK |
| include_tag | OK | 0 | False | False | OK |
| param_tag | OK | 0 | False | False | OK |
| comment | OK | 0 | False | False | OK |
| html_mixed | OK | 0 | False | False | OK |
| custom_tag | OK | 0 | False | False | OK |
| script_tag | OK | 0 | False | False | OK |
| cfoutput_query | OK | 0 | False | False | OK |
| cflocation | OK | 0 | False | False | OK |
| script_var | OK | 0 | False | False | OK |
| script_struct | OK | 0 | False | False | OK |
| script_function | OK | 0 | False | False | OK |
| script_if | OK | 0 | False | False | OK |
| script_for | OK | 0 | False | False | OK |
| script_forin | OK | 0 | False | False | OK |
| script_while | OK | 0 | False | False | OK |
| script_dowhile | OK | 0 | False | False | OK |
| script_switch | OK | 0 | False | False | OK |
| script_trycatch | OK | 0 | False | False | OK |
| script_throw | OK | 0 | False | False | OK |
| script_new | OK | 0 | False | False | OK |
| script_arrow | OK | 0 | False | False | OK |
| script_spread | OK | 0 | False | False | OK |
| script_elvis_chain | OK | 0 | False | False | OK |
| script_operator_prec | OK | 0 | False | False | OK |
| script_component_decl | NO | 1 | True | False | MISMATCH |
| script_interface_decl | NO | 1 | True | False | MISMATCH |
| cfs_basic (wrapped for tp) | OK | 0 | False | False | OK |
| cfs_function (wrapped for tp) | OK | 0 | False | False | OK |
| cfs_component (wrapped for tp) | NO | 1 | True | False | MISMATCH |
| cfs_import (wrapped for tp) | OK | 0 | False | False | OK |
| cfs_binary_ops (wrapped for tp) | OK | 0 | False | False | OK |

## Distinct CST node kinds per parser (whole corpus)

| textparser kind | count | | tree-sitter kind | count |
|---|---|---|---|---|
| Variable | 187 | | identifier | 88 |
| AssignOperator | 92 | | tok:<cf | 85 |
| Template | 76 | | tok:> | 82 |
| Repeat | 75 | | ( | 77 |
| Number | 65 | | tok:program | 77 |
| TagEnd | 46 | | tok:= | 72 |
| TagSelfClose | 45 | | tok: | 58 |
| Sequence | 43 | | binary_expression | 53 |
| SetStartTag_Start | 41 | | cf_selfclose_void_tag_end | 48 |
| SetTag | 41 | | number | 46 |
| Semicolon | 34 | | self_closing_tag_delimiter | 46 |
| DoubleString | 30 | | assignment_expression | 41 |
| ExpressionStatement | 26 | | cf_set_tag | 41 |
| ScriptStartTag_Start | 24 | | tok:</cf | 37 |
| TagAttribute | 24 | | cf_attribute | 26 |
| CompareOperator | 21 | | cf_attribute_name | 26 |
| LParen | 21 | | quoted_cf_attribute_value | 26 |
| RParen | 21 | | tok: (attribute_value)  | 24 |
| ScriptEndTag | 21 | | tok:' | 24 |
| ScriptTagPair | 21 | | cf_script_content | 19 |
| Boolean | 20 | | cf_script_tag | 19 |
| LBrace | 18 | | cf_tag_name | 16 |
| RBrace | 18 | | string | 15 |
| SharpExpression_Start | 18 | | cf_tag_attributes | 14 |
| SingleString | 16 | | html_text | 12 |
| PostfixExpressionSuffix | 14 | | string_fragment | 12 |
| StartTag_Start | 14 | | false | 10 |
| Argument | 13 | | hash_single | 10 |
| BlockStatement | 12 | | tok:# | 10 |
| Separator | 12 | | tok:false | 10 |
| StartTag | 10 | | tok:true | 10 |
| AddOperator | 9 | | true | 10 |
| ObjectMember | 9 | | cf_end_tag | 8 |
| SharpExpression | 9 | | cf_start_tag | 8 |
| EndTag | 8 | | cf_tag | 8 |
| ArgumentList | 7 | | tok:, | 7 |
| LogicalAndOperator | 6 | | tok:; | 7 |
| Parameter | 6 | | tag_name | 6 |
| FunctionKeyword | 5 | | not_expression | 5 |
| LogicalNotOperator | 5 | | not_operator | 5 |
| MulOperator | 5 | | tok:( | 5 |
| PowerOperator | 5 | | tok:) | 5 |
| StructMember | 5 | | cf_output_tag | 4 |
| ArrayLiteral | 4 | | cf_selfclose_tag | 4 |
| FunctionDeclaration | 4 | | hash_expression | 4 |
| LBracket | 4 | | member_expression | 4 |
| LogicalOrOperator | 4 | | property_identifier | 4 |
| OutputEndTag | 4 | | tok:* | 4 |
| OutputStartTag_Start | 4 | | tok:+ | 4 |
| OutputTagPair | 4 | | tok:. | 4 |
| RBracket | 4 | | tok:^ | 4 |
| ReturnKeyword | 4 | | tok:not | 4 |
| ReturnStatement | 4 | | tok:{ | 4 |
| SelfClosingTag | 4 | | tok:} | 4 |
| ConcatOperator | 3 | | augmented_assignment_expression | 3 |
| IncDecOperator | 3 | | element | 3 |
| LoopEndTag | 3 | | end_tag | 3 |
| LoopStartTag_Start | 3 | | expression_statement | 3 |
| LoopTagPair | 3 | | object_assignment_pattern | 3 |
| ModOperator | 3 | | parenthesized_expression | 3 |
| ParameterList | 3 | | shorthand_property_identifier_pattern | 3 |
| ParenthesizedExpression | 3 | | start_tag | 3 |
| Statement | 3 | | tok: (identifier)  | 3 |
| Colon | 2 | | tok:< | 3 |
| ComponentKeyword | 2 | | tok:</ | 3 |
| DefaultKeyword | 2 | | array | 2 |
| ElvisOperator | 2 | | cf_component_close_tag | 2 |
| ForKeyword | 2 | | cf_component_open_tag | 2 |
| IntegerDivOperator | 2 | | cf_if_alt | 2 |
| LogicalEqvOperator | 2 | | component_attribute | 2 |
| LogicalImpOperator | 2 | | formal_parameters | 2 |
| LogicalXorOperator | 2 | | function_declaration | 2 |
| NewExpression | 2 | | optional_chain | 2 |
| NewKeyword | 2 | | parameter_type | 2 |
| PropertyKeyword | 2 | | return_statement | 2 |
| SafeNavigation | 2 | | statement_block | 2 |
| StructLiteral | 2 | | tok: (hash_expression  | 2 |
| StructMemberList | 2 | | tok: (string_fragment)  | 2 |
| SwitchCase | 2 | | tok:& | 2 |
| ThisKeyword | 2 | | tok:)  | 2 |
| WhileKeyword | 2 | | tok:- | 2 |
| <no-cst> | 1 | | tok:[ | 2 |
| ArrowFunction | 1 | | tok:\ | 2 |
| BreakKeyword | 1 | | tok:] | 2 |
| BreakStatement | 1 | | tok:function | 2 |
| CaseKeyword | 1 | | tok:return | 2 |
| CatchClause | 1 | | cf_comment | 1 |
| CatchKeyword | 1 | | cf_else_tag | 1 |
| DoKeyword | 1 | | cf_elseif_tag | 1 |
| DoWhileStatement | 1 | | cf_function_tag | 1 |
| ElseIfStartTag_Start | 1 | | cf_if_tag | 1 |
| ElseIfTag | 1 | | cf_query_content | 1 |
| ElseKeyword | 1 | | cf_query_tag | 1 |
| ElseStartTag_Start | 1 | | cf_return_tag | 1 |
| ElseTag | 1 | | cf_savecontent_body | 1 |
| ForInStatement | 1 | | cf_savecontent_tag | 1 |
| IfEndTag | 1 | | component | 1 |
| IfKeyword | 1 | | component_body | 1 |
| IfStartTag_Start | 1 | | elvis_expression | 1 |
| IfStatement | 1 | | import_path | 1 |
| IfTagPair | 1 | | import_statement | 1 |
| ImportKeyword | 1 | | object_pattern | 1 |
| ImportStatement | 1 | | property_declaration | 1 |
| InKeyword | 1 | | ternary_expression | 1 |
| InterfaceKeyword | 1 | | this | 1 |
| LambdaOperator | 1 | | tok: (string_fragment) (hash_expression  | 1 |
| MailEndTag | 1 | | tok:! | 1 |
| MailStartTag_Start | 1 | | tok:% | 1 |
| MailTagPair | 1 | | tok:&& | 1 |
| NullCoalescingOperator | 1 | | tok:&= | 1 |
| QualifiedIdentifier | 1 | | tok:) (string_fragment)  | 1 |
| Question | 1 | | tok:+= | 1 |
| SavecontentEndTag | 1 | | tok:-= | 1 |
| SavecontentStartTag_Start | 1 | | tok:: | 1 |
| SavecontentTagPair | 1 | | tok:<= | 1 |
| SpreadOperator | 1 | | tok:=== | 1 |
| StandardForStatement | 1 | | tok:>= | 1 |
| SwitchKeyword | 1 | | tok:? | 1 |
| SwitchStatement | 1 | | tok:?: | 1 |
| ThrowKeyword | 1 | | tok:?? | 1 |
| ThrowStatement | 1 | | tok:component | 1 |
| TryCatchStatement | 1 | | tok:import | 1 |
| TryKeyword | 1 | | tok:property | 1 |
| VarKeyword | 1 | | tok:this | 1 |
| VariableDeclarationStatement | 1 | | unary_expression | 1 |
| VariableDeclarator | 1 | | unary_operator | 1 |
| VariableDeclaratorList | 1 | |  |  |
| WhileStatement | 1 | |  |  |

Total distinct kinds: textparser=128, tree-sitter=126

## Mismatches: 5
- **component_basic** — tp status `NO` (reject=True), ts reject=False
- **query_tag** — tp status `ERROR` (reject=True), ts reject=False
- **script_component_decl** — tp status `NO` (reject=True), ts reject=False
- **script_interface_decl** — tp status `NO` (reject=True), ts reject=False
- **cfs_component (wrapped for tp)** — tp status `NO` (reject=True), ts reject=False
