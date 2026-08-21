package com.textparser;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class TokenItem {
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

    public String toJson(int indentLevel) {
        StringBuilder sb = new StringBuilder();
        String indent = "  ".repeat(indentLevel);
        String childIndent = "  ".repeat(indentLevel + 1);

        sb.append(indent).append("{\n");
        sb.append(childIndent).append("\"id\": \"").append(escapeJson(id)).append("\",\n");
        sb.append(childIndent).append("\"position\": ").append(position).append(",\n");
        sb.append(childIndent).append("\"length\": ").append(length);

        if (children != null && !children.isEmpty()) {
            sb.append(",\n").append(childIndent).append("\"children\": [\n");
            for (int i = 0; i < children.size(); i++) {
                sb.append(children.get(i).toJson(indentLevel + 2));
                if (i < children.size() - 1) {
                    sb.append(",");
                }
                sb.append("\n");
            }
            sb.append(childIndent).append("]");
        }
        sb.append("\n").append(indent).append("}");
        return sb.toString();
    }

    private static String escapeJson(String input) {
        if (input == null) return "";
        return input.replace("\\", "\\\\")
                    .replace("\"", "\\\"")
                    .replace("\n", "\\n")
                    .replace("\r", "\\r")
                    .replace("\t", "\\t");
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
        return toJson(0);
    }
}
