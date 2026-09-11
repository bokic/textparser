package com.textparser;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class TextParser {
    private static final Gson GSON = new GsonBuilder().create();

    public Definition definition;
    private final Map<String, Pattern> patternCache = new HashMap<>();
    private String currentText;

    public TextParser(Definition definition) {
        this.definition = definition;
        initializeDefaults();
    }

    public TextParser(String definitionJson) {
        if (definitionJson == null || definitionJson.trim().isEmpty()) {
            throw new IllegalArgumentException("Definition JSON cannot be empty");
        }
        this.definition = GSON.fromJson(definitionJson, Definition.class);
        initializeDefaults();
    }

    public static TextParser fromFile(String filePath) throws IOException {
        String content = Files.readString(Path.of(filePath));
        return new TextParser(content);
    }

    private void initializeDefaults() {
        if (definition == null) return;

        // C's JSON loader supports "formatVersion: 2" definitions that carry a
        // contextual `lexer` instead of a top-level `tokens` map. The CLI path
        // normalises the lexer tokens and trivia into plain SimpleToken entries
        // and generates `startTokens` from all of them; the grammar engine is
        // only reachable through the separate execute_language_grammar() API.
        if ((definition.tokens == null || definition.tokens.isEmpty()) && definition.lexer != null) {
            Map<String, Definition.TokenDef> normalized = new LinkedHashMap<>();
            addLexerTokens(normalized, definition.lexer.tokens);
            addLexerTokens(normalized, definition.lexer.trivia);
            definition.tokens = normalized;
        }

        if (definition.tokens != null) {
            for (Map.Entry<String, Definition.TokenDef> entry : definition.tokens.entrySet()) {
                Definition.TokenDef token = entry.getValue();
                if (token.id == null) {
                    token.id = entry.getKey();
                }
            }
        }

        if ((definition.startTokens == null || definition.startTokens.isEmpty()) &&
            definition.tokens != null && !definition.tokens.isEmpty()) {
            definition.startTokens = new ArrayList<>(definition.tokens.keySet());
        }
    }

    private void addLexerTokens(Map<String, Definition.TokenDef> out, Map<String, Definition.LexerToken> src) {
        if (src == null) return;
        for (Map.Entry<String, Definition.LexerToken> entry : src.entrySet()) {
            Definition.LexerToken lex = entry.getValue();
            Definition.TokenDef token = new Definition.TokenDef();
            token.id = entry.getKey();
            token.type = Definition.TokenType.SimpleToken;
            if (lex != null) {
                token.startRegex = lex.regex;
                token.textColor = lex.textColor;
                token.multiLine = Boolean.TRUE.equals(lex.multiLine);
            }
            out.put(entry.getKey(), token);
        }
    }

    private int regexFlags() {
        // C compiles patterns with PCRE2_CASELESS only when the definition is
        // case-insensitive; PCRE2_DOTALL is never enabled.
        return definition.caseSensitivity ? 0 : Pattern.CASE_INSENSITIVE;
    }

    /**
     * Rewrite PCRE2 Unicode derived properties that java.util.regex does not
     * know into equivalent Unicode category sets. All occurrences in the shipped
     * definitions appear inside character classes, so concatenation is safe.
     */
    private String normalizeRegex(String regex) {
        if (regex == null) return null;
        if (regex.indexOf("\\p{ID_Start}") < 0 && regex.indexOf("\\p{ID_Continue}") < 0) {
            return regex;
        }
        return regex.replace("\\p{ID_Start}", "\\p{L}\\p{Nl}")
                    .replace("\\p{ID_Continue}", "\\p{L}\\p{Nl}\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}");
    }

    private Pattern getCompiledPattern(String regexStr) {
        if (regexStr == null) return null;
        return patternCache.computeIfAbsent(regexStr, r -> {
            String normalized = normalizeRegex(r);
            try {
                return Pattern.compile(normalized, regexFlags());
            } catch (Exception e) {
                return Pattern.compile(Pattern.quote(normalized), regexFlags());
            }
        });
    }

    private Matcher matcherFrom(String text, Pattern pattern, int pos) {
        Matcher m = pattern.matcher(text);
        if (pos > 0) {
            m.region(Math.min(pos, text.length()), text.length());
        }
        return m;
    }

    private int getSearchLen(String text, int pos, Definition.TokenDef token) {
        int len = text.length() - pos;
        if (token != null && !token.multiLine) {
            int lineLen = 0;
            while (pos + lineLen < text.length()) {
                char c = text.charAt(pos + lineLen);
                if (c == '\n' || c == '\r') {
                    break;
                }
                lineLen++;
            }
            if (lineLen < len) {
                len = lineLen;
            }
        }
        return len;
    }

    private int skipWhitespace(String text, int pos) {
        while (pos < text.length()) {
            char ch = text.charAt(pos);
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                pos++;
            } else {
                break;
            }
        }
        return pos;
    }

    private boolean appendUnprocessed(List<TokenItem> list, String text, int start, int len) {
        if (len <= 0) return false;
        boolean hadWhitespace = false;
        int curr = start;
        int end = start + len;
        while (curr < end) {
            while (curr < end && Character.isWhitespace(text.charAt(curr))) {
                curr++;
                hadWhitespace = true;
            }
            int wordStart = curr;
            while (curr < end && !Character.isWhitespace(text.charAt(curr))) {
                curr++;
            }
            if (curr > wordStart) {
                int wordLen = curr - wordStart;
                if (!list.isEmpty()) {
                    TokenItem last = list.get(list.size() - 1);
                    if (TokenItem.UNPROCESSED.equals(last.id) && last.position + last.length == wordStart) {
                        last.length += wordLen;
                        continue;
                    }
                }
                list.add(new TokenItem(TokenItem.UNPROCESSED, wordStart, wordLen));
            }
        }
        return hadWhitespace;
    }

    private void appendUnprocessedSpan(List<TokenItem> list, int start, int len) {
        if (len <= 0) return;
        if (!list.isEmpty()) {
            TokenItem last = list.get(list.size() - 1);
            if (TokenItem.UNPROCESSED.equals(last.id) && last.position + last.length == start) {
                last.length += len;
                return;
            }
        }
        list.add(new TokenItem(TokenItem.UNPROCESSED, start, len));
    }

    private void pruneDelimiters(TokenItem ret, Definition.TokenDef token, boolean hasWhitespace) {
        if (token.hasCustomDelimiterStyling()) return;
        if (hasWhitespace) return;
        List<TokenItem> ch = ret.children;
        if (ch.size() == 3 &&
            TokenItem.START_DELIMITER.equals(ch.get(0).id) &&
            TokenItem.UNPROCESSED.equals(ch.get(1).id) &&
            TokenItem.END_DELIMITER.equals(ch.get(2).id)) {
            ret.children.clear();
        } else if (ch.size() == 2 &&
                   TokenItem.START_DELIMITER.equals(ch.get(0).id) &&
                   TokenItem.END_DELIMITER.equals(ch.get(1).id)) {
            ret.children.clear();
        } else if (ch.size() == 1 &&
                   TokenItem.UNPROCESSED.equals(ch.get(0).id) &&
                   ch.get(0).length == ret.length) {
            ret.children.clear();
        }
    }

    /**
     * Mirror of C's {@code textparser_find_token}: the pattern is anchored at
     * {@code pos} (PCRE2_ANCHORED) and the returned offset is the first capture
     * group's offset relative to {@code pos}. Returns {@code null} when the
     * token does not match at {@code pos}.
     */
    private Integer findToken(String text, int pos, Definition.TokenDef token, boolean otherTextInside) {
        if (token == null || token.type == null) return null;
        if (pos >= text.length()) return null;
        switch (token.type) {
            case Group, GroupOneChildOnly -> {
                if (token.nestedTokens == null) return null;
                int closestChildPos = Integer.MAX_VALUE;
                for (String childTokenName : token.nestedTokens) {
                    Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                    Integer childPos = findToken(text, pos, childDef, otherTextInside);
                    if (childPos != null && childPos < closestChildPos) {
                        closestChildPos = childPos;
                        if (closestChildPos == 0) break;
                    }
                }
                return closestChildPos == Integer.MAX_VALUE ? null : closestChildPos;
            }
            case GroupAllChildrenInSameOrder, Sequence -> {
                if (token.nestedTokens == null || token.nestedTokens.isEmpty()) return null;
                Definition.TokenDef childDef = definition.tokens.get(token.nestedTokens.get(0));
                return findToken(text, pos, childDef, otherTextInside);
            }
            case SimpleToken, StartStop, StartOptStop -> {
                if (token.startRegex == null) return null;
                // C bounds find_token to the current line for non-multiLine
                // tokens (textparser_get_search_len). Besides matching C's
                // semantics this keeps Java's recursive regex matcher from
                // walking across newlines and blowing the stack.
                int searchLen = getSearchLen(text, pos, token);
                if (searchLen <= 0) return null;
                Pattern p = getCompiledPattern(token.startRegex);
                Matcher m = p.matcher(text);
                m.region(pos, pos + searchLen);
                if (!m.lookingAt()) return null;

                int group = m.groupCount();
                int matchStart = m.start(group);
                int matchEnd = m.end(group);

                if (matchEnd - matchStart == 0) return null;

                return matchStart - pos;
            }
            default -> throw new RuntimeException("Unknown token type: " + token.type);
        }
    }

    private boolean matchesAt(String text, int pos, String regex) {
        if (regex == null || pos >= text.length()) return false;
        Matcher m = matcherFrom(text, getCompiledPattern(regex), pos);
        return m.lookingAt();
    }

    private int[] matchEndToken(String text, int pos, String regex, boolean onlyAtStart, int searchLen) {
        if (regex == null) return null;
        int end = Math.min(text.length(), pos + Math.max(0, searchLen));
        if (end <= pos) return null;
        Pattern p = getCompiledPattern(regex);
        Matcher m = p.matcher(text);
        m.region(pos, end);
        boolean matched = onlyAtStart ? m.lookingAt() : m.find();
        if (!matched) return null;
        int group = m.groupCount();
        int s = m.start(group);
        int e = m.end(group);
        return new int[]{s - pos, e - s};
    }

    private TokenItem parseSimpleToken(String text, String tokenName, Definition.TokenDef token, int pos) {
        pos = skipWhitespace(text, pos);
        Pattern p = getCompiledPattern(token.startRegex);
        Matcher m = matcherFrom(text, p, pos);
        if (!m.lookingAt()) {
            throw new RuntimeException("Expected " + token.startRegex + " at position: " + pos);
        }
        int group = m.groupCount();
        int matchStart = m.start(group);
        int matchEnd = m.end(group);

        TokenItem item = new TokenItem(tokenName, matchStart, matchEnd - matchStart);
        return item;
    }

    private TokenItem parseGroup(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        if (token.nestedTokens == null || token.nestedTokens.isEmpty()) {
            throw new RuntimeException("nested_tokens list is empty!");
        }

        int startOffset = pos;
        TokenItem ret = new TokenItem(tokenName, pos, 0);

        while (true) {
            pos = skipWhitespace(text, pos);

            if (pos >= text.length()) {
                if (!ret.children.isEmpty()) {
                    ret.length = pos - startOffset;
                    return ret;
                }
                throw new RuntimeException("Search for group token type failed. Can't find any child.");
            }

            if (matchesAt(text, pos, parentRegex)) {
                ret.length = pos - startOffset;
                break;
            }

            TokenItem newChild = null;
            RuntimeException firstError = null;
            for (String childTokenName : token.nestedTokens) {
                Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                Integer found = findToken(text, pos, childDef, token.otherTextInside);
                if (found != null && found == 0) {
                    try {
                        TokenItem attempt = parseToken(text, childTokenName, childDef, parentRegex, pos);
                        if (attempt != null && attempt.length > 0) {
                            newChild = attempt;
                            break;
                        }
                    } catch (RuntimeException e) {
                        if (firstError == null) {
                            firstError = e;
                        }
                    }
                }
            }

            if (newChild != null) {
                ret.children.add(newChild);
                pos = newChild.position + newChild.length;
                ret.length = pos - startOffset;
            } else if (token.otherTextInside) {
                appendUnprocessedSpan(ret.children, pos, 1);
                pos += 1;
            } else {
                if (!ret.children.isEmpty()) {
                    ret.length = pos - startOffset;
                    return ret;
                }
                if (firstError != null) {
                    throw firstError;
                }
                throw new RuntimeException("Unrecognized token inside group");
            }
        }

        return ret;
    }

    private TokenItem parseGroupOneChildOnly(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        if (token.nestedTokens == null || token.nestedTokens.isEmpty()) {
            throw new RuntimeException("group_one_child token type nested_tokens list is empty!");
        }

        int startOffset = pos;
        TokenItem ret = new TokenItem(tokenName, pos, 0);
        TokenItem child = null;

        for (String childTokenName : token.nestedTokens) {
            Definition.TokenDef childDef = definition.tokens.get(childTokenName);
            Integer found = findToken(text, pos, childDef, token.otherTextInside);
            if (found != null && found == 0) {
                try {
                    TokenItem attempt = parseToken(text, childTokenName, childDef, parentRegex, pos);
                    if (attempt != null && attempt.length > 0) {
                        child = attempt;
                        break;
                    }
                } catch (RuntimeException e) {
                    // speculative candidate; try the next one
                }
            }
        }

        if (child == null) {
            int closest = Integer.MAX_VALUE;
            String closestName = null;
            for (String childTokenName : token.nestedTokens) {
                Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                Integer found = findToken(text, pos, childDef, token.otherTextInside);
                if (found != null && found > 0 && found < closest) {
                    closest = found;
                    closestName = childTokenName;
                }
            }

            if (closestName == null) {
                throw new RuntimeException("Search for group_one_child token type failed. Can't find one child.");
            }

            appendUnprocessedSpan(ret.children, pos, closest);
            pos += closest;
            child = parseToken(text, closestName, definition.tokens.get(closestName), parentRegex, pos);
            if (child == null) {
                throw new RuntimeException("Search for group_one_child token type failed. Child token parsing failed.");
            }
        }

        ret.children.add(child);
        ret.position = startOffset;
        ret.length = child.position + child.length - startOffset;

        return ret;
    }

    private TokenItem parseGroupAllChildrenInSameOrder(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        pos = skipWhitespace(text, pos);
        if (token.nestedTokens == null || token.nestedTokens.size() != 3) {
            throw new RuntimeException("GroupAllChildrenInSameOrder should have exactly 3 nested tokens, but " +
                (token.nestedTokens == null ? 0 : token.nestedTokens.size()) + " were found");
        }

        String startToken = token.nestedTokens.get(0);
        String innerToken = token.nestedTokens.get(1);
        String endToken = token.nestedTokens.get(2);

        int startOffset = pos;
        TokenItem ret = new TokenItem(tokenName, pos, 0);

        Integer startFound = findToken(text, pos, definition.tokens.get(startToken), definition.otherTextInside);
        if (startFound == null || startFound != 0) {
            throw new RuntimeException("Expected start token!");
        }

        TokenItem child = parseToken(text, startToken, definition.tokens.get(startToken), parentRegex, pos);
        if (child == null) {
            throw new RuntimeException("Parsing start token failed");
        }
        ret.children.add(child);
        pos = child.position + child.length;

        String innerParentRegex = definition.tokens.get(endToken).startRegex;

        while (true) {
            pos = skipWhitespace(text, pos);
            if (pos >= text.length()) {
                throw new RuntimeException("Expected end token, reached end of text!");
            }

            Integer endFound = findToken(text, pos, definition.tokens.get(endToken), definition.otherTextInside);
            if (endFound != null && endFound == 0) {
                break;
            }

            Integer innerFound = findToken(text, pos, definition.tokens.get(innerToken), definition.otherTextInside);
            if (innerFound != null && innerFound == 0) {
                TokenItem inner = parseToken(text, innerToken, definition.tokens.get(innerToken), innerParentRegex, pos);
                if (inner == null) {
                    throw new RuntimeException("Parsing inner token failed");
                }
                ret.children.add(inner);
                pos = inner.position + inner.length;
                continue;
            }

            if (token.otherTextInside) {
                appendUnprocessedSpan(ret.children, pos, 1);
                pos += 1;
            } else {
                throw new RuntimeException("Expected inner or end token!");
            }
        }

        pos = skipWhitespace(text, pos);
        TokenItem endItem = parseToken(text, endToken, definition.tokens.get(endToken), parentRegex, pos);
        if (endItem == null) {
            throw new RuntimeException("Parsing end token failed");
        }
        ret.children.add(endItem);
        pos = endItem.position + endItem.length;
        ret.length = pos - startOffset;

        return ret;
    }

    private TokenItem parseStartStop(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos, boolean endRequired) {
        String myEndRegex = token.endRegex;
        pos = skipWhitespace(text, pos);
        int startOffset = pos;

        Pattern startP = getCompiledPattern(token.startRegex);
        Matcher startM = matcherFrom(text, startP, pos);
        if (!startM.lookingAt()) {
            throw new RuntimeException("Expected " + token.startRegex + " at position: " + pos);
        }
        int startGroup = startM.groupCount();
        int startMatchStart = startM.start(startGroup);
        int startMatchEnd = startM.end(startGroup);

        pos = startMatchStart;
        int startDelimiterLength = startMatchEnd - startMatchStart;

        TokenItem ret = new TokenItem(tokenName, pos, 0);
        boolean hasWhitespace = false;
        if (startDelimiterLength > 0) {
            ret.children.add(new TokenItem(TokenItem.START_DELIMITER, ret.position, startDelimiterLength));
        }
        pos += startDelimiterLength;

        if (pos > text.length()) {
            throw new RuntimeException("offset >= total units count!");
        }
        if (pos == text.length()) {
            if (endRequired) {
                throw new RuntimeException("reached end of text!");
            }
            ret.length = pos - startOffset;
            return ret;
        }

        List<String> nested = token.nestedTokens;
        if (nested != null && !nested.isEmpty()) {
            while (true) {
                int wsStart = pos;
                pos = skipWhitespace(text, pos);
                if (pos > wsStart) {
                    hasWhitespace = true;
                }

                if (!token.searchParentEndTokenLast && matchesAt(text, pos, myEndRegex)) {
                    break;
                }

                if (pos >= text.length()) {
                    if (token.searchParentEndTokenLast && matchesAt(text, pos, myEndRegex)) {
                        break;
                    }
                    if (endRequired) {
                        throw new RuntimeException("Reached end of text before finding end token!");
                    }
                    break;
                }

                TokenItem newChild = null;
                RuntimeException firstError = null;
                for (String childTokenName : nested) {
                    Definition.TokenDef childDef = definition.tokens.get(childTokenName);
                    Integer found = findToken(text, pos, childDef, token.otherTextInside);
                    if (found != null && found == 0) {
                        try {
                            TokenItem attempt = parseToken(text, childTokenName, childDef, myEndRegex, pos);
                            if (attempt != null && attempt.length > 0) {
                                newChild = attempt;
                                break;
                            }
                        } catch (RuntimeException e) {
                            if (firstError == null) {
                                firstError = e;
                            }
                        }
                    }
                }

                if (newChild != null) {
                    ret.length = newChild.position + newChild.length - ret.position;
                    ret.children.add(newChild);
                    if (newChild.length == 0) {
                        throw new RuntimeException("0-length child token match caused infinite loop");
                    }
                    pos = newChild.position + newChild.length;
                    continue;
                }

                if (token.searchParentEndTokenLast && matchesAt(text, pos, myEndRegex)) {
                    break;
                }

                if (token.otherTextInside) {
                    // C clears any speculative nested-attempt error here and consumes
                    // a single character as Unprocessed before retrying.
                    appendUnprocessedSpan(ret.children, pos, 1);
                    pos += 1;
                } else {
                    if (firstError != null) {
                        throw firstError;
                    }
                    throw new RuntimeException("Unexpected token inside start-stop block!");
                }
            }
        }

        int wsStart = pos;
        pos = skipWhitespace(text, pos);
        if (pos > wsStart) {
            hasWhitespace = true;
        }

        boolean endOnlyAtStart = false;
        int endSearchLen = text.length() - pos;
        if (!token.otherTextInside && nested != null && !nested.isEmpty()) {
            endOnlyAtStart = true;
            endSearchLen = getSearchLen(text, pos, token);
        } else if (token.otherTextInside && !token.multiLine) {
            endSearchLen = getSearchLen(text, pos, token);
        }

        int[] endMatch = matchEndToken(text, pos, myEndRegex, endOnlyAtStart, endSearchLen);
        if (endMatch == null && token.otherTextInside && !token.multiLine) {
            endMatch = matchEndToken(text, pos, myEndRegex, false, text.length() - pos);
        }
        if (endMatch == null) {
            if (endRequired) {
                throw new RuntimeException("Can't find end of the token!");
            }
            ret.length = pos - startOffset;
            return ret;
        }

        int tokenEnd = endMatch[0];
        int endLen = endMatch[1];
        if (tokenEnd > 0) {
            appendUnprocessedSpan(ret.children, pos, tokenEnd);
        }
        if (endLen > 0) {
            ret.children.add(new TokenItem(TokenItem.END_DELIMITER, pos + tokenEnd, endLen));
        }
        pos += tokenEnd + endLen;
        ret.length = pos - startOffset;

        pruneDelimiters(ret, token, hasWhitespace);
        return ret;
    }

    private TokenItem parseSequence(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        int startOffset = pos;
        if (token.nestedTokens == null || token.nestedTokens.isEmpty()) {
            return null;
        }

        TokenItem ret = new TokenItem(tokenName, pos, 0);

        for (String elemName : token.nestedTokens) {
            pos = skipWhitespace(text, pos);
            if (pos >= text.length()) {
                return null;
            }

            Definition.TokenDef elemDef = definition.tokens.get(elemName);
            if (elemDef == null) {
                return null;
            }

            Integer found = findToken(text, pos, elemDef, token.otherTextInside);
            if (found == null || found != 0) {
                return null;
            }

            TokenItem elemItem;
            try {
                elemItem = parseToken(text, elemName, elemDef, parentRegex, pos);
            } catch (RuntimeException e) {
                return null;
            }
            if (elemItem == null || elemItem.length == 0) {
                return null;
            }

            ret.children.add(elemItem);
            pos = elemItem.position + elemItem.length;
        }

        ret.length = pos - startOffset;
        return ret;
    }

    private boolean hasNewline(String text, int start, int len) {
        int end = Math.min(text.length(), start + len);
        for (int i = Math.max(0, start); i < end; i++) {
            char c = text.charAt(i);
            if (c == '\n' || c == '\r') {
                return true;
            }
        }
        return false;
    }

    private TokenItem parseToken(String text, String tokenName, Definition.TokenDef token, String parentRegex, int pos) {
        int startPos = pos;
        pos = skipWhitespace(text, pos);
        TokenItem result = switch (token.type) {
            case Group -> parseGroup(text, tokenName, token, parentRegex, pos);
            case GroupOneChildOnly -> parseGroupOneChildOnly(text, tokenName, token, parentRegex, pos);
            case GroupAllChildrenInSameOrder -> parseGroupAllChildrenInSameOrder(text, tokenName, token, parentRegex, pos);
            case Sequence -> parseSequence(text, tokenName, token, parentRegex, pos);
            case SimpleToken -> parseSimpleToken(text, tokenName, token, pos);
            case StartStop -> parseStartStop(text, tokenName, token, parentRegex, pos, true);
            case StartOptStop -> parseStartStop(text, tokenName, token, parentRegex, pos, false);
            default -> throw new RuntimeException("Unknown token type: " + token.type);
        };
        if (result != null && !token.multiLine && hasNewline(text, startPos, result.length)) {
            throw new RuntimeException("Token spans multiple lines but multi_line flag is not set!");
        }
        return result;
    }

    private boolean isPlusMinusAt(int pos) {
        if (currentText == null || pos < 0 || pos >= currentText.length()) return false;
        char c = currentText.charAt(pos);
        return c == '+' || c == '-';
    }

    private boolean isTriviaToken(String id) {
        return TokenItem.UNPROCESSED.equals(id) ||
               TokenItem.WHITESPACE.equals(id) ||
               TokenItem.START_DELIMITER.equals(id) ||
               TokenItem.END_DELIMITER.equals(id);
    }

    private TokenItem nearestNonTriviaBefore(List<TokenItem> list, int index) {
        for (int k = index - 1; k >= 0; k--) {
            TokenItem t = list.get(k);
            if (!isTriviaToken(t.id)) {
                return t;
            }
        }
        return null;
    }

    private void maybeMergeSign(TokenItem tokenItem) {
        if (definition.mergeSignIntoNumber == null) return;
        if (tokenItem.children == null || tokenItem.children.isEmpty()) return;

        List<String> signTokens = definition.mergeSignIntoNumber.signTokens;
        List<String> numberTokens = definition.mergeSignIntoNumber.numberTokens;
        List<String> operandTokens = definition.mergeSignIntoNumber.operandTokens;

        int i = 0;
        while (i < tokenItem.children.size()) {
            boolean isNum = numberTokens.contains(tokenItem.children.get(i).id);
            if (isNum && i > 0) {
                int[] signOpt = null;
                TokenItem prev = tokenItem.children.get(i - 1);
                // C only treats a literal single '+'/'-' as a sign; never absorb
                // longer or other operators (e.g. ">&2" or "!3").
                if (signTokens.contains(prev.id) && prev.length == 1 && isPlusMinusAt(prev.position)) {
                    TokenItem context = nearestNonTriviaBefore(tokenItem.children, i - 1);
                    boolean isOperandCtx = context != null && operandTokens.contains(context.id);
                    if (!isOperandCtx) {
                        signOpt = new int[]{prev.position, prev.length, 1};
                    }
                } else if (prev.children != null && !prev.children.isEmpty() && signTokens.contains(prev.children.get(prev.children.size() - 1).id)) {
                    TokenItem lastSign = prev.children.get(prev.children.size() - 1);
                    if (lastSign.length == 1 && isPlusMinusAt(lastSign.position)) {
                        TokenItem context = nearestNonTriviaBefore(prev.children, prev.children.size() - 1);
                        if (context == null) {
                            context = nearestNonTriviaBefore(tokenItem.children, i - 1);
                        }
                        boolean isOperandCtx = context != null && operandTokens.contains(context.id);
                        if (!isOperandCtx) {
                            signOpt = new int[]{lastSign.position, lastSign.length, 2};
                        }
                    }
                }

                if (signOpt != null) {
                    int signPos = signOpt[0];
                    int signLen = signOpt[1];
                    int mode = signOpt[2];

                    int numPos = tokenItem.children.get(i).position;
                    int numLen = tokenItem.children.get(i).length;

                    boolean adjacent = (signPos + signLen) == numPos;
                    if (adjacent) {
                        if (mode == 1) {
                            tokenItem.children.remove(i - 1);
                            i--;
                        } else if (mode == 2) {
                            tokenItem.children.get(i - 1).children.remove(tokenItem.children.get(i - 1).children.size() - 1);
                        }

                        tokenItem.children.get(i).position = signPos;
                        tokenItem.children.get(i).length = numLen + signLen;
                    }
                }
            }

            if (tokenItem.children.get(i).children != null && !tokenItem.children.get(i).children.isEmpty()) {
                maybeMergeSign(tokenItem.children.get(i));
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
        try {
            return parseInternal(text);
        } catch (StackOverflowError e) {
            // java.util.regex matches recursive loops on the call stack, so long
            // multi-line literals can exhaust the default thread stack where
            // PCRE2 does not. Retry once on a thread with a large reserved stack.
            final List<TokenItem>[] result = new List[1];
            final Throwable[] failure = new Throwable[1];
            Thread worker = new Thread(null, () -> {
                try {
                    result[0] = parseInternal(text);
                } catch (Throwable t) {
                    failure[0] = t;
                }
            }, "textparser-parse", 512L * 1024 * 1024);
            worker.start();
            try {
                worker.join();
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
                throw new RuntimeException(ie);
            }
            if (failure[0] instanceof RuntimeException re) throw re;
            if (failure[0] instanceof Error er) throw er;
            if (failure[0] != null) throw new RuntimeException(failure[0]);
            return result[0];
        }
    }

    private List<TokenItem> parseInternal(String text) {
        currentText = text;
        List<TokenItem> tokens = new ArrayList<>();
        int pos = 0;

        while (pos < text.length()) {
            pos = skipWhitespace(text, pos);
            if (pos >= text.length()) {
                break;
            }

            TokenItem child = null;
            RuntimeException firstError = null;
            for (String candName : definition.startTokens) {
                Definition.TokenDef tokenDef = definition.tokens.get(candName);
                Integer offset = findToken(text, pos, tokenDef, definition.otherTextInside);
                if (offset != null && offset == 0) {
                    try {
                        TokenItem attempt = parseToken(text, candName, tokenDef, null, pos);
                        if (attempt != null && attempt.length > 0) {
                            child = attempt;
                            break;
                        }
                    } catch (RuntimeException e) {
                        if (firstError == null) {
                            firstError = e;
                        }
                    }
                }
            }

            if (child != null) {
                tokens.add(child);
                pos = child.position + child.length;
                continue;
            }

            // C keeps the first failed candidate's error and reports it as fatal
            // when no other candidate could parse the token.
            if (firstError != null) {
                throw firstError;
            }

            if (definition.otherTextInside) {
                appendUnprocessedSpan(tokens, pos, 1);
                pos += 1;
            } else {
                break;
            }
        }

        if (pos < text.length() && !definition.otherTextInside) {
            appendUnprocessedSpan(tokens, pos, text.length() - pos);
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
        List<TokenItem> tokens = parse(text);
        byte[] array = new byte[text.length()];
        List<String> tokenKeys = new ArrayList<>(definition.tokens.keySet());

        for (TokenItem item : tokens) {
            recursiveFormat(array, item, tokenKeys);
        }

        return array;
    }
}
