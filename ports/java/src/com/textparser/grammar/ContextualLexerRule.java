package com.textparser.grammar;

public class ContextualLexerRule {
    public int priority;
    public boolean isTrivia;
    public String pushMode;
    public boolean popMode;
    public String validator;
    public int capture;
    public int captureFlag;
    public int dynamic;
    public int dynamicTrigger;

    public ContextualLexerRule() {}
}
