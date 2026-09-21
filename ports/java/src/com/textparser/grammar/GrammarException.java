package com.textparser.grammar;

public class GrammarException extends RuntimeException {
    public static final int NO_ERROR = 0;
    public static final int INVALID_TOKEN_TYPE = 29;
    public static final int GRAMMAR_NOT_OBJECT = 30;
    public static final int GRAMMAR_START_NOT_FOUND = 31;
    public static final int GRAMMAR_PRODUCTIONS_NOT_OBJECT = 32;
    public static final int GRAMMAR_INVALID_PRODUCTION = 33;
    public static final int GRAMMAR_UNDEFINED_TOKEN = 34;
    public static final int GRAMMAR_UNDEFINED_REFERENCE = 35;
    public static final int GRAMMAR_LEFT_RECURSION = 36;
    public static final int GRAMMAR_NULLABLE_REPEAT = 37;

    private final int errorCode;

    public GrammarException(int errorCode, String message) {
        super(message);
        this.errorCode = errorCode;
    }

    public int getErrorCode() {
        return errorCode;
    }

    public static String getErrorString(int code) {
        switch (code) {
            case GRAMMAR_NOT_OBJECT:
                return "Grammar section must be an object";
            case GRAMMAR_START_NOT_FOUND:
                return "Start production not specified in grammar";
            case GRAMMAR_PRODUCTIONS_NOT_OBJECT:
                return "Grammar productions must be a non-empty object";
            case GRAMMAR_INVALID_PRODUCTION:
                return "Invalid grammar production construct";
            case GRAMMAR_UNDEFINED_TOKEN:
                return "Grammar references undefined token";
            case GRAMMAR_UNDEFINED_REFERENCE:
                return "Grammar references undefined production";
            case GRAMMAR_LEFT_RECURSION:
                return "Grammar contains left recursion cycle";
            case GRAMMAR_NULLABLE_REPEAT:
                return "Grammar contains repeat over nullable production";
            case INVALID_TOKEN_TYPE:
                return "Invalid token type or mode definition";
            default:
                return "Unknown grammar error (" + code + ")";
        }
    }
}
