package com.textparser.cli;

import com.textparser.TextParser;
import com.textparser.TokenItem;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public class Parse {

    private static void printUsage() {
        System.out.println("Usage: Parse <definition_file> [<text_file> --format] | [--stdinformat]");
        System.exit(1);
    }

    public static void main(String[] args) {
        if (args.length < 2) {
            printUsage();
        }

        try {
            TextParser parser = TextParser.fromFile(args[0]);

            if (args.length == 2 && "--stdinformat".equals(args[1])) {
                byte[] bytes = System.in.readAllBytes();
                String text = new String(bytes, StandardCharsets.ISO_8859_1);
                byte[] formatted = parser.parseFormat(text);
                System.out.println(bytesToHex(formatted));
            } else if (args.length == 2) {
                byte[] bytes = Files.readAllBytes(Path.of(args[1]));
                String text = new String(bytes, StandardCharsets.ISO_8859_1);
                List<TokenItem> tokens = parser.parse(text);
                System.out.println(toJsonArray(tokens, 0));
            } else if (args.length == 3 && "--format".equals(args[2])) {
                byte[] bytes = Files.readAllBytes(Path.of(args[1]));
                String text = new String(bytes, StandardCharsets.ISO_8859_1);
                byte[] formatted = parser.parseFormat(text);
                System.out.println(bytesToHex(formatted));
            } else {
                printUsage();
            }
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    public static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b & 0xff));
        }
        return sb.toString();
    }

    public static String toJsonArray(List<TokenItem> tokens, int indentLevel) {
        StringBuilder sb = new StringBuilder();
        String indent = "  ".repeat(indentLevel);
        sb.append("[\n");
        for (int i = 0; i < tokens.size(); i++) {
            sb.append(tokens.get(i).toJson(indentLevel + 1));
            if (i < tokens.size() - 1) {
                sb.append(",");
            }
            sb.append("\n");
        }
        sb.append(indent).append("]");
        return sb.toString();
    }
}
