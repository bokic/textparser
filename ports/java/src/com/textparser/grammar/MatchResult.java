package com.textparser.grammar;

public class MatchResult {
    public MatchStatus status = MatchStatus.NO;
    public Node node;
    public int consumedTokens;
    public boolean committed;

    public MatchResult() {}

    public MatchResult(MatchStatus status, Node node, int consumedTokens, boolean committed) {
        this.status = status;
        this.node = node;
        this.consumedTokens = consumedTokens;
        this.committed = committed;
    }
}
