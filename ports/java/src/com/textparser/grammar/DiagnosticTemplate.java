package com.textparser.grammar;

public class DiagnosticTemplate {
    public String code;
    public String message;

    public DiagnosticTemplate() {}

    public DiagnosticTemplate(String code, String message) {
        this.code = code;
        this.message = message;
    }
}
