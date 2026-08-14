package com.textparser.cli;

import com.textparser.JsonUtils;
import com.textparser.TextParser;
import com.textparser.TokenItem;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class Validate {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: Validate <definition_file> <directory_or_file>");
            System.exit(1);
        }

        String definitionFile = args[0];
        String targetPathStr = args[1];

        try {
            TextParser parser = TextParser.fromFile(definitionFile);
            File targetPath = new File(targetPathStr);

            List<String> defaultExts = new ArrayList<>();
            if (parser.definition != null && parser.definition.tokens != null) {
                defaultExts.add("cfm");
                defaultExts.add("cfc");
            }

            recursiveParseDirectory(parser, definitionFile, targetPath, defaultExts);
        } catch (Exception e) {
            System.err.println("Error loading definition or executing validation: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static void recursiveParseDirectory(TextParser parser, String definitionFile, File targetPath, List<String> defaultExts) {
        if (targetPath.isFile()) {
            processFile(parser, definitionFile, targetPath);
            return;
        }

        File[] files = targetPath.listFiles();
        if (files == null) return;

        for (File file : files) {
            if (file.isFile()) {
                String name = file.getName().toLowerCase();
                boolean isTargetExt = name.endsWith(".py") || name.endsWith(".rs") || name.endsWith(".java");
                for (String ext : defaultExts) {
                    if (name.endsWith("." + ext.toLowerCase())) {
                        isTargetExt = true;
                        break;
                    }
                }
                if (isTargetExt) {
                    processFile(parser, definitionFile, file);
                }
            } else if (file.isDirectory()) {
                recursiveParseDirectory(parser, definitionFile, file, defaultExts);
            }
        }
    }

    private static void processFile(TextParser parser, String definitionFile, File file) {
        System.out.print("Comparing " + file.getPath() + "...");
        System.out.flush();

        try {
            byte[] bytes = Files.readAllBytes(file.toPath());
            String text = new String(bytes, StandardCharsets.ISO_8859_1);

            List<TokenItem> javaTree = parser.parse(text);

            ProcessBuilder pb = new ProcessBuilder("bin/textparser", file.getPath(), "--definition", definitionFile, "--json");
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
                System.out.println(" bin/textparser failed with exit code " + exitCode);
                System.exit(1);
            }

            Object cJsonObj = JsonUtils.parseJson(cJsonSb.toString());
            @SuppressWarnings("unchecked")
            List<Object> cTree = (List<Object>) cJsonObj;

            if (!compareTrees(javaTree, cTree)) {
                System.exit(1);
            }

            System.out.println(" done");
        } catch (Exception e) {
            System.out.println(" error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    @SuppressWarnings("unchecked")
    private static boolean compareTrees(List<TokenItem> javaTree, List<Object> cTree) {
        if (javaTree.size() != cTree.size()) {
            System.out.println("\nLength mismatch. Java: " + javaTree.size() + ", C: " + cTree.size());
            return false;
        }

        for (int i = 0; i < javaTree.size(); i++) {
            TokenItem javaNode = javaTree.get(i);
            Map<String, Object> cNode = (Map<String, Object>) cTree.get(i);

            String cId = cNode.get("id") != null ? cNode.get("id").toString() : "";
            int cPos = cNode.get("position") != null ? ((Number) cNode.get("position")).intValue() : 0;
            int cLen = cNode.get("length") != null ? ((Number) cNode.get("length")).intValue() : 0;

            if (!javaNode.id.equals(cId)) {
                System.out.println("\nID mismatch.");
                System.out.println("ID - Java: " + javaNode.id + ", C: " + cId);
                System.out.println("Position - Java: " + javaNode.position + ", C: " + cPos);
                System.out.println("Length - Java: " + javaNode.length + ", C: " + cLen);
                return false;
            }

            if (javaNode.position != cPos) {
                System.out.println("\nPosition mismatch.");
                System.out.println("ID - Java: " + javaNode.id + ", C: " + cId);
                System.out.println("Position - Java: " + javaNode.position + ", C: " + cPos);
                System.out.println("Length - Java: " + javaNode.length + ", C: " + cLen);
                return false;
            }

            if (javaNode.length != cLen) {
                System.out.println("\nLength mismatch. Java: " + javaNode.length + ", C: " + cLen);
                System.out.println("ID - Java: " + javaNode.id + ", C: " + cId);
                System.out.println("Position - Java: " + javaNode.position + ", C: " + cPos);
                System.out.println("Length - Java: " + javaNode.length + ", C: " + cLen);
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
