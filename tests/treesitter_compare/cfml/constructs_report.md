# Node kinds & tree shape: single-construct comparison

`tp` = textparser CFML CST; `ts` = tree-sitter cfml/cfscript parse tree (anonymous tokens shown quoted).


## set_basic.cfm

```
<cfset a = 1234 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_expr.cfm

```
<cfset x = a + b * (c - d) % e />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            AddOperator
                Variable
                ModOperator
                    MulOperator
                        Variable
                        ParenthesizedExpression
                            LParen
                            AddOperator
                                Variable
                                Variable
                            RParen
                    Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    identifier
                "+"
                    binary_expression
                        binary_expression
                            identifier
                        "*"
                            parenthesized_expression
                            "("
                                binary_expression
                                    identifier
                                "-"
                                    identifier
                            ")"
                    "%"
                        identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_ternary.cfm

```
<cfset t = cond ? x : y />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Question
                Variable
                Variable
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                ternary_expression
                    identifier
                "?"
                    identifier
                ":"
                    identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_elvis.cfm

```
<cfset t = maybe ?: fallback />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            ElvisOperator
                Variable
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                elvis_expression
                    identifier
                "?:"
                    identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_nullcoalesce.cfm

```
<cfset t = maybe ?? fallback />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            NullCoalescingOperator
                Variable
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    identifier
                "??"
                    identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_power.cfm

```
<cfset r = 2 ^ 3 * 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            MulOperator
                PowerOperator
                    Number
                    Number
                Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        number
                    "^"
                        number
                "*"
                    number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_intdiv.cfm

```
<cfset r = 8 \ 3 * 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            IntegerDivOperator
                Number
                MulOperator
                    Number
                    Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    number
                "\"
                    binary_expression
                        number
                    "*"
                        number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_mod_word.cfm

```
<cfset r = 8 MOD 3 * 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            ModOperator
                Number
                MulOperator
                    Number
                    Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    number
                    binary_expression
                        number
                    "*"
                        number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_concat.cfm

```
<cfset s = 'A' & 2 + 3 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            ConcatOperator
                SingleString
                AddOperator
                    Number
                    Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    string
                    "'"
                        string_fragment
                    "'"
                "&"
                    binary_expression
                        number
                    "+"
                        number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_compare_word.cfm

```
<cfset b = 1 EQ '1' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                Number
                SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    number
                    string
                    "'"
                        string_fragment
                    "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_compare_symbols.cfm

```
<cfset b = (a >= 1) && (a <= 10) />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalAndOperator
                ParenthesizedExpression
                    LParen
                    CompareOperator
                        Variable
                        Number
                    RParen
                ParenthesizedExpression
                    LParen
                    CompareOperator
                        Variable
                        Number
                    RParen
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    parenthesized_expression
                    "("
                        binary_expression
                            identifier
                        ">="
                            number
                    ")"
                "&&"
                    parenthesized_expression
                    "("
                        binary_expression
                            identifier
                        "<="
                            number
                    ")"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_contains.cfm

```
<cfset b = 'hello' CONTAINS 'ell' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                SingleString
                SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    string
                    "'"
                        string_fragment
                    "'"
                    string
                    "'"
                        string_fragment
                    "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_does_not_contain.cfm

```
<cfset b = 'hello' DOES NOT CONTAIN 'xyz' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                SingleString
                SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    string
                    "'"
                        string_fragment
                    "'"
                    string
                    "'"
                        string_fragment
                    "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_is.cfm

```
<cfset b = x IS 5 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                Variable
                Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    identifier
                    number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_negation.cfm

```
<cfset b = NOT a AND b OR c />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalOrOperator
                LogicalAndOperator
                    LogicalNotOperator
                        Variable
                    Variable
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        not_expression
                            not_operator
                            "not"
                            identifier
                        identifier
                    identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_imp_eqv_xor.cfm

```
<cfset b = false IMP true EQV false XOR true />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalImpOperator
                Boolean
                LogicalEqvOperator
                    Boolean
                    LogicalXorOperator
                        Boolean
                        Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    false
                    "false"
                    binary_expression
                        true
                        "true"
                        binary_expression
                            false
                            "false"
                            true
                            "true"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_compound_assign.cfm

