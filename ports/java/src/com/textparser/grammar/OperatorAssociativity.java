package com.textparser.grammar;

public enum OperatorAssociativity {
    LEFT,
    RIGHT;

    public static OperatorAssociativity fromString(String str) {
        if (str == null) return LEFT;
        if (str.equalsIgnoreCase("right")) return RIGHT;
        return LEFT;
    }
}
