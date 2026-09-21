package com.textparser.grammar;

public class Production {
    public int id;
    public String name;
    public ProductionKind kind;
    public int[] children;
    public int tokenId = -1;
    public int referencedProduction = -1;
    public String predicateName;
    public String contextName;
    public long contextValue;
    public int minimumPrecedence;
    public int recoveryInsertToken = -1;
    public boolean recoveryInsertEnabled;
    public int[] recoverySyncTokens;
    public boolean recoverySkip;
    public boolean allowAutomaticSemicolon;
    public String expectedDescription;
    public String validateHandler;
    public String validateConfiguration;
    public String commitHandler;
    public String commitConfiguration;
    public String recoveryHandler;
    public String recoveryConfiguration;
    public String lexicalGoal;
    public String captureName;
    public CstCategory category = CstCategory.UNKNOWN;
    public Guard guard;
    public DiagnosticTemplates diagnostics = new DiagnosticTemplates();
    public String astKind;

    public Production() {}

    public Production(int id, String name, ProductionKind kind) {
        this.id = id;
        this.name = name;
        this.kind = kind;
    }
}
