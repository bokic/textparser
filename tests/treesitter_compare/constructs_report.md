# Node kinds & tree shape: single-construct comparison

`tp` = textparser grammar CST (from `definitions/typescript_definition.json`); `ts` = tree-sitter-typescript parse tree (anonymous tokens shown quoted).


## var_stmt.ts

```
const answer: number = 42;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    TypeAnnotation
                        Colon
                        TypeContext
                            ConditionalType
                                UnionType
                                    IntersectionType
                                        PostfixType
                                            NumberKeyword
                    Sequence
                        Assign
                        NumericLiteral
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
                type_annotation
                ":"
                    predefined_type
                    "number"
            "="
                number
        ";"
```


## binary_expr.ts

```
const r = a + b * (c - d) % e;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        Plus
                            Identifier
                            Remainder
                                Multiply
                                    Identifier
                                    BasePrimaryExpression
                                        LParen
                                        Minus
                                            Identifier
                                            Identifier
                                        RParen
                                Identifier
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
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
        ";"
```


## ternary.ts

```
const t = cond ? a : b;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        Question
                            Identifier
                            Identifier
                            Identifier
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                ternary_expression
                    identifier
                "?"
                    identifier
                ":"
                    identifier
        ";"
```


## fn_decl.ts

```
function add(a: number, b?: string, ...r: boolean[]): number { return a + b; }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        FunctionDeclaration
            FunctionKeyword
            Identifier
            BindingParameterList
                LParen
                BindingParameterListElements
                    BindingParameter
                        Identifier
                        TypeAnnotation
                            Colon
                            TypeContext
                                ConditionalType
                                    UnionType
                                        IntersectionType
                                            PostfixType
                                                NumberKeyword
                    Sequence
                        Comma
                        BindingParameterListElements
                            BindingParameter
                                Identifier
                                Question
                                TypeAnnotation
                                    Colon
                                    TypeContext
                                        ConditionalType
                                            UnionType
                                                IntersectionType
                                                    PostfixType
                                                        StringKeyword
                            Sequence
                                Comma
                                RestBindingParameter
                                    Ellipsis
                                    Identifier
                                    TypeAnnotation
                                        Colon
                                        TypeContext
                                            ConditionalType
                                                UnionType
                                                    IntersectionType
                                                        PostfixType
                                                            BooleanKeyword
                                                            Repeat
                                                                TypeSuffix
                                                                    LBracket
                                                                    RBracket
                RParen
            TypeAnnotation
                Colon
                TypeContext
                    ConditionalType
                        UnionType
                            IntersectionType
                                PostfixType
                                    NumberKeyword
            BlockStatement
                LBrace
                StatementList
                    ReturnStatement
                        ReturnKeyword
                        Sequence
                            Plus
                                Identifier
                                Identifier
                        Semicolon
                RBrace
```

**tree-sitter CST**

```
    program
        function_declaration
        "function"
            identifier
            formal_parameters
            "("
                required_parameter
                    identifier
                    type_annotation
                    ":"
                        predefined_type
                        "number"
            ","
                optional_parameter
                    identifier
                "?"
                    type_annotation
                    ":"
                        predefined_type
                        "string"
            ","
                required_parameter
                    rest_pattern
                    "..."
                        identifier
                    type_annotation
                    ":"
                        array_type
                            predefined_type
                            "boolean"
                        "["
                        "]"
            ")"
            type_annotation
            ":"
                predefined_type
                "number"
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


## arrow.ts

```
const f = (x: number): number => x * 2;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        ParenthesizedArrowFunction
                            BindingParameterList
                                LParen
                                BindingParameterListElements
                                    BindingParameter
                                        Identifier
                                        TypeAnnotation
                                            Colon
                                            TypeContext
                                                ConditionalType
                                                    UnionType
                                                        IntersectionType
                                                            PostfixType
                                                                NumberKeyword
                                RParen
                            TypeAnnotation
                                Colon
                                TypeContext
                                    ConditionalType
                                        UnionType
                                            IntersectionType
                                                PostfixType
                                                    NumberKeyword
                            ArrowBody
                                Arrow
                                Multiply
                                    Identifier
                                    NumericLiteral
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                arrow_function
                    formal_parameters
                    "("
                        required_parameter
                            identifier
                            type_annotation
                            ":"
                                predefined_type
                                "number"
                    ")"
                    type_annotation
                    ":"
                        predefined_type
                        "number"
                "=>"
                    binary_expression
                        identifier
                    "*"
                        number
        ";"
