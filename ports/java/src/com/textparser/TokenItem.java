package com.textparser;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class TokenItem {
    private static final Gson PRETTY_GSON = new GsonBuilder().setPrettyPrinting().create();
    private static final Gson COMPACT_GSON = new GsonBuilder().create();

    public String id;
    public int position;
    public int length;
    public List<TokenItem> children;

    public TokenItem() {
        this.children = new ArrayList<>();
    }

    public TokenItem(String id, int position, int length) {
        this.id = id;
        this.position = position;
        this.length = length;
        this.children = new ArrayList<>();
    }

    public String toJson() {
        return toJson(true);
    }

    public String toJson(boolean pretty) {
        return pretty ? PRETTY_GSON.toJson(this) : COMPACT_GSON.toJson(this);
    }

    public String toJson(int indentLevel) {
        return toJson(true);
    }

    public static String toJson(List<TokenItem> tokens) {
        return toJson(tokens, true);
    }

    public static String toJson(List<TokenItem> tokens, boolean pretty) {
        return pretty ? PRETTY_GSON.toJson(tokens) : COMPACT_GSON.toJson(tokens);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        TokenItem tokenItem = (TokenItem) o;
        return position == tokenItem.position &&
                length == tokenItem.length &&
                Objects.equals(id, tokenItem.id) &&
                Objects.equals(children, tokenItem.children);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, position, length, children);
    }

    @Override
    public String toString() {
        return toJson();
    }
}