```
<cfset x += 1 /><cfset y -= 2 /><cfset z &= 'end' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Number
        TagSelfClose
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Number
        TagSelfClose
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            augmented_assignment_expression
                identifier
            "+="
                number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
        cf_set_tag
        "<cf"
            augmented_assignment_expression
                identifier
            "-="
                number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
        cf_set_tag
        "<cf"
            augmented_assignment_expression
                identifier
            "&="
                string
                "'"
                    string_fragment
                "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_pow_leftassoc.cfm

```
<cfset r = 2 ^ 3 ^ 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            PowerOperator
                PowerOperator
                    Number
                    Number
                Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        number
                    "^"
                        number
                "^"
                    number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_pow_unary.cfm

```
<cfset r = -2 ^ 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            PowerOperator
                AddOperator
                    Number
                Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    unary_expression
                        unary_operator
                        "-"
                        number
                "^"
                    number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_intdiv_mod.cfm

```
<cfset r = 8 MOD 5 \ 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            ModOperator
                Number
                IntegerDivOperator
                    Number
                    Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    number
                    binary_expression
                        number
                    "\"
                        number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_concat_eq.cfm

```
<cfset r = 'a' & 'b' EQ 'ab' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                ConcatOperator
                    SingleString
                    SingleString
                SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        string
                        "'"
                            string_fragment
                        "'"
                    "&"
                        string
                        "'"
                            string_fragment
                        "'"
                    string
                    "'"
                        string_fragment
                    "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_not_compare.cfm

```
<cfset r = NOT 0 GT 3 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalNotOperator
                CompareOperator
                    Number
                    Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                not_expression
                    not_operator
                    "not"
                    binary_expression
                        number
                        number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_not_and.cfm

```
<cfset r = not false and false />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalAndOperator
                LogicalNotOperator
                    Boolean
                Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    not_expression
                        not_operator
                        "not"
                        false
                        "false"
                    false
                    "false"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_and_or.cfm

```
<cfset r = true or true and false />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalOrOperator
                Boolean
                LogicalAndOperator
                    Boolean
                    Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    true
                    "true"
                    binary_expression
                        true
                        "true"
                        false
                        "false"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_xor_or.cfm

```
<cfset r = true xor false or true />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalXorOperator
                Boolean
                LogicalOrOperator
                    Boolean
                    Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    true
                    "true"
                    binary_expression
                        false
                        "false"
                        true
                        "true"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_imp_eqv.cfm

```
<cfset r = false imp false eqv false />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalImpOperator
                Boolean
                LogicalEqvOperator
                    Boolean
                    Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    false
                    "false"
                    binary_expression
                        false
                        "false"
                        false
                        "false"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_eq_lt_mixed.cfm

```
<cfset r = 1 EQ 1 LT 2 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                CompareOperator
                    Number
                    Number
                Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        number
                        number
                    number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_bang.cfm

```
<cfset r = !false and true />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            LogicalAndOperator
                LogicalNotOperator
                    Boolean
                Boolean
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    not_expression
                        not_operator
                        "!"
                        false
                        "false"
                    true
                    "true"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_is_not_word.cfm

```
<cfset r = x IS NOT y />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                Variable
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    identifier
                    identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_triple_eq.cfm

```
<cfset r = 1 === '1' />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            CompareOperator
                Number
                SingleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                binary_expression
                    number
                "==="
                    string
                    "'"
                        string_fragment
                    "'"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_safe_nav.cfm

