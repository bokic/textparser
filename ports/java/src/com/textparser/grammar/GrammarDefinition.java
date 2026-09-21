package com.textparser.grammar;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class GrammarDefinition {
    public int startProduction = -1;
    public List<Production> productions = new ArrayList<>();
    public String sourceCompleteHandler;
    public String sourceCompleteConfiguration;
    public RecoveryPolicy recoveryPolicy = new RecoveryPolicy();
    public String initialLexerMode = "default";
    public Map<String, LexerMode> lexerModes = new LinkedHashMap<>();
    public Map<String, LexerGoal> lexerGoals = new LinkedHashMap<>();
    public Map<Integer, ContextualLexerRule> lexerRules = new LinkedHashMap<>();
    public List<OperatorDef> operators = new ArrayList<>();
    public Map<String, List<String>> sourceFileKinds = new LinkedHashMap<>();

    public Production getProduction(int id) {
        if (id >= 0 && id < productions.size()) {
            return productions.get(id);
        }
        return null;
    }

    public Production getProductionByName(String name) {
        if (name == null) return null;
        for (Production p : productions) {
            if (name.equals(p.name)) {
                return p;
            }
        }
        return null;
    }
}
