package com.textparser.cli;

import com.textparser.TextParser;
import com.textparser.TokenItem;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;

public class ParseDir {

    private static int fileCount = 0;
    private static int totalTokens = 0;
    private static int successCount = 0;

    private static void printUsage() {
        System.out.println("Usage: ParseDir <definition_file> <directory_path>");
        System.exit(1);
    }

    public static void main(String[] args) {
        if (args.length < 2) {
            printUsage();
        }

        try {
            TextParser parser = TextParser.fromFile(args[0]);
            File dir = new File(args[1]);

            if (!dir.exists()) {
                System.err.println("Error: Directory or file does not exist: " + args[1]);
                System.exit(1);
            }

            processDirectory(parser, dir);

            System.out.println("\nSummary: Successfully parsed " + successCount + " / " + fileCount + " files, total top-level tokens: " + totalTokens);
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static void processDirectory(TextParser parser, File target) {
        if (target.isFile()) {
            parseFile(parser, target);
            return;
        }

        File[] files = target.listFiles();
        if (files == null) return;

        for (File file : files) {
            if (file.isDirectory()) {
                processDirectory(parser, file);
            } else if (file.isFile()) {
                String name = file.getName().toLowerCase();
                if (name.endsWith(".cfm") || name.endsWith(".cfc") || name.endsWith(".json") || name.endsWith(".js") || name.endsWith(".c") || name.endsWith(".py")) {
                    parseFile(parser, file);
                }
            }
        }
    }

    private static void parseFile(TextParser parser, File file) {
        fileCount++;
        try {
            byte[] bytes = Files.readAllBytes(file.toPath());
            String text = new String(bytes, StandardCharsets.ISO_8859_1);
            List<TokenItem> tokens = parser.parse(text);
            successCount++;
            totalTokens += tokens.size();
            System.out.println("Parsed " + file.getPath() + " -> " + tokens.size() + " top-level tokens.");
        } catch (Exception e) {
            System.err.println("Failed to parse " + file.getPath() + ": " + e.getMessage());
        }
    }
}