```


## async_arrow.ts

```
const g = async (a, b) => a + b;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        AsyncArrowFunction
                            AsyncKeyword
                            BindingParameterList
                                LParen
                                BindingParameterListElements
                                    BindingParameter
                                        Identifier
                                    Sequence
                                        Comma
                                        BindingParameterListElements
                                            BindingParameter
                                                Identifier
                                RParen
                            ArrowBody
                                Arrow
                                Plus
                                    Identifier
                                    Identifier
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                arrow_function
                "async"
                    formal_parameters
                    "("
                        required_parameter
                            identifier
                    ","
                        required_parameter
                            identifier
                    ")"
                "=>"
                    binary_expression
                        identifier
                    "+"
                        identifier
        ";"
```


## class_decl.ts

```
class C extends B implements I { private readonly x: number = 1; m(): void {} }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        ClassDeclaration
            ClassKeyword
            Identifier
            Sequence
                ExtendsKeyword
                TypeContext
                    ConditionalType
                        UnionType
                            IntersectionType
                                PostfixType
                                    TypeReference
                                        QualifiedName
                                            Identifier
            Sequence
                ImplementsKeyword
                TypeList
                    ConditionalType
                        UnionType
                            IntersectionType
                                PostfixType
                                    TypeReference
                                        QualifiedName
                                            Identifier
            ClassBody
                LBrace
                Repeat
                    ClassElement
                        PropertyDeclaration
                            Repeat
                                PrivateKeyword
                                ReadonlyKeyword
                            Identifier
                            TypeAnnotation
                                Colon
                                TypeContext
                                    ConditionalType
                                        UnionType
                                            IntersectionType
                                                PostfixType
                                                    NumberKeyword
                            Sequence
                                Assign
                                NumericLiteral
                            Semicolon
                    ClassElement
                        MethodDeclaration
                            Identifier
                            BindingParameterList
                                LParen
                                RParen
                            TypeAnnotation
                                Colon
                                TypeContext
                                    ConditionalType
                                        UnionType
                                            IntersectionType
                                                PostfixType
                                                    VoidKeyword
                            BlockStatement
                                LBrace
                                RBrace
                RBrace
```

**tree-sitter CST**

```
    program
        class_declaration
        "class"
            type_identifier
            class_heritage
                extends_clause
                "extends"
                    identifier
                implements_clause
                "implements"
                    type_identifier
            class_body
            "{"
                public_field_definition
                    accessibility_modifier
                    "private"
                "readonly"
                    property_identifier
                    type_annotation
                    ":"
                        predefined_type
                        "number"
                "="
                    number
            ";"
                method_definition
                    property_identifier
                    formal_parameters
                    "("
                    ")"
                    type_annotation
                    ":"
                        predefined_type
                        "void"
                    statement_block
                    "{"
                    "}"
            "}"
```


## accessor.ts

```
class C { override readonly accessor auto = 2; }
```

- textparser status: `OK`
- tree-sitter root error: `True`

**textparser CST**

```
SourceFile
    Repeat
        ClassDeclaration
            ClassKeyword
            Identifier
            ClassBody
                LBrace
                Repeat
                    ClassElement
                        PropertyDeclaration
                            Repeat
                                OverrideKeyword
                                ReadonlyKeyword
                                AccessorKeyword
                            Identifier
                            Sequence
                                Assign
                                NumericLiteral
                            Semicolon
                RBrace
```

**tree-sitter CST**

```
    program
        class_declaration
        "class"
            type_identifier
            class_body
            "{"
                public_field_definition
                    override_modifier
                    "override"
                "readonly"
                    property_identifier
                    ERROR
                        identifier
                "="
                    number
            ";"
            "}"
```


## interface.ts

```
interface P { x: number; m(a: string): void; }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        InterfaceDeclaration
            InterfaceKeyword
            Identifier
            ObjectType
                LBrace
                Repeat
                    PropertySignature
                        Identifier
                        Sequence
                            Colon
                            ConditionalType
                                UnionType
                                    IntersectionType
                                        PostfixType
                                            NumberKeyword
                        Semicolon
                    MethodSignature
                        Identifier
                        ParameterList
                            LParen
                            Sequence
                                TypeParameterDeclaration
                                    Identifier
                                    Colon
                                    ConditionalType
                                        UnionType
                                            IntersectionType
                                                PostfixType
                                                    StringKeyword
                            RParen
                        Sequence
                            Colon
                            ConditionalType
                                UnionType
                                    IntersectionType
                                        PostfixType
                                            VoidKeyword
                        Semicolon
                RBrace
