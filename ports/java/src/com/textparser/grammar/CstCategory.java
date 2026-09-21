package com.textparser.grammar;

public enum CstCategory {
    UNKNOWN(0, "unknown"),
    TOKEN(1, "token"),
    SOURCE_FILE(2, "source_file"),
    DECLARATION(3, "declaration"),
    STATEMENT(4, "statement"),
    EXPRESSION(5, "expression"),
    TYPE(6, "type"),
    JSX(7, "jsx"),
    PATTERN(8, "pattern"),
    OTHER(9, "other");

    private final int value;
    private final String name;

    CstCategory(int value, String name) {
        this.value = value;
        this.name = name;
    }

    public int getValue() {
        return value;
    }

    public String getName() {
        return name;
    }

    public static CstCategory fromValue(int val) {
        for (CstCategory c : values()) {
            if (c.value == val) return c;
        }
        return UNKNOWN;
    }

    public static CstCategory fromString(String str) {
        if (str == null) return null;
        for (CstCategory c : values()) {
            if (c.name.equalsIgnoreCase(str)) return c;
        }
        return null;
    }
}