```
<cfset v = st?.a?.b />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            PostfixExpressionSuffix
                PostfixExpressionSuffix
                    Variable
                    SafeNavigation
                    Variable
                SafeNavigation
                Variable
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                member_expression
                    member_expression
                        identifier
                        optional_chain
                        property_identifier
                    optional_chain
                    property_identifier
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_struct_literal.cfm

```
<cfset s = { a = 1, b = 'two', c = [1, 2, 3] } />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            StructLiteral
                LBrace
                StructMemberList
                    StructMember
                        Variable
                        AssignOperator
                        Number
                    Repeat
                        Sequence
                            Separator
                            StructMember
                                Variable
                                AssignOperator
                                SingleString
                        Sequence
                            Separator
                            StructMember
                                Variable
                                AssignOperator
                                ArrayLiteral
                                    LBracket
                                    ArgumentList
                                        Argument
                                            Number
                                        Repeat
                                            Sequence
                                                Separator
                                                Argument
                                                    Number
                                            Sequence
                                                Separator
                                                Argument
                                                    Number
                                    RBracket
                RBrace
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                object_pattern
                "{"
                    object_assignment_pattern
                        shorthand_property_identifier_pattern
                    "="
                        number
                ","
                    object_assignment_pattern
                        shorthand_property_identifier_pattern
                    "="
                        string
                        "'"
                            string_fragment
                        "'"
                ","
                    object_assignment_pattern
                        shorthand_property_identifier_pattern
                    "="
                        array
                        "["
                            number
                        ","
                            number
                        ","
                            number
                        "]"
                "}"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_array_literal.cfm

```
<cfset a = [1, 2, 3] />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            ArrayLiteral
                LBracket
                ArgumentList
                    Argument
                        Number
                    Repeat
                        Sequence
                            Separator
                            Argument
                                Number
                        Sequence
                            Separator
                            Argument
                                Number
                RBracket
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                array
                "["
                    number
                ","
                    number
                ","
                    number
                "]"
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## set_hash_in_string.cfm

```
<cfset s = "hello #name# world" />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            DoubleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                string
                ""
                " (string_fragment) (hash_expression "
                #
                " (identifier) "
                #
                ") (string_fragment) "
                ""
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## output_basic.cfm

```
<cfoutput>Hello #name#</cfoutput>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    OutputTagPair
        OutputStartTag_Start
        TagEnd
        Repeat
            Variable
            SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
        OutputEndTag
```

**tree-sitter CST**

```
    program
        cf_output_tag
        "<cf"
        ">"
            html_text
            hash_expression
                identifier
            "#"
        "</cf"
        ">"
```


## output_hash_expr.cfm

```
<cfoutput>#a + b#</cfoutput>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    OutputTagPair
        OutputStartTag_Start
        TagEnd
        Repeat
            SharpExpression
                SharpExpression_Start
                AddOperator
                    Variable
                    Variable
                SharpExpression_Start
        OutputEndTag
```

**tree-sitter CST**

```
    program
        cf_output_tag
        "<cf"
        ">"
            hash_expression
                binary_expression
                    identifier
                "+"
                    identifier
            "#"
        "</cf"
        ">"
```


## component_basic.cfm

```
<cfcomponent>
  <cffunction name="foo">
    <cfreturn 1 />
  </cffunction>
</cfcomponent>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    StartTag
        StartTag_Start
        TagEnd
    StartTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
    ReturnTag
        ReturnStartTag_Start
        Number
        TagSelfClose
    EndTag
    EndTag
```

**tree-sitter CST**

```
    program
        cf_component_open_tag
        "<cf"
            cf_selfclose_void_tag_end
            ">"
        cf_function_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
        ">"
            cf_return_tag
            "<cf"
                number
                cf_selfclose_void_tag_end
                    self_closing_tag_delimiter
        "</cf"
        ">"
        cf_component_close_tag
        "</cf"
        ">"
```


## component_extends.cfm

```
<cfcomponent extends="Base" hint="the base">
  <cfproperty name="x" type="numeric" />
</cfcomponent>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    StartTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
    SelfClosingTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagSelfClose
    EndTag
```

**tree-sitter CST**

```
    program
        cf_component_open_tag
        "<cf"
            cf_tag_attributes
                cf_attribute
                    cf_attribute_name
                "="
                    quoted_cf_attribute_value
                    ""
                    " (attribute_value) "
                    ""
            cf_tag_attributes
                cf_attribute
                    cf_attribute_name
                "="
                    quoted_cf_attribute_value
                    ""
                    " (attribute_value) "
                    ""
            cf_selfclose_void_tag_end
            ">"
        cf_selfclose_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
        cf_component_close_tag
        "</cf"
        ">"
```


## if_elseif_else.cfm

