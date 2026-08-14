package com.textparser;

import java.util.*;

public class Definition {
    public String name;
    public String version = "0.0";
    public boolean otherTextInside = false;
    public List<String> startTokens = new ArrayList<>();
    public Map<String, TokenDef> tokens = new LinkedHashMap<>();
    public MergeSignConfig mergeSignIntoNumber = null;

    public static class MergeSignConfig {
        public List<String> signTokens = new ArrayList<>();
        public List<String> numberTokens = new ArrayList<>();
        public List<String> operandTokens = new ArrayList<>();
    }

    public static class TokenDef {
        public String id;
        public String type; // SimpleToken, StartStop, StartOptStop, Group, GroupOneChildOnly, GroupAllChildrenInSameOrder
        public String startRegex;
        public String endRegex;
        public List<String> nestedTokens;
        public Boolean otherTextInside = null;
        public boolean deleteIfOnlyOneChild = false;
        public boolean mustHaveOneChild = false;
        public boolean multiLine = false;
        public boolean searchParentEndTokenLast = false;
        public String textColor;
        public String bgColor;
    }

    @SuppressWarnings("unchecked")
    public static Definition fromJson(Map<String, Object> map) {
        Definition def = new Definition();
        if (map.containsKey("name") && map.get("name") != null) {
            def.name = map.get("name").toString();
        }
        if (map.containsKey("version") && map.get("version") != null) {
            def.version = map.get("version").toString();
        }
        if (map.containsKey("otherTextInside") && map.get("otherTextInside") instanceof Boolean b) {
            def.otherTextInside = b;
        }

        if (map.get("startTokens") instanceof List<?> list) {
            for (Object item : list) {
                if (item != null) def.startTokens.add(item.toString());
            }
        }

        if (map.get("mergeSignIntoNumber") instanceof Map<?, ?> signMap) {
            MergeSignConfig config = new MergeSignConfig();
            if (signMap.get("signTokens") instanceof List<?> stList) {
                for (Object item : stList) if (item != null) config.signTokens.add(item.toString());
            }
            if (signMap.get("numberTokens") instanceof List<?> ntList) {
                for (Object item : ntList) if (item != null) config.numberTokens.add(item.toString());
            }
            if (signMap.get("operandTokens") instanceof List<?> opList) {
                for (Object item : opList) if (item != null) config.operandTokens.add(item.toString());
            }
            def.mergeSignIntoNumber = config;
        }

        if (map.get("tokens") instanceof Map<?, ?> tokenMap) {
            for (Map.Entry<?, ?> entry : tokenMap.entrySet()) {
                String tokenId = entry.getKey().toString();
                if (entry.getValue() instanceof Map<?, ?> tData) {
                    TokenDef tDef = new TokenDef();
                    tDef.id = tokenId;
                    if (tData.get("type") != null) tDef.type = tData.get("type").toString();
                    if (tData.get("startRegex") != null) tDef.startRegex = tData.get("startRegex").toString();
                    if (tData.get("endRegex") != null) tDef.endRegex = tData.get("endRegex").toString();

                    if (tData.get("otherTextInside") instanceof Boolean b) tDef.otherTextInside = b;
                    if (tData.get("deleteIfOnlyOneChild") instanceof Boolean b) tDef.deleteIfOnlyOneChild = b;
                    if (tData.get("mustHaveOneChild") instanceof Boolean b) tDef.mustHaveOneChild = b;
                    if (tData.get("multiLine") instanceof Boolean b) tDef.multiLine = b;
                    if (tData.get("searchParentEndTokenLast") instanceof Boolean b) tDef.searchParentEndTokenLast = b;

                    if (tData.get("textColor") != null) tDef.textColor = tData.get("textColor").toString();
                    if (tData.get("bgColor") != null) tDef.bgColor = tData.get("bgColor").toString();

                    if (tData.get("nestedTokens") instanceof List<?> nList) {
                        tDef.nestedTokens = new ArrayList<>();
                        for (Object item : nList) if (item != null) tDef.nestedTokens.add(item.toString());
                    }

                    def.tokens.put(tokenId, tDef);
                }
            }
        }

        return def;
    }
}