```

**tree-sitter CST**

```
    program
        interface_declaration
        "interface"
            type_identifier
            interface_body
            "{"
                property_signature
                    property_identifier
                    type_annotation
                    ":"
                        predefined_type
                        "number"
            ";"
                method_signature
                    property_identifier
                    formal_parameters
                    "("
                        required_parameter
                            identifier
                            type_annotation
                            ":"
                                predefined_type
                                "string"
                    ")"
                    type_annotation
                    ":"
                        predefined_type
                        "void"
            ";"
            "}"
```


## type_alias.ts

```
type T = { a: number } | string[];
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        TypeAliasDeclaration
            TypeKeyword
            Identifier
            Assign
            TypeContext
                ConditionalType
                    UnionType
                        IntersectionType
                            PostfixType
                                ObjectType
                                    LBrace
                                    Repeat
                                        PropertySignature
                                            Identifier
                                            Sequence
                                                Colon
                                                ConditionalType
                                                    UnionType
                                                        IntersectionType
                                                            PostfixType
                                                                NumberKeyword
                                    RBrace
                        Repeat
                            Sequence
                                BitOr
                                IntersectionType
                                    PostfixType
                                        StringKeyword
                                        Repeat
                                            TypeSuffix
                                                LBracket
                                                RBracket
            Semicolon
```

**tree-sitter CST**

```
    program
        type_alias_declaration
        "type"
            type_identifier
        "="
            union_type
                object_type
                "{"
                    property_signature
                        property_identifier
                        type_annotation
                        ":"
                            predefined_type
                            "number"
                "}"
            "|"
                array_type
                    predefined_type
                    "string"
                "["
                "]"
        ";"
```


## generic_type.ts

```
type U<T> = T extends string ? { v: T } : never;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        TypeAliasDeclaration
            TypeKeyword
            Identifier
            TypeParametersContext
                TypeParameters
                    LessThan
                    TypeParameter
                        Identifier
                    GreaterThan
            Assign
            TypeContext
                ConditionalType
                    UnionType
                        IntersectionType
                            PostfixType
                                TypeReference
                                    QualifiedName
                                        Identifier
                    Sequence
                        ExtendsKeyword
                        ConditionalType
                            UnionType
                                IntersectionType
                                    PostfixType
                                        StringKeyword
                        Question
                        ConditionalType
                            UnionType
                                IntersectionType
                                    PostfixType
                                        ObjectType
                                            LBrace
                                            Repeat
                                                PropertySignature
                                                    Identifier
                                                    Sequence
                                                        Colon
                                                        ConditionalType
                                                            UnionType
                                                                IntersectionType
                                                                    PostfixType
                                                                        TypeReference
                                                                            QualifiedName
                                                                                Identifier
                                            RBrace
                        Colon
                        ConditionalType
                            UnionType
                                IntersectionType
                                    PostfixType
                                        NeverKeyword
            Semicolon
```

**tree-sitter CST**

```
    program
        type_alias_declaration
        "type"
            type_identifier
            type_parameters
            "<"
                type_parameter
                    type_identifier
            ">"
        "="
            conditional_type
                type_identifier
            "extends"
                predefined_type
                "string"
            "?"
                object_type
                "{"
                    property_signature
                        property_identifier
                        type_annotation
                        ":"
                            type_identifier
                "}"
            ":"
                predefined_type
                "never"
        ";"
```


## if_stmt.ts

```
function f(a: number) { if (a > 0) return 1; else { a--; } }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        FunctionDeclaration
            FunctionKeyword
            Identifier
            BindingParameterList
                LParen
                BindingParameterListElements
                    BindingParameter
                        Identifier
                        TypeAnnotation
                            Colon
                            TypeContext
                                ConditionalType
                                    UnionType
                                        IntersectionType
                                            PostfixType
                                                NumberKeyword
                RParen
            BlockStatement
                LBrace
                StatementList
                    IfStatement
                        IfKeyword
                        LParen
                        GreaterThan
                            Identifier
                            NumericLiteral
                        RParen
                        ReturnStatement
                            ReturnKeyword
                            Sequence
                                NumericLiteral
                            Semicolon
                        Sequence
                            ElseKeyword
                            BlockStatement
                                LBrace
                                StatementList
                                    ExpressionStatement
                                        Decrement
                                            Identifier
                                        Semicolon
                                RBrace
                RBrace
