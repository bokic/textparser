package com.textparser;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonSerializer;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class TokenItem {
    public static final String ERROR = "Error";
    public static final String UNPROCESSED = "Unprocessed";
    public static final String WHITESPACE = "Whitespace";
    public static final String START_DELIMITER = "StartDelimiter";
    public static final String END_DELIMITER = "EndDelimiter";

    /**
     * Serialize a node, omitting hidden whitespace leaves the way the C JSON
     * writer does (they are part of the in-memory CST but not the output).
     */
    private static final JsonSerializer<TokenItem> SERIALIZER = (src, type, ctx) -> {
        JsonObject obj = new JsonObject();
        obj.addProperty("id", src.id);
        obj.addProperty("position", src.position);
        obj.addProperty("length", src.length);
        if (src.children != null && !src.children.isEmpty()) {
            JsonArray arr = new JsonArray();
            for (TokenItem child : src.children) {
                if (isHiddenWhitespace(child)) {
                    continue;
                }
                arr.add(ctx.serialize(child));
            }
            obj.add("children", arr);
        }
        return obj;
    };

    private static final Gson PRETTY_GSON = new GsonBuilder()
            .registerTypeAdapter(TokenItem.class, SERIALIZER)
            .setPrettyPrinting().create();
    private static final Gson COMPACT_GSON = new GsonBuilder()
            .registerTypeAdapter(TokenItem.class, SERIALIZER).create();

    private static boolean isHiddenWhitespace(TokenItem item) {
        return WHITESPACE.equals(item.id) && (item.children == null || item.children.isEmpty());
    }

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
        List<TokenItem> visible = new ArrayList<>();
        if (tokens != null) {
            for (TokenItem token : tokens) {
                if (isHiddenWhitespace(token)) {
                    continue;
                }
                visible.add(token);
            }
        }
        return pretty ? PRETTY_GSON.toJson(visible) : COMPACT_GSON.toJson(visible);
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
