package com.textparser.cli;

import com.textparser.JsonUtils;
import com.textparser.TextParser;
import com.textparser.TokenItem;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ValidateAll {

    private static int totalFiles = 0;
    private static int matchedFiles = 0;
    private static int javaExceptions = 0;
    private static int cFailures = 0;
    private static int cEmptyOutput = 0;
    private static int treeMismatches = 0;

    private static final Map<String, Integer> javaExceptionTypes = new HashMap<>();

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: ValidateAll <definition_file> <directory>");
            System.exit(1);
        }

        String definitionFile = args[0];
        String targetDir = args[1];

        try {
            TextParser parser = TextParser.fromFile(definitionFile);
            File dir = new File(targetDir);

            if (!dir.exists()) {
                System.err.println("Error: Directory does not exist: " + targetDir);
                System.exit(1);
            }

            processDirectory(parser, definitionFile, dir);

            System.out.println("\n=== Detailed Validation Breakdown ===");
            System.out.println("  Total .cfm/.cfc files inspected: " + totalFiles);
            System.out.println("  100% Identical Token Trees (Java vs C): " + matchedFiles);
            System.out.println("  Java Parser Exceptions: " + javaExceptions);
            for (Map.Entry<String, Integer> entry : javaExceptionTypes.entrySet()) {
                System.out.println("    - " + entry.getKey() + ": " + entry.getValue());
            }
            System.out.println("  C Binary Failures / Exit != 0: " + cFailures);
            System.out.println("  C Binary Empty JSON Output: " + cEmptyOutput);
            System.out.println("  AST Tree Structure Mismatches: " + treeMismatches);

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static void processDirectory(TextParser parser, String definitionFile, File target) {
        if (target.isFile()) {
            validateSingleFile(parser, definitionFile, target);
            return;
        }

        File[] files = target.listFiles();
        if (files == null) return;

        for (File file : files) {
            if (file.isDirectory()) {
                processDirectory(parser, definitionFile, file);
            } else if (file.isFile()) {
                String name = file.getName().toLowerCase();
                if (name.endsWith(".cfm") || name.endsWith(".cfc")) {
                    validateSingleFile(parser, definitionFile, file);
                }
            }
        }
    }

    private static void validateSingleFile(TextParser parser, String definitionFile, File file) {
        totalFiles++;

        List<TokenItem> javaTree;
        try {
            byte[] bytes = Files.readAllBytes(file.toPath());
            String text = new String(bytes, StandardCharsets.ISO_8859_1);
            javaTree = parser.parse(text);
        } catch (Exception e) {
            javaExceptions++;
            String msg = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
            javaExceptionTypes.put(msg, javaExceptionTypes.getOrDefault(msg, 0) + 1);
            return;
        }

        List<Object> cTree;
        try {
            ProcessBuilder pb = new ProcessBuilder("bin/textparser", file.getPath(), "--json");
            Process proc = pb.start();

            StringBuilder cJsonSb = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    cJsonSb.append(line).append("\n");
                }
            }

            int exitCode = proc.waitFor();
            if (exitCode != 0) {
                cFailures++;
                return;
            }

            String cOutput = cJsonSb.toString().trim();
            if (cOutput.isEmpty()) {
                cEmptyOutput++;
                return;
            }

            Object cJsonObj = JsonUtils.parseJson(cOutput);
            @SuppressWarnings("unchecked")
            List<Object> list = (List<Object>) cJsonObj;
            cTree = list;
        } catch (Exception e) {
            cFailures++;
            return;
        }

        if (compareTrees(javaTree, cTree)) {
            matchedFiles++;
        } else {
            treeMismatches++;
        }
    }

    @SuppressWarnings("unchecked")
    private static boolean compareTrees(List<TokenItem> javaTree, List<Object> cTree) {
        if (javaTree.size() != cTree.size()) return false;

        for (int i = 0; i < javaTree.size(); i++) {
            TokenItem javaNode = javaTree.get(i);
            Map<String, Object> cNode = (Map<String, Object>) cTree.get(i);

            String cId = cNode.get("id") != null ? cNode.get("id").toString() : "";
            int cPos = cNode.get("position") != null ? ((Number) cNode.get("position")).intValue() : 0;
            int cLen = cNode.get("length") != null ? ((Number) cNode.get("length")).intValue() : 0;

            if (!javaNode.id.equals(cId) || javaNode.position != cPos || javaNode.length != cLen) {
                return false;
            }

            List<Object> cChildren = cNode.containsKey("children") && cNode.get("children") instanceof List<?> ? (List<Object>) cNode.get("children") : null;
            if (cChildren != null && !cChildren.isEmpty()) {
                if (!compareTrees(javaNode.children, cChildren)) {
                    return false;
                }
            }
        }
        return true;
    }
}