```

**tree-sitter CST**

```
    program
        function_declaration
        "function"
            identifier
            formal_parameters
            "("
                required_parameter
                    identifier
                    type_annotation
                    ":"
                        predefined_type
                        "number"
            ")"
            statement_block
            "{"
                if_statement
                "if"
                    parenthesized_expression
                    "("
                        binary_expression
                            identifier
                        ">"
                            number
                    ")"
                    return_statement
                    "return"
                        number
                    ";"
                    else_clause
                    "else"
                        statement_block
                        "{"
                            expression_statement
                                update_expression
                                    identifier
                                "--"
                            ";"
                        "}"
            "}"
```


## for_stmt.ts

```
for (let i = 0; i < 10; i++) { use(i); }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        ForStatement
            ForKeyword
            LParen
            VariableDeclarationList
                LetKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        NumericLiteral
            Semicolon
            LessThan
                Identifier
                NumericLiteral
            Semicolon
            Increment
                Identifier
            RParen
            BlockStatement
                LBrace
                StatementList
                    ExpressionStatement
                        Arguments
                            Identifier
                            LParen
                            Sequence
                                Argument
                                    Identifier
                            RParen
                        Semicolon
                RBrace
```

**tree-sitter CST**

```
    program
        for_statement
        "for"
        "("
            lexical_declaration
            "let"
                variable_declarator
                    identifier
                "="
                    number
            ";"
            binary_expression
                identifier
            "<"
                number
        ";"
            update_expression
                identifier
            "++"
        ")"
            statement_block
            "{"
                expression_statement
                    call_expression
                        identifier
                        arguments
                        "("
                            identifier
                        ")"
                ";"
            "}"
```


## object_lit.ts

```
const o = { a: 1, b, "c": 2, [k]: 3, ...spread };
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        ObjectLiteralBody
                            LBrace
                            Sequence
                                ObjectPropertyAssignment
                                    Identifier
                                    Colon
                                    NumericLiteral
                                Repeat
                                    Sequence
                                        Comma
                                        Identifier
                                    Sequence
                                        Comma
                                        ObjectPropertyAssignment
                                            StringLiteral
                                            Colon
                                            NumericLiteral
                                    Sequence
                                        Comma
                                        ObjectPropertyAssignment
                                            ObjectPropertyName
                                                LBracket
                                                Identifier
                                                RBracket
                                            Colon
                                            NumericLiteral
                                    Sequence
                                        Comma
                                        ObjectSpreadAssignment
                                            Ellipsis
                                            Identifier
                            RBrace
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                object
                "{"
                    pair
                        property_identifier
                    ":"
                        number
                ","
                    shorthand_property_identifier
                ","
                    pair
                        string
                        ""
                        " (string_fragment) "
                        ""
                    ":"
                        number
                ","
                    pair
                        computed_property_name
                        "["
                            identifier
                        "]"
                    ":"
                        number
                ","
                    spread_element
                    "..."
                        identifier
                "}"
        ";"
```


## array_lit.ts

```
const arr = [1, 2, , 3, ...[4]];
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        ArrayLiteralExpression
                            LBracket
                            Sequence
                                ArrayElement
                                    NumericLiteral
                                Repeat
                                    Sequence
                                        Comma
                                        ArrayElement
                                            NumericLiteral
                                    Sequence
                                        Comma
                                    Sequence
                                        Comma
                                        ArrayElement
                                            NumericLiteral
                                    Sequence
                                        Comma
                                        ArrayElement
                                            Ellipsis
                                            ArrayLiteralExpression
                                                LBracket
                                                Sequence
                                                    ArrayElement
                                                        NumericLiteral
                                                RBracket
                            RBracket
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                array
                "["
                    number
                ","
                    number
                ","
                ","
                    number
                ","
                    spread_element
                    "..."
                        array
                        "["
                            number
                        "]"
                "]"
        ";"
