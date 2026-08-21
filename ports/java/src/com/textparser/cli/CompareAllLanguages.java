package com.textparser.cli;

import com.textparser.JsonUtils;
import com.textparser.TextParser;
import com.textparser.TokenItem;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public class CompareAllLanguages {

    public static void main(String[] args) {
        String samplesDir = "tmp/samples";
        new File(samplesDir).mkdirs();

        Map<String, String> sampleCode = new LinkedHashMap<>();
        sampleCode.put("json", "{\"name\": \"textparser\", \"version\": 1.0, \"active\": true, \"tags\": [\"c\", \"java\"]}");
        sampleCode.put("javascript", "function add(a, b) { let res = a + b; return res; }");
        sampleCode.put("c", "#include <stdio.h>\nint main() { printf(\"Hello %d\\n\", 42); return 0; }");
        sampleCode.put("java", "public class Main { public static void main(String[] args) { System.out.println(123); } }");
        sampleCode.put("python", "def greet(name):\n    x = -10 + 20\n    return f\"Hello {name}\"\n");
        sampleCode.put("rust", "fn main() { let x = -10; println!(\"{}\", x); }");
        sampleCode.put("css", "body { margin: 0; padding: 10px; background-color: #ffffff; }");
        sampleCode.put("html", "<html><head><title>Test</title></head><body><h1>Hello</h1></body></html>");
        sampleCode.put("sql", "SELECT id, name FROM users WHERE age >= 18 ORDER BY id DESC;");
        sampleCode.put("bash", "#!/bin/bash\necho \"Starting task...\"\nx=100\n");

        int totalTested = 0;
        int exactMatches = 0;
        int totalDiffs = 0;

        for (Map.Entry<String, String> entry : sampleCode.entrySet()) {
            String lang = entry.getKey();
            String code = entry.getValue();

            String defFile = "definitions/" + lang + "_definition.json";
            String sampleFile = samplesDir + "/test_" + lang + "." + getExt(lang);

            try {
                Files.writeString(new File(sampleFile).toPath(), code, StandardCharsets.ISO_8859_1);

                TextParser parser = TextParser.fromFile(defFile);
                List<TokenItem> javaTree = parser.parse(code);

                ProcessBuilder pb = new ProcessBuilder("bin/textparser", sampleFile, "--definition", defFile, "--json");
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
                    System.out.println("❌ [" + lang.toUpperCase() + "] C binary exited with code " + exitCode);
                    totalDiffs++;
                    continue;
                }

                Object cJsonObj = JsonUtils.parseJson(cJsonSb.toString().trim());
                @SuppressWarnings("unchecked")
                List<Object> cTree = (List<Object>) cJsonObj;

                totalTested++;
                List<String> diffs = new ArrayList<>();
                boolean match = compareTreesDetailed(javaTree, cTree, "", diffs);

                if (match) {
                    exactMatches++;
                    System.out.println("✅ [" + lang.toUpperCase() + "] PERFECT MATCH: Java AST matches C AST 100% token-for-token.");
                } else {
                    totalDiffs++;
                    System.out.println("⚠️  [" + lang.toUpperCase() + "] AST DIFFERENCE DETECTED:");
                    for (String diff : diffs) {
                        System.out.println("    " + diff);
                    }
                }

            } catch (Exception e) {
                totalDiffs++;
                System.out.println("❌ [" + lang.toUpperCase() + "] Exception: " + e.getMessage());
            }
        }

        System.out.println("\n==========================================");
        System.out.println("Language Validation Summary:");
        System.out.println("  Languages Tested: " + totalTested);
        System.out.println("  100% Perfect AST Matches: " + exactMatches + " / " + totalTested);
        System.out.println("  Differences / Failures: " + totalDiffs);
        System.out.println("==========================================");
    }

    private static String getExt(String lang) {
        return switch (lang) {
            case "javascript" -> "js";
            case "python" -> "py";
            case "rust" -> "rs";
            default -> lang;
        };
    }

    @SuppressWarnings("unchecked")
    private static boolean compareTreesDetailed(List<TokenItem> javaTree, List<Object> cTree, String path, List<String> diffs) {
        if (javaTree.size() != cTree.size()) {
            diffs.add(path + " Child count mismatch -> Java: " + javaTree.size() + ", C: " + cTree.size());
            return false;
        }

        boolean allMatch = true;
        for (int i = 0; i < javaTree.size(); i++) {
            TokenItem jNode = javaTree.get(i);
            Map<String, Object> cNode = (Map<String, Object>) cTree.get(i);

            String currentPath = path + "[" + i + ":" + jNode.id + "]";
            String cId = cNode.get("id") != null ? cNode.get("id").toString() : "";
            int cPos = cNode.get("position") != null ? ((Number) cNode.get("position")).intValue() : 0;
            int cLen = cNode.get("length") != null ? ((Number) cNode.get("length")).intValue() : 0;

            if (!jNode.id.equals(cId)) {
                diffs.add(currentPath + " ID mismatch -> Java: '" + jNode.id + "', C: '" + cId + "'");
                allMatch = false;
            }
            if (jNode.position != cPos) {
                diffs.add(currentPath + " Position mismatch -> Java: " + jNode.position + ", C: " + cPos);
                allMatch = false;
            }
            if (jNode.length != cLen) {
                diffs.add(currentPath + " Length mismatch -> Java: " + jNode.length + ", C: " + cLen);
                allMatch = false;
            }

            List<Object> cChildren = cNode.containsKey("children") && cNode.get("children") instanceof List<?> ? (List<Object>) cNode.get("children") : new ArrayList<>();
            List<TokenItem> jChildren = jNode.children != null ? jNode.children : new ArrayList<>();

            if (!compareTreesDetailed(jChildren, cChildren, currentPath + "/", diffs)) {
                allMatch = false;
            }
        }

        return allMatch;
    }
}
