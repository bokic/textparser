package com.textparser.grammar;

public enum ProductionKind {
    TOKEN,
    REF,
    SEQUENCE,
    CHOICE,
    OPTIONAL,
    REPEAT,
    LOOKAHEAD,
    NOT,
    PREDICATE,
    CONTEXT,
    COMMIT,
    PRATT,
    LEXICAL_GOAL,
    CAPTURE,
    MATCH_CAPTURE
}