```
<cfif x GT 0>
  <cfset pos = true />
<cfelseif x LT 0>
  <cfset neg = true />
<cfelse>
  <cfset zero = true />
</cfif>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    IfTagPair
        IfStartTag_Start
        CompareOperator
            Variable
            Number
        TagEnd
        Repeat
            SetTag
                SetStartTag_Start
                AssignOperator
                    Variable
                    Boolean
                TagSelfClose
        Repeat
            Sequence
                ElseIfTag
                    ElseIfStartTag_Start
                    CompareOperator
                        Variable
                        Number
                    TagEnd
                Repeat
                    SetTag
                        SetStartTag_Start
                        AssignOperator
                            Variable
                            Boolean
                        TagSelfClose
        Sequence
            ElseTag
                ElseStartTag_Start
                TagEnd
            Repeat
                SetTag
                    SetStartTag_Start
                    AssignOperator
                        Variable
                        Boolean
                    TagSelfClose
        IfEndTag
```

**tree-sitter CST**

```
    program
        cf_if_tag
        "<cf"
            binary_expression
                identifier
                number
        ">"
            cf_set_tag
            "<cf"
                assignment_expression
                    identifier
                "="
                    true
                    "true"
                cf_selfclose_void_tag_end
                    self_closing_tag_delimiter
            cf_if_alt
            "<cf"
                cf_elseif_tag
                    binary_expression
                        identifier
                        number
                ">"
                cf_set_tag
                "<cf"
                    assignment_expression
                        identifier
                    "="
                        true
                        "true"
                    cf_selfclose_void_tag_end
                        self_closing_tag_delimiter
                cf_if_alt
                "<cf"
                    cf_else_tag
                    ">"
                    cf_set_tag
                    "<cf"
                        assignment_expression
                            identifier
                        "="
                            true
                            "true"
                        cf_selfclose_void_tag_end
                            self_closing_tag_delimiter
        "</cf"
        ">"
```


## query_tag.cfm

```
<cfquery name="q" datasource="ds">SELECT * FROM t WHERE id = <cfqueryparam value="#id#" /></cfquery>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    QueryTagPair
        QueryStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            Variable
            MulOperator
            Variable
            Variable
            Variable
            Variable
            AssignOperator
            SelfClosingTag
                StartTag_Start
                Repeat
                    TagAttribute
                        Variable
                        Sequence
                            AssignOperator
                            DoubleString
                TagSelfClose
        QueryEndTag
```

**tree-sitter CST**

```
    program
        cf_query_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
        ">"
            cf_query_content
        "</cf"
        ">"
```


## loop_index.cfm

