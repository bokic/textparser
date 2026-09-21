package com.textparser.grammar;

public class LexerMode {
    public String name;
    public int[] tokens;
    public int[] trivia;

    public LexerMode() {}

    public LexerMode(String name, int[] tokens, int[] trivia) {
        this.name = name;
        this.tokens = tokens;
        this.trivia = trivia;
    }
}
