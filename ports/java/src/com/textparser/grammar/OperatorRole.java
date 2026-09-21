package com.textparser.grammar;

public enum OperatorRole {
    INFIX,
    PREFIX,
    POSTFIX,
    TERNARY;

    public static OperatorRole fromString(String name) {
        if (name == null) return null;
        for (OperatorRole role : values()) {
            if (role.name().equalsIgnoreCase(name)) return role;
        }
        return null;
    }
}