```


## destructure.ts

```
const { x, y: renamed = 5, ...rest } = source;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    ObjectBindingPattern
                        LBrace
                        ObjectBindingElements
                            ObjectBindingProperty
                                Identifier
                            Sequence
                                Comma
                                ObjectBindingElements
                                    ObjectBindingProperty
                                        Identifier
                                        Colon
                                        BindingElement
                                            Identifier
                                            Sequence
                                                Assign
                                                NumericLiteral
                                    Sequence
                                        Comma
                                        ObjectBindingRestElement
                                            Ellipsis
                                            Identifier
                        RBrace
                    Assign
                    Identifier
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                object_pattern
                "{"
                    shorthand_property_identifier_pattern
                ","
                    pair_pattern
                        property_identifier
                    ":"
                        assignment_pattern
                            identifier
                        "="
                            number
                ","
                    rest_pattern
                    "..."
                        identifier
                "}"
            "="
                identifier
        ";"
```


## import_stmt.ts

```
import { x as y, type T } from "./m";
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        ImportDeclaration
            ImportKeyword
            NamedImports
                LBrace
                Sequence
                    ImportSpecifier
                        Identifier
                        Sequence
                            AsKeyword
                            Identifier
                    Repeat
                        Sequence
                            Comma
                            ImportSpecifier
                                TypeKeyword
                                Identifier
                RBrace
            FromKeyword
            StringLiteral
            Semicolon
```

**tree-sitter CST**

```
    program
        import_statement
        "import"
            import_clause
                named_imports
                "{"
                    import_specifier
                        identifier
                    "as"
                        identifier
                ","
                    import_specifier
                    "type"
                        identifier
                "}"
        "from"
            string
            ""
            " (string_fragment) "
            ""
        ";"
```


## export_stmt.ts

```
export * from "./m";
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        ExportAllDeclaration
            ExportKeyword
            Multiply
            FromKeyword
            StringLiteral
            Semicolon
```

**tree-sitter CST**

```
    program
        export_statement
        "export"
        "*"
        "from"
            string
            ""
            " (string_fragment) "
            ""
        ";"
```


## import_defer.ts

```
import defer * as deferred from "./d";
```

- textparser status: `OK`
- tree-sitter root error: `True`

**textparser CST**

```
SourceFile
    Repeat
        ImportDeclaration
            ImportKeyword
            DeferKeyword
            NamespaceImport
                Multiply
                AsKeyword
                Identifier
            FromKeyword
            StringLiteral
            Semicolon
```

**tree-sitter CST**

```
    program
        import_statement
        "import"
            ERROR
                identifier
            import_clause
                namespace_import
                "*"
                "as"
                    identifier
        "from"
            string
            ""
            " (string_fragment) "
            ""
        ";"
```


## enum_decl.ts

```
enum Color { Red, Green = 2, Blue = "b" }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        EnumDeclaration
            EnumKeyword
            Identifier
            LBrace
            Sequence
                EnumMember
                    Identifier
                Repeat
                    Sequence
                        Comma
                        EnumMember
                            Identifier
                            Sequence
                                Assign
                                NumericLiteral
                    Sequence
                        Comma
                        EnumMember
                            Identifier
                            Sequence
                                Assign
                                StringLiteral
            RBrace
```

**tree-sitter CST**

```
    program
        enum_declaration
        "enum"
            identifier
            enum_body
            "{"
                property_identifier
            ","
                enum_assignment
                    property_identifier
                "="
                    number
            ","
                enum_assignment
                    property_identifier
                "="
                    string
                    ""
                    " (string_fragment) "
                    ""
            "}"
```


## namespace.ts

```
namespace Util { export const helper = 1; }
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        NamespaceDeclaration
            NamespaceKeyword
            QualifiedName
                Identifier
            LBrace
            StatementList
                ExportedDeclaration
                    ExportKeyword
                    VariableStatement
                        VariableDeclarationList
                            ConstKeyword
                            VariableDeclaration
                                Identifier
                                Sequence
                                    Assign
                                    NumericLiteral
                        Semicolon
            RBrace
```

**tree-sitter CST**

```
    program
        expression_statement
            internal_module
            "namespace"
                identifier
                statement_block
                "{"
                    export_statement
                    "export"
                        lexical_declaration
                        "const"
                            variable_declarator
                                identifier
                            "="
                                number
                        ";"
                "}"
