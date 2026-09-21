package com.textparser.grammar;

public class OperatorDef {
    public int tokenId = -1;
    public OperatorRole role;
    public int precedence;
    public OperatorAssociativity associativity = OperatorAssociativity.LEFT;
    public int secondaryTokenId = -1;
    public String leftValidator;
    public String operandValidator;

    public OperatorDef() {}
}
