package com.textparser.grammar;

public class Guard {
    /** 0: unrestricted, 1: require newline, -1: forbid newline. */
    public int lineTerminatorBefore = 0;
    public int[] nextTokens;
    public boolean allowEof = false;
    public String nextTokenText;
    public String[] fileSuffixes;
}
