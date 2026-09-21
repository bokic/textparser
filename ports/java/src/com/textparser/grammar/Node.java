package com.textparser.grammar;

public class Node {
    public static final int NODE_NONE = 0;
    public static final int NODE_SYNTHETIC = 1 << 0;
    public static final int NODE_MISSING = 1 << 1;
    public static final int NODE_RECOVERED = 1 << 2;
    public static final int NODE_TRIVIA = 1 << 3;
    public static final int NODE_GRAMMAR_POSTFIX = 1 << 4;
    public static final int NODE_EXPLICIT_SPAN = 1 << 7;

    public Node prev;
    public Node next;
    public Node child;
    public Node parent;

    public int tokenId = -1;
    public int position;
    public int length;
    public long textColor;
    public long textBackground;
    public long textFlags;
    public String error;

    public long id;
    public int nodeFlags;
    public String decodedValue;
    public Object userData;

    public String cstKind;
    public int sourceStart;
    public int sourceEnd;
    public CstCategory category = CstCategory.UNKNOWN;
    public int spanLen;

    public Node() {}

    public Node(int tokenId, int position, int length) {
        this.tokenId = tokenId;
        this.position = position;
        this.length = length;
        this.spanLen = length;
        this.sourceStart = position;
        this.sourceEnd = position + length;
    }

    public void addChild(Node newChild) {
        if (newChild == null) return;
        newChild.parent = this;
        if (this.child == null) {
            this.child = newChild;
        } else {
            Node curr = this.child;
            while (curr.next != null) {
                curr = curr.next;
            }
            curr.next = newChild;
            newChild.prev = curr;
        }
        this.spanLen += newChild.spanLen;
    }

    public boolean hasExplicitSpan() {
        return (nodeFlags & NODE_EXPLICIT_SPAN) != 0;
    }
}
