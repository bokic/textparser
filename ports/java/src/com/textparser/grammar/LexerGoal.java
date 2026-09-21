package com.textparser.grammar;

import java.util.LinkedHashMap;
import java.util.Map;

public class LexerGoal {
    public String name;
    /** Map of source token ID to target token ID under this goal. */
    public Map<Integer, Integer> mappings = new LinkedHashMap<>();

    public LexerGoal() {}

    public LexerGoal(String name) {
        this.name = name;
    }

    public void addMapping(int sourceToken, int targetToken) {
        mappings.put(sourceToken, targetToken);
    }
}