```
<cfloop from="1" to="10" index="i">
  <cfoutput>#i#</cfoutput>
</cfloop>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    LoopTagPair
        LoopStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            OutputTagPair
                OutputStartTag_Start
                TagEnd
                Repeat
                    SharpExpression
                        SharpExpression_Start
                        Variable
                        SharpExpression_Start
                OutputEndTag
        LoopEndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
            ">"
            cf_output_tag
            "<cf"
            ">"
                hash_expression
                    identifier
                "#"
            "</cf"
            ">"
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## loop_array.cfm

```
<cfloop array="#arr#" index="i">#i#</cfloop>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    LoopTagPair
        LoopStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
        LoopEndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (hash_expression "
                        #
                        " (identifier) "
                        #
                        ") "
                        ""
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
            ">"
            hash_single
            html_text
            hash_single
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## loop_query.cfm

```
<cfloop query="q">#q.name#</cfloop>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    LoopTagPair
        LoopStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            SharpExpression
                SharpExpression_Start
                PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
                SharpExpression_Start
        LoopEndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
            ">"
            hash_single
            html_text
            hash_single
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## switch_tag.cfm

```
<cfswitch expression="#x#">
  <cfcase value="1">one</cfcase>
  <cfdefaultcase>other</cfdefaultcase>
</cfswitch>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    StartTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
    StartTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
    Variable
    EndTag
    StartTag
        StartTag_Start
        TagEnd
    Variable
    EndTag
    EndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (hash_expression "
                        #
                        " (identifier) "
                        #
                        ") "
                        ""
            ">"
            cf_tag
                cf_start_tag
                "<cf"
                    cf_tag_name
                    cf_tag_attributes
                        cf_attribute
                            cf_attribute_name
                        "="
                            quoted_cf_attribute_value
                            ""
                            " (attribute_value) "
                            ""
                ">"
                html_text
                cf_end_tag
                "</cf"
                    cf_tag_name
                ">"
            cf_tag
                cf_start_tag
                "<cf"
                    cf_tag_name
                ">"
                html_text
                cf_end_tag
                "</cf"
                    cf_tag_name
                ">"
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## mail_tag.cfm

```
<cfmail to="a@b.c" from="d@e.f" subject="hi">body #x#</cfmail>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    MailTagPair
        MailStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            Variable
            SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
        MailEndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
            ">"
            html_text
            hash_single
            html_text
            hash_single
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## savecontent.cfm

```
<cfsavecontent variable="buf">#a# #b#</cfsavecontent>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SavecontentTagPair
        SavecontentStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
            SharpExpression
                SharpExpression_Start
                Variable
                SharpExpression_Start
        SavecontentEndTag
```

**tree-sitter CST**

```
    program
        cf_savecontent_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
        ">"
            cf_savecontent_body
                hash_single
                html_text
                hash_single
                html_text
                hash_single
                html_text
                hash_single
        "</cf"
        ">"
```


## include_tag.cfm

```
<cfinclude template="page.cfm" />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SelfClosingTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_selfclose_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## param_tag.cfm

```
<cfparam name="x" default="1" />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SelfClosingTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                DefaultKeyword
                Sequence
                    AssignOperator
                    DoubleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_selfclose_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## comment.cfm

```
<!--- a comment --->
<cfset x = 1 />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Number
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_comment
        cf_set_tag
        "<cf"
            assignment_expression
                identifier
            "="
                number
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## html_mixed.cfm

```
<html><body><p>Hello</p><cfset y = 2 /></body></html>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    StartTag
        StartTag_Start
        TagEnd
    StartTag
        StartTag_Start
        TagEnd
    StartTag
        StartTag_Start
        TagEnd
    Variable
    EndTag
    SetTag
        SetStartTag_Start
        AssignOperator
            Variable
            Number
        TagSelfClose
    EndTag
    EndTag
```

**tree-sitter CST**

```
    program
        element
            start_tag
            "<"
                tag_name
            ">"
            element
                start_tag
                "<"
                    tag_name
                ">"
                element
                    start_tag
                    "<"
                        tag_name
                    ">"
                    html_text
                    end_tag
                    "</"
                        tag_name
                    ">"
                cf_set_tag
                "<cf"
                    assignment_expression
                        identifier
                    "="
                        number
                    cf_selfclose_void_tag_end
                        self_closing_tag_delimiter
                end_tag
                "</"
                    tag_name
                ">"
            end_tag
            "</"
                tag_name
            ">"
```


## custom_tag.cfm

```
<cf_myTag attr="1" >body</cf_myTag>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    StartTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
    Variable
    EndTag
```

**tree-sitter CST**

```
    program
        cf_tag
            cf_start_tag
            "<cf"
                cf_tag_name
                cf_tag_attributes
                    cf_attribute
                        cf_attribute_name
                    "="
                        quoted_cf_attribute_value
                        ""
                        " (attribute_value) "
                        ""
            ">"
            html_text
            cf_end_tag
            "</cf"
                cf_tag_name
            ">"
```


## script_tag.cfm

```
<cfscript>
  x = 1;
  y = 2;
</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    Number
                Semicolon
            ExpressionStatement
                AssignOperator
                    Variable
                    Number
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## cfoutput_query.cfm

```
<cfoutput query="q">#q.name#</cfoutput>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    OutputTagPair
        OutputStartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagEnd
        Repeat
            SharpExpression
                SharpExpression_Start
                PostfixExpressionSuffix
                    Variable
                    ObjectMember
                    Variable
                SharpExpression_Start
        OutputEndTag
```

**tree-sitter CST**

```
    program
        cf_output_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
        ">"
            hash_expression
                member_expression
                    identifier
                "."
                    property_identifier
            "#"
        "</cf"
        ">"
```


## cflocation.cfm

```
<cflocation url="index.cfm" addtoken="false" />
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    SelfClosingTag
        StartTag_Start
        Repeat
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
            TagAttribute
                Variable
                Sequence
                    AssignOperator
                    DoubleString
        TagSelfClose
```

**tree-sitter CST**

```
    program
        cf_selfclose_tag
        "<cf"
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_attribute
                cf_attribute_name
            "="
                quoted_cf_attribute_value
                ""
                " (attribute_value) "
                ""
            cf_selfclose_void_tag_end
                self_closing_tag_delimiter
```


## script_var.cfm

```
<cfscript>var x = 1;</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            VariableDeclarationStatement
                VarKeyword
                VariableDeclaratorList
                    VariableDeclarator
                        Variable
                        Sequence
                            AssignOperator
                            Number
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_struct.cfm

```
<cfscript>s = { a = 1, b = [1,2] };</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    StructLiteral
                        LBrace
                        StructMemberList
                            StructMember
                                Variable
                                AssignOperator
                                Number
                            Repeat
                                Sequence
                                    Separator
                                    StructMember
                                        Variable
                                        AssignOperator
                                        ArrayLiteral
                                            LBracket
                                            ArgumentList
                                                Argument
                                                    Number
                                                Repeat
                                                    Sequence
                                                        Separator
                                                        Argument
                                                            Number
                                            RBracket
                        RBrace
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_function.cfm

```
<cfscript>function add(a, b) { return a + b; }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            FunctionDeclaration
                FunctionKeyword
                Variable
                LParen
                ParameterList
                    Parameter
                        Variable
                    Repeat
                        Sequence
                            Separator
                            Parameter
                                Variable
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ReturnStatement
                            ReturnKeyword
                            AddOperator
                                Variable
                                Variable
                            Semicolon
                    RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_if.cfm

```
<cfscript>if (a > 1) { b = 2; } else { b = 3; }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            IfStatement
                IfKeyword
                LParen
                CompareOperator
                    Variable
                    Number
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            AssignOperator
                                Variable
                                Number
                            Semicolon
                    RBrace
                Sequence
                    ElseKeyword
                    BlockStatement
                        LBrace
                        Repeat
                            ExpressionStatement
                                AssignOperator
                                    Variable
                                    Number
                                Semicolon
                        RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_for.cfm

```
<cfscript>for (i = 1; i <= 10; i++) { sum += i; }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            StandardForStatement
                ForKeyword
                LParen
                AssignOperator
                    Variable
                    Number
                Semicolon
                CompareOperator
                    Variable
                    Number
                Semicolon
                IncDecOperator
                    Variable
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            AssignOperator
                                Variable
                                Variable
                            Semicolon
                    RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_forin.cfm

```
<cfscript>for (k in s) { writeOutput(k); }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ForInStatement
                ForKeyword
                LParen
                Variable
                InKeyword
                Variable
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            PostfixExpressionSuffix
                                Variable
                                LParen
                                ArgumentList
                                    Argument
                                        Variable
                                RParen
                            Semicolon
                    RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_while.cfm

```
<cfscript>while (x < 10) { x++; }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            WhileStatement
                WhileKeyword
                LParen
                CompareOperator
                    Variable
                    Number
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            IncDecOperator
                                Variable
                            Semicolon
                    RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_dowhile.cfm

```
<cfscript>do { x++; } while (x < 10);</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            DoWhileStatement
                DoKeyword
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            IncDecOperator
                                Variable
                            Semicolon
                    RBrace
                WhileKeyword
                LParen
                CompareOperator
                    Variable
                    Number
                RParen
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_switch.cfm

```
<cfscript>switch (x) { case 1: break; default: x = 0; }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            SwitchStatement
                SwitchKeyword
                LParen
                Variable
                RParen
                LBrace
                Repeat
                    SwitchCase
                        CaseKeyword
                        Number
                        Colon
                        Repeat
                            BreakStatement
                                BreakKeyword
                                Semicolon
                    SwitchCase
                        DefaultKeyword
                        Colon
                        Repeat
                            ExpressionStatement
                                AssignOperator
                                    Variable
                                    Number
                                Semicolon
                RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_trycatch.cfm

```
<cfscript>try { risky(); } catch (any e) { log(e.message); }</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            TryCatchStatement
                TryKeyword
                BlockStatement
                    LBrace
                    Repeat
                        ExpressionStatement
                            PostfixExpressionSuffix
                                Variable
                                LParen
                                RParen
                            Semicolon
                    RBrace
                Repeat
                    CatchClause
                        CatchKeyword
                        LParen
                        Variable
                        Variable
                        RParen
                        BlockStatement
                            LBrace
                            Repeat
                                ExpressionStatement
                                    PostfixExpressionSuffix
                                        Variable
                                        LParen
                                        ArgumentList
                                            Argument
                                                PostfixExpressionSuffix
                                                    Variable
                                                    ObjectMember
                                                    Variable
                                        RParen
                                    Semicolon
                            RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_throw.cfm

```
<cfscript>throw new Exception('boom');</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ThrowStatement
                ThrowKeyword
                NewExpression
                    NewKeyword
                    PostfixExpressionSuffix
                        Variable
                        LParen
                        ArgumentList
                            Argument
                                SingleString
                        RParen
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_new.cfm

```
<cfscript>obj = new com.example.Foo();</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    NewExpression
                        NewKeyword
                        PostfixExpressionSuffix
                            PostfixExpressionSuffix
                                PostfixExpressionSuffix
                                    Variable
                                    ObjectMember
                                    Variable
                                ObjectMember
                                Variable
                            LParen
                            RParen
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_arrow.cfm

```
<cfscript>f = (a, b) => a + b;</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    ArrowFunction
                        Sequence
                            LParen
                            ParameterList
                                Parameter
                                    Variable
                                Repeat
                                    Sequence
                                        Separator
                                        Parameter
                                            Variable
                            RParen
                        LambdaOperator
                        AddOperator
                            Variable
                            Variable
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_spread.cfm

```
<cfscript>x = [1, ...more];</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    ArrayLiteral
                        LBracket
                        ArgumentList
                            Argument
                                Number
                            Repeat
                                Sequence
                                    Separator
                                    Argument
                                        SpreadOperator
                                        Variable
                        RBracket
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_elvis_chain.cfm

```
<cfscript>v = data.maybe ?: 'fallback';</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    ElvisOperator
                        PostfixExpressionSuffix
                            Variable
                            ObjectMember
                            Variable
                        SingleString
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_operator_prec.cfm

```
<cfscript>r = 2 ^ 3 * 2 + 1 & 'x' EQ '9x';</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    CompareOperator
                        ConcatOperator
                            AddOperator
                                MulOperator
                                    PowerOperator
                                        Number
                                        Number
                                    Number
                                Number
                            SingleString
                        SingleString
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_component_decl.cfm

```
<cfscript>
component extends="Base" {
  // member comment
  property name="p" type="string";
  function init() { return this; }
}
</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ComponentDeclaration
                ComponentKeyword
                Repeat
                    TagAttribute
                        Variable
                        Sequence
                            AssignOperator
                            DoubleString
                LBrace
                Repeat
                    PropertyDeclaration
                        PropertyKeyword
                        Repeat
                            TagAttribute
                                Variable
                                Sequence
                                    AssignOperator
                                    DoubleString
                            TagAttribute
                                Variable
                                Sequence
                                    AssignOperator
                                    DoubleString
                        Semicolon
                    FunctionDeclaration
                        FunctionKeyword
                        Variable
                        LParen
                        RParen
                        BlockStatement
                            LBrace
                            Repeat
                                ReturnStatement
                                    ReturnKeyword
                                    ThisKeyword
                                    Semicolon
                            RBrace
                RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## script_interface_decl.cfm

```
<cfscript>
interface Marker {
  function doIt();
}
</cfscript>
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            InterfaceDeclaration
                InterfaceKeyword
                Repeat
                    TagAttribute
                        Variable
                LBrace
                Repeat
                    InterfaceMethodDeclaration
                        FunctionKeyword
                        Variable
                        LParen
                        RParen
                        Semicolon
                RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        cf_script_tag
        "<cf"
        ">"
            cf_script_content
        "</cf"
        ">"
```


## cfs_basic.cfs

```
x = 1;
y = 2;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    Number
                Semicolon
            ExpressionStatement
                AssignOperator
                    Variable
                    Number
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        expression_statement
            assignment_expression
                identifier
            "="
                number
        ";"
        expression_statement
            assignment_expression
                identifier
            "="
                number
        ";"
```


## cfs_function.cfs

```
function add(a, b) {
  return a + b;
}
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            FunctionDeclaration
                FunctionKeyword
                Variable
                LParen
                ParameterList
                    Parameter
                        Variable
                    Repeat
                        Sequence
                            Separator
                            Parameter
                                Variable
                RParen
                BlockStatement
                    LBrace
                    Repeat
                        ReturnStatement
                            ReturnKeyword
                            AddOperator
                                Variable
                                Variable
                            Semicolon
                    RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        function_declaration
        "function"
            identifier
            formal_parameters
            "("
                parameter_type
                    identifier
            ","
                parameter_type
                    identifier
            ")"
            statement_block
            "{"
                return_statement
                "return"
                    binary_expression
                        identifier
                    "+"
                        identifier
                ";"
            "}"
```


## cfs_component.cfs

```
component {
  property name="a" type="numeric";
  function getA() { return this.a; }
}
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ComponentDeclaration
                ComponentKeyword
                LBrace
                Repeat
                    PropertyDeclaration
                        PropertyKeyword
                        Repeat
                            TagAttribute
                                Variable
                                Sequence
                                    AssignOperator
                                    DoubleString
                            TagAttribute
                                Variable
                                Sequence
                                    AssignOperator
                                    DoubleString
                        Semicolon
                    FunctionDeclaration
                        FunctionKeyword
                        Variable
                        LParen
                        RParen
                        BlockStatement
                            LBrace
                            Repeat
                                ReturnStatement
                                    ReturnKeyword
                                    PostfixExpressionSuffix
                                        ThisKeyword
                                        ObjectMember
                                        Variable
                                    Semicolon
                            RBrace
                RBrace
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        component
        "component"
            component_body
            "{"
                property_declaration
                "property"
                    component_attribute
                        identifier
                    "="
                        string
                        ""
                        " (string_fragment) "
                        ""
                    component_attribute
                        identifier
                    "="
                        string
                        ""
                        " (string_fragment) "
                        ""
                ";"
                function_declaration
                "function"
                    identifier
                    formal_parameters
                    "("
                    ")"
                    statement_block
                    "{"
                        return_statement
                        "return"
                            member_expression
                                this
                                "this"
                            "."
                                property_identifier
                        ";"
                    "}"
            "}"
```


## cfs_import.cfs

```
import com.foo.Bar;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ImportStatement
                ImportKeyword
                QualifiedIdentifier
                    Variable
                    Repeat
                        Sequence
                            ObjectMember
                            Variable
                        Sequence
                            ObjectMember
                            Variable
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        import_statement
        "import"
            import_path
                identifier
            "."
                identifier
            "."
                identifier
        ";"
```


## cfs_binary_ops.cfs

```
r = a EQ b AND c GT d OR NOT e;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
Template
    ScriptTagPair
        ScriptStartTag_Start
        TagEnd
        Repeat
            ExpressionStatement
                AssignOperator
                    Variable
                    LogicalOrOperator
                        LogicalAndOperator
                            CompareOperator
                                Variable
                                Variable
                            CompareOperator
                                Variable
                                Variable
                        LogicalNotOperator
                            Variable
                Semicolon
        ScriptEndTag
```

**tree-sitter CST**

```
    program
        expression_statement
            assignment_expression
                identifier
            "="
                binary_expression
                    binary_expression
                        binary_expression
                            identifier
                            identifier
                        binary_expression
                            identifier
                            identifier
                    not_expression
                        not_operator
                        "not"
                        identifier
        ";"
```