```


## call_chain.ts

```
const v = obj.method(1, 2).prop[3];
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        PostfixExpressionSuffix
                            PostfixExpressionSuffix
                                Arguments
                                    PostfixExpressionSuffix
                                        Identifier
                                        Dot
                                        Identifier
                                    LParen
                                    Sequence
                                        Argument
                                            NumericLiteral
                                        Repeat
                                            Sequence
                                                Comma
                                                Argument
                                                    NumericLiteral
                                    RParen
                                Dot
                                Identifier
                            LBracket
                            NumericLiteral
                            RBracket
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                subscript_expression
                    member_expression
                        call_expression
                            member_expression
                                identifier
                            "."
                                property_identifier
                            arguments
                            "("
                                number
                            ","
                                number
                            ")"
                    "."
                        property_identifier
                "["
                    number
                "]"
        ";"
```


## template.ts

```
const tpl = tag`hi ${name}!`;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        TemplateLiteral
                            Identifier
                            TemplateHead
                            Identifier
                            TemplateTail
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                call_expression
                    identifier
                    template_string
                    "`"
                        string_fragment
                        template_substitution
                        "${"
                            identifier
                        "}"
                        string_fragment
                    "`"
        ";"
```


## jsx_attr.tsx

```
const el = <div onClick={() => go()} disabled>Hi {name}</div>;
```

- textparser status: `ERROR`
- tree-sitter root error: `False`

**textparser CST**

```
(no cst)
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                jsx_element
                    jsx_opening_element
                    "<"
                        identifier
                        jsx_attribute
                            property_identifier
                        "="
                            jsx_expression
                            "{"
                                arrow_function
                                    formal_parameters
                                    "("
                                    ")"
                                "=>"
                                    call_expression
                                        identifier
                                        arguments
                                        "("
                                        ")"
                            "}"
                        jsx_attribute
                            property_identifier
                    ">"
                    jsx_text
                    jsx_expression
                    "{"
                        identifier
                    "}"
                    jsx_closing_element
                    "</"
                        identifier
                    ">"
        ";"
```


## dts_ambient.d.ts

```
declare var gv: number;
declare function gf(a: string): void;
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        DeclaredVariableStatement
            DeclareKeyword
            VariableStatement
                VariableDeclarationList
                    VarKeyword
                    VariableDeclaration
                        Identifier
                        TypeAnnotation
                            Colon
                            TypeContext
                                ConditionalType
                                    UnionType
                                        IntersectionType
                                            PostfixType
                                                NumberKeyword
                Semicolon
        FunctionDeclaration
            Repeat
                DeclareKeyword
            FunctionKeyword
            Identifier
            BindingParameterList
                LParen
                BindingParameterListElements
                    BindingParameter
                        Identifier
                        TypeAnnotation
                            Colon
                            TypeContext
                                ConditionalType
                                    UnionType
                                        IntersectionType
                                            PostfixType
                                                StringKeyword
                RParen
            TypeAnnotation
                Colon
                TypeContext
                    ConditionalType
                        UnionType
                            IntersectionType
                                PostfixType
                                    VoidKeyword
            Semicolon
```

**tree-sitter CST**

```
    program
        ambient_declaration
        "declare"
            variable_declaration
            "var"
                variable_declarator
                    identifier
                    type_annotation
                    ":"
                        predefined_type
                        "number"
            ";"
        ambient_declaration
        "declare"
            function_signature
            "function"
                identifier
                formal_parameters
                "("
                    required_parameter
                        identifier
                        type_annotation
                        ":"
                            predefined_type
                            "string"
                ")"
                type_annotation
                ":"
                    predefined_type
                    "void"
            ";"
```


## new_expr.ts

```
const made = new Foo.Bar(1);
```

- textparser status: `OK`
- tree-sitter root error: `False`

**textparser CST**

```
SourceFile
    Repeat
        VariableStatement
            VariableDeclarationList
                ConstKeyword
                VariableDeclaration
                    Identifier
                    Sequence
                        Assign
                        NewExpression
                            NewKeyword
                            Identifier
                            Repeat
                                NewMemberSuffix
                                    Dot
                                    Identifier
                            Arguments
                                LParen
                                Sequence
                                    Argument
                                        NumericLiteral
                                RParen
            Semicolon
```

**tree-sitter CST**

```
    program
        lexical_declaration
        "const"
            variable_declarator
                identifier
            "="
                new_expression
                "new"
                    member_expression
                        identifier
                    "."
                        property_identifier
                    arguments
                    "("
                        number
                    ")"
        ";"
```

