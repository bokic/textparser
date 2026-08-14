package com.textparser;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

public class TextParser {
    public Definition definition;
    private final Map<String, Pattern> patternCache = new HashMap<>();

    public TextParser(Definition definition) {
        this.definition = definition;
        initializeDefaults();
    }

    public TextParser(String definitionJson) {
        @SuppressWarnings("unchecked")
        Map<String, Object> map = (Map<String, Object>) JsonUtils.parseJson(definitionJson);
        this.definition = Definition.fromJson(map);
        initializeDefaults();
    }

    public static TextParser fromFile(String filePath) throws IOException {
        String content = Files.readString(Path.of(filePath));
        return new TextParser(content);
    }

    private void initializeDefaults() {
        if (definition.tokens != null) {
            for (Map.Entry<String, Definition.TokenDef> entry : definition.tokens.entrySet()) {
                Definition.TokenDef token = entry.getValue();
                if (token.id == null) {
                    token.id = entry.getKey();
                }
                if (token.otherTextInside == null) {
                    token.otherTextInside = false;
                }
            }
        }
    }

    private Pattern getCompiledPattern(String regexStr) {
        if (regexStr == null) return null;
        return patternCache.computeIfAbsent(regexStr, r -> {
            try {
                return Pattern.compile(r, Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
            } catch (PatternSyntaxException e) {
                String fixed = fixRegexPattern(r);
                try {
                    return Pattern.compile(fixed, Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
                } catch (Exception e2) {
                    throw new RuntimeException("Failed to compile regex pattern: " + r, e);
                }
            } catch (Exception e) {
                throw new RuntimeException("Failed to compile regex pattern: " + r, e);
            }
        });
    }

    private String fixRegexPattern(String r) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < r.length(); i++) {
            char c = r.charAt(i);
            if ((c == '{' || c == '}') && (i == 0 || r.charAt(i - 1) != '\\')) {
                sb.append('\\');
            }
            sb.append(c);
        }
        return sb.toString();
    }

