package com.textparser.grammar;

public class RecoveryPolicy {
    public int maximumDiagnostics = 100;
    public int maximumSkippedTokens = 256;
    public int maximumRecoveryAttempts = 100;
    public int[] synchronizationTokens;

    public RecoveryPolicy() {}
}