    private int skipWhitespace(String text, int pos) {
        while (pos < text.length()) {
            char c = text.charAt(pos);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos++;
            } else {
                break;
            }
        }
        return pos;
    }

    private Integer findToken(String text, int pos, Definition.TokenDef token, boolean otherTextInside) {
        if (token == null || token.type == null) return null;
        switch (token.type) {
            case "Group", "GroupOneChildOnly" -> {
                if (token.nestedTokens == null) return null;
                int closestChildPos = Integer.MAX_VALUE;
                for (String childTokenName : token.nestedTokens) {
                    Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                    Integer childPos = findToken(text, pos, childDef, otherTextInside);
                    if (childPos != null && childPos < closestChildPos) {
                        closestChildPos = childPos;
                    }
                }
                return closestChildPos == Integer.MAX_VALUE ? null : closestChildPos;
            }
            case "GroupAllChildrenInSameOrder" -> {
                if (token.nestedTokens == null || token.nestedTokens.isEmpty()) return null;
                Definition.TokenDef childDef = definition.tokens.get(token.nestedTokens.get(0));
                return findToken(text, pos, childDef, otherTextInside);
            }
            case "SimpleToken", "StartStop", "StartOptStop" -> {
                if (token.startRegex == null) return null;
                Pattern p = getCompiledPattern(token.startRegex);
                Matcher m = p.matcher(text.substring(pos));
                if (!m.find()) return null;

                int group = m.groupCount();
                int matchStart = m.start(group);
                int matchEnd = m.end(group);

                if (matchEnd - matchStart == 0) return null;
                if (!otherTextInside && m.start() != 0) return null;

                return matchStart;
            }
            default -> throw new RuntimeException("Unknown token type: " + token.type);
        }
    }

    private TokenItem parseSimpleToken(String text, String tokenName, Definition.TokenDef token, int pos) {
        pos = skipWhitespace(text, pos);
        Pattern p = getCompiledPattern(token.startRegex);
        Matcher m = p.matcher(text.substring(pos));
        if (!m.lookingAt()) {
            throw new RuntimeException("Expected " + token.startRegex + " at position: " + pos);
        }
        int group = m.groupCount();
        int matchStart = m.start(group);
        int matchEnd = m.end(group);

        TokenItem item = new TokenItem(tokenName, pos + matchStart, matchEnd - matchStart);
        return item;
    }

    private TokenItem parseGroup(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        TokenItem ret = new TokenItem(tokenName, pos, 0);

        while (pos < text.length()) {
            int endTokenPos = Integer.MAX_VALUE;
            pos = skipWhitespace(text, pos);

            int closestChildTokenPos = Integer.MAX_VALUE;
            String closestChildTokenName = null;

            if (!token.searchParentEndTokenLast && parentRegex != null) {
                Pattern p = getCompiledPattern(parentRegex);
                Matcher m = p.matcher(text.substring(pos));
                if (m.find()) {
                    int group = m.groupCount();
                    endTokenPos = m.start(group);
                }
            }

            if (token.nestedTokens != null) {
                for (String childTokenName : token.nestedTokens) {
                    Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                    Integer childPos = findToken(text, pos, childDef, definition.otherTextInside);
                    if (childPos != null && childPos < closestChildTokenPos) {
                        closestChildTokenPos = childPos;
                        closestChildTokenName = childTokenName;
                        if (closestChildTokenPos == 0) break;
                    }
                }
            }

            if (closestChildTokenPos > 0 && token.searchParentEndTokenLast && parentRegex != null) {
                Pattern p = getCompiledPattern(parentRegex);
                Matcher m = p.matcher(text.substring(pos));
                if (m.find()) {
                    int group = m.groupCount();
                    endTokenPos = m.start(group);
                }
            }

            if (endTokenPos != Integer.MAX_VALUE && endTokenPos <= closestChildTokenPos) {
                ret.length = pos + endTokenPos - ret.position;
                break;
            }

            if (closestChildTokenPos == Integer.MAX_VALUE || closestChildTokenName == null) {
                break;
            }

            if (closestChildTokenPos > 0 && !definition.tokens.get(tokenName).otherTextInside) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has illegal position!");
            }

            pos += closestChildTokenPos;
            Definition.TokenDef childDef = definition.tokens.get(closestChildTokenName);
            TokenItem child = parseToken(text, closestChildTokenName, childDef, parentRegex, pos);

            if (child.position < pos) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has illegal position!");
            }
            if (child.length <= 0) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has illegal length!");
            }

            ret.length = child.position + child.length - ret.position;
            if (ret.length <= 0) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has illegal length!");
            }

            ret.children.add(child);
            pos = child.position + child.length;
        }

        return ret;
    }

    private TokenItem parseGroupOneChildOnly(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        TokenItem ret = new TokenItem(tokenName, pos, 0);

        if (token.nestedTokens == null || token.nestedTokens.isEmpty()) {
            throw new RuntimeException("GroupOneChildOnly token type nested_tokens list is empty!");
        }

        int closestChildTokenPos = Integer.MAX_VALUE;
        String closestChildTokenName = null;

        for (String childTokenName : token.nestedTokens) {
            Definition.TokenDef childDef = definition.tokens.get(childTokenName);
            Integer childPos = findToken(text, pos, childDef, token.otherTextInside);
            if (childPos != null && childPos >= 0 && childPos < closestChildTokenPos) {
                closestChildTokenPos = childPos;
                closestChildTokenName = childTokenName;
            }
        }

        if (closestChildTokenName == null) {
            throw new RuntimeException("Search for GroupOneChildOnly token type failed. Can't find one child.");
        }

        Definition.TokenDef childDef = definition.tokens.get(closestChildTokenName);
        TokenItem child = parseToken(text, closestChildTokenName, childDef, parentRegex, pos);

        ret.position = child.position;
        ret.length = child.length;
        ret.children.add(child);

        return ret;
    }

    private TokenItem parseGroupAllChildrenInSameOrder(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        TokenItem ret = new TokenItem(tokenName, pos, 0);

        if (token.nestedTokens == null || token.nestedTokens.size() != 3) {
            throw new RuntimeException("GroupAllChildrenInSameOrder should have exactly 3 nested tokens, but " + 
                (token.nestedTokens == null ? 0 : token.nestedTokens.size()) + " were found");
        }

        String startToken = token.nestedTokens.get(0);
        String innerToken = token.nestedTokens.get(1);
        String endToken = token.nestedTokens.get(2);

        Integer startTokenPos = findToken(text, pos, definition.tokens.get(startToken), definition.otherTextInside);
        if (startTokenPos == null) {
            throw new RuntimeException("Expected " + startToken + " at position: " + pos);
        }

        TokenItem child = parseToken(text, startToken, definition.tokens.get(startToken), parentRegex, pos);
        ret.length = child.position + child.length - ret.position;
        ret.children.add(child);
        pos = child.position + child.length;

        parentRegex = definition.tokens.get(endToken).startRegex;

        int endTokenPos = 0;

        while (pos < text.length()) {
            Integer innerTokenPos = findToken(text, pos, definition.tokens.get(innerToken), definition.otherTextInside);
            Integer endTokenPosOpt = findToken(text, pos, definition.tokens.get(endToken), definition.otherTextInside);

            if (endTokenPosOpt == null) {
                throw new RuntimeException("GroupAllChildrenInSameOrder end token " + endToken + " not found");
            }
            endTokenPos = endTokenPosOpt;

            if (innerTokenPos == null || endTokenPos < innerTokenPos) {
                break;
            }

            if (endTokenPos == innerTokenPos && !token.searchParentEndTokenLast) {
                break;
            }

            pos += innerTokenPos;
            child = parseToken(text, innerToken, definition.tokens.get(innerToken), parentRegex, pos);

            ret.length = child.position + child.length - ret.position;
            ret.children.add(child);
            pos = child.position + child.length;
        }

        pos += endTokenPos;
        TokenItem endItem = parseToken(text, endToken, definition.tokens.get(endToken), parentRegex, pos);

        ret.length = endItem.position + endItem.length - ret.position;
        ret.children.add(endItem);

        return ret;
    }

    private TokenItem parseStartStop(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos, boolean endRequired) {
        String myEndRegex = token.endRegex;
        pos = skipWhitespace(text, pos);

        Pattern startP = getCompiledPattern(token.startRegex);
        Matcher startM = startP.matcher(text.substring(pos));
        if (!startM.lookingAt()) {
            throw new RuntimeException("Expected " + token.startRegex + " at position: " + pos);
        }
        int startGroup = startM.groupCount();
        int startMatchStart = startM.start(startGroup);
        int startMatchEnd = startM.end(startGroup);

        pos += startMatchStart;

        TokenItem ret = new TokenItem(tokenName, pos, 0);
        pos += (startMatchEnd - startMatchStart);

        if (token.nestedTokens == null) {
            Pattern endP = getCompiledPattern(myEndRegex);
            Matcher endM = endP.matcher(text.substring(pos));
            if (!endM.find()) {
                throw new RuntimeException("Expected " + myEndRegex + " at position: " + pos);
            }
            int endGroup = endM.groupCount();
            int endTokenPos = endM.start(endGroup);
            int endTokenLength = endM.end(endGroup) - endTokenPos;
            ret.length = pos + endTokenPos + endTokenLength - ret.position;
            return ret;
        }

        while (pos <= text.length()) {
            int endTokenPos = Integer.MAX_VALUE;
            int endTokenLength = 0;
            pos = skipWhitespace(text, pos);

            String checkRegex = (token.searchParentEndTokenLast && parentRegex != null) ? parentRegex : myEndRegex;

            if (!token.searchParentEndTokenLast && checkRegex != null) {
                Pattern endP = getCompiledPattern(checkRegex);
                Matcher endM = endP.matcher(text.substring(pos));
                if (endM.find()) {
                    int endGroup = endM.groupCount();
                    endTokenPos = endM.start(endGroup);
                    endTokenLength = endM.end(endGroup) - endTokenPos;
                    if (endTokenPos == 0) {
                        ret.length = pos - ret.position + endTokenLength;
                        break;
                    }
                }
            }

            int closestChildTokenPos = Integer.MAX_VALUE;
            String closestChildTokenName = null;

            for (String childTokenName : token.nestedTokens) {
                Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                Integer childPos = findToken(text, pos, childDef, token.otherTextInside);
                if (childPos != null && childPos < closestChildTokenPos) {
                    closestChildTokenPos = childPos;
                    closestChildTokenName = childTokenName;
                    if (closestChildTokenPos == 0) break;
                }
            }

            if (token.searchParentEndTokenLast && checkRegex != null) {
                Pattern endP = getCompiledPattern(checkRegex);
                Matcher endM = endP.matcher(text.substring(pos));
                if (endM.find()) {
                    int endGroup = endM.groupCount();
                    endTokenPos = endM.start(endGroup);
                    endTokenLength = endM.end(endGroup) - endTokenPos;
                    if (endTokenPos < closestChildTokenPos) {
                        ret.length = pos - ret.position + endTokenPos + endTokenLength;
                        break;
                    }
                }
            }

            if (endTokenPos < closestChildTokenPos) {
                ret.length = pos - ret.position + endTokenPos + endTokenLength;
                break;
            }

            if (closestChildTokenPos == Integer.MAX_VALUE || closestChildTokenName == null) {
                if (endRequired && checkRegex != null) {
                    Pattern endP = getCompiledPattern(checkRegex);
                    Matcher endM = endP.matcher(text.substring(pos));
                    if (endM.find()) {
                        int endGroup = endM.groupCount();
                        endTokenPos = endM.start(endGroup);
                        endTokenLength = endM.end(endGroup) - endTokenPos;
                        ret.length = pos - ret.position + endTokenPos + endTokenLength;
                    }
                }
                break;
            }

            if (closestChildTokenPos > 0 && !definition.tokens.get(tokenName).otherTextInside) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has illegal position!");
            }

            pos += closestChildTokenPos;
            Definition.TokenDef childDef = definition.tokens.get(closestChildTokenName);
            TokenItem child = parseToken(text, closestChildTokenName, childDef, myEndRegex, pos);

            if (child.length == 0) {
                throw new RuntimeException("Child token " + closestChildTokenName + " has no length!");
            }

            ret.length = child.position + child.length - ret.position;
            ret.children.add(child);
            pos = child.position + child.length;
        }

        return ret;
    }

    private TokenItem parseToken(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        return switch (token.type) {
            case "Group" -> parseGroup(text, tokenName, token, parentRegex, pos);
            case "GroupOneChildOnly" -> parseGroupOneChildOnly(text, tokenName, token, parentRegex, pos);
            case "GroupAllChildrenInSameOrder" -> parseGroupAllChildrenInSameOrder(text, tokenName, token, parentRegex, pos);
            case "SimpleToken" -> parseSimpleToken(text, tokenName, token, pos);
            case "StartStop" -> parseStartStop(text, tokenName, token, parentRegex, pos, true);
            case "StartOptStop" -> parseStartStop(text, tokenName, token, parentRegex, pos, false);
            default -> throw new RuntimeException("Unknown token type: " + token.type);
        };
    }

    private void maybeMergeSign(TokenItem tokenItem) {
        if (definition.mergeSignIntoNumber == null) return;
        if (tokenItem.children == null || tokenItem.children.isEmpty()) return;

        List<String> signTokens = definition.mergeSignIntoNumber.signTokens;
        List<String> numberTokens = definition.mergeSignIntoNumber.numberTokens;
        List<String> operandTokens = definition.mergeSignIntoNumber.operandTokens;

        int i = 0;
        while (i < tokenItem.children.size()) {
            TokenItem curr = tokenItem.children.get(i);
            if (numberTokens.contains(curr.id) && i > 0) {
                TokenItem prev = tokenItem.children.get(i - 1);
                TokenItem sign = null;
                TokenItem context = null;
                int signKind = 0;

                if (signTokens.contains(prev.id)) {
                    sign = prev;
                    context = (i >= 2) ? tokenItem.children.get(i - 2) : null;
                    signKind = 1;
                } else if (prev.children != null && !prev.children.isEmpty() &&
                           signTokens.contains(prev.children.get(prev.children.size() - 1).id)) {
                    sign = prev.children.get(prev.children.size() - 1);
                    if (prev.children.size() >= 2) {
                        context = prev.children.get(prev.children.size() - 2);
                    } else if (i >= 2) {
                        context = tokenItem.children.get(i - 2);
                    } else {
                        context = null;
                    }
                    signKind = 2;
                }

                if (sign != null && (sign.position + sign.length == curr.position) && sign.length == 1) {
                    if (context == null || !operandTokens.contains(context.id)) {
                        curr.length += (curr.position - sign.position);
                        curr.position = sign.position;

                        if (signKind == 1) {
                            tokenItem.children.remove(i - 1);
                            i--;
                        } else {
                            prev.children.remove(prev.children.size() - 1);
                            if (prev.children.isEmpty()) {
                                tokenItem.children.remove(i - 1);
                                i--;
                            }
                        }
                    }
                }
            }

            if (curr.children != null && !curr.children.isEmpty()) {
                maybeMergeSign(curr);
            }
            i++;
        }
    }

    public void postProcess(List<TokenItem> tokens) {
        if (tokens == null) return;
        int i = 0;
        while (i < tokens.size()) {
            TokenItem curr = tokens.get(i);
            if (curr.children != null && !curr.children.isEmpty()) {
                postProcess(curr.children);
            }

            Definition.TokenDef tokenDef = definition.tokens.get(curr.id);
            if (tokenDef != null && tokenDef.deleteIfOnlyOneChild && curr.children != null && curr.children.size() == 1) {
                TokenItem onlyChild = curr.children.get(0);
                tokens.set(i, onlyChild);
                curr = onlyChild;
            }
            i++;
        }
    }

    public List<TokenItem> parse(String text) {
        List<TokenItem> tokens = new ArrayList<>();
        int pos = 0;

        while (pos < text.length()) {
            pos = skipWhitespace(text, pos);
            int closestTokenPos = Integer.MAX_VALUE;
            String closestTokenName = null;

            for (String tokenName : definition.startTokens) {
                Definition.TokenDef tokenDef = definition.tokens.get(tokenName);
                Integer offset = findToken(text, pos, tokenDef, definition.otherTextInside);
                if (offset != null && offset < closestTokenPos) {
                    closestTokenPos = offset;
                    closestTokenName = tokenName;
                }
            }

            if (closestTokenName == null) {
                if (definition.otherTextInside && pos < text.length()) {
                    tokens.add(new TokenItem("", pos, text.length() - pos));
                }
                break;
            }

            if (closestTokenPos > 0 && definition.otherTextInside) {
                int errEnd = pos + closestTokenPos;
                while (errEnd > pos && Character.isWhitespace(text.charAt(errEnd - 1))) {
                    errEnd--;
                }
                if (errEnd > pos) {
                    tokens.add(new TokenItem("", pos, errEnd - pos));
                }
            }

            pos += closestTokenPos;
            Definition.TokenDef tokenDef = definition.tokens.get(closestTokenName);
            TokenItem child = parseToken(text, closestTokenName, tokenDef, null, pos);

            if (child.length == 0) {
                throw new RuntimeException("Child token " + closestTokenName + " has no length!");
            }

            tokens.add(child);
            pos = child.position + child.length;
        }

        if (definition.mergeSignIntoNumber != null) {
            TokenItem dummyRoot = new TokenItem("ROOT", 0, 0);
            dummyRoot.children = tokens;
            maybeMergeSign(dummyRoot);
            tokens = dummyRoot.children;
        }

        postProcess(tokens);

        return tokens;
    }

    private void recursiveFormat(byte[] array, TokenItem token, List<String> tokenKeys) {
        int tokenId = tokenKeys.indexOf(token.id);
        if (tokenId >= 0) {
            byte charByte = (byte) tokenId;
            int end = Math.min(array.length, token.position + token.length);
            for (int k = token.position; k < end; k++) {
                if (k >= 0) {
                    array[k] = charByte;
                }
            }
        }

        if (token.children != null) {
            for (TokenItem child : token.children) {
                recursiveFormat(array, child, tokenKeys);
            }
        }
    }

    public byte[] parseFormat(String text) {
        byte[] ret = new byte[text.length()];
        List<TokenItem> tokens = parse(text);
        List<String> tokenKeys = new ArrayList<>(definition.tokens.keySet());

        for (TokenItem token : tokens) {
            recursiveFormat(ret, token, tokenKeys);
        }

        return ret;
    }
}
