package com.textparser.grammar;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

/**
 * Loads schema v2 declarative grammars, contextual lexer definitions,
 * operator tables, and recovery policies into GrammarDefinition.
 *
 * Implements full graph validation including:
 * - Direct and indirect left recursion cycle detection (3-color DFS)
 * - Nullable repeat loops detection (fixed-point iteration)
 * - Undefined token and production reference detection
 * - Desugaring of oneOrMore into sequence [item, repeat(item)]
 */
public class GrammarLoader {
    private static final Gson GSON = new Gson();

    private static final String[] CONSTRUCT_KEYS = {
        "token", "ref", "sequence", "choice", "optional", "repeat",
        "lookahead", "not", "when", "withContext", "commit", "pratt", "withGoal",
        "capture", "matchCapture", "oneOrMore"
    };

    private static final Set<String> ALLOWED_CONSTRUCT_KEYS = new HashSet<>(
        Arrays.asList(CONSTRUCT_KEYS)
    );

    static {
        ALLOWED_CONSTRUCT_KEYS.add("astKind");
        ALLOWED_CONSTRUCT_KEYS.add("expect");
        ALLOWED_CONSTRUCT_KEYS.add("recover");
        ALLOWED_CONSTRUCT_KEYS.add("recoverUntil");
        ALLOWED_CONSTRUCT_KEYS.add("allowASI");
        ALLOWED_CONSTRUCT_KEYS.add("events");
        ALLOWED_CONSTRUCT_KEYS.add("diagnostics");
        ALLOWED_CONSTRUCT_KEYS.add("category");
    }

    public static GrammarDefinition loadFromJson(String jsonString) {
        Map<String, Object> map = GSON.fromJson(
            jsonString, new TypeToken<Map<String, Object>>() {}.getType()
        );
        return load(map);
    }

    public static GrammarDefinition loadFromFile(String filePath) throws IOException {
        String content = Files.readString(Paths.get(filePath));
        return loadFromJson(content);
    }

    public static GrammarDefinition load(Map<String, Object> root) {
        if (root == null) {
            throw new GrammarException(GrammarException.GRAMMAR_NOT_OBJECT, "Root definition is null");
        }

        // 1. Build token name-to-id mapping
        Map<String, Integer> tokenIds = new LinkedHashMap<>();
        collectTokens(root, tokenIds);

        GrammarDefinition grammarDef = new GrammarDefinition();

        // 2. Parse Recovery Policy
        parseRecoveryPolicy(root, tokenIds, grammarDef);

        // 3. Parse Contextual Lexer
        parseContextualLexer(root, tokenIds, grammarDef);

        // 4. Parse Pratt Operators
        parsePrattOperators(root, tokenIds, grammarDef);

        // 5. Parse Grammar
        parseGrammar(root, tokenIds, grammarDef);

        return grammarDef;
    }

    @SuppressWarnings("unchecked")
    private static void collectTokens(Map<String, Object> root, Map<String, Integer> tokenIds) {
        Map<String, Object> tokensObj = null;
        if (root.get("tokens") instanceof Map<?, ?> t) {
            tokensObj = (Map<String, Object>) t;
        } else if (root.get("lexer") instanceof Map<?, ?> lexer) {
            if (lexer.get("tokens") instanceof Map<?, ?> lt) {
                for (Object key : lt.keySet()) {
                    if (key != null && !tokenIds.containsKey(key.toString())) {
                        tokenIds.put(key.toString(), tokenIds.size());
                    }
                }
            }
            if (lexer.get("trivia") instanceof Map<?, ?> tr) {
                for (Object key : tr.keySet()) {
                    if (key != null && !tokenIds.containsKey(key.toString())) {
                        tokenIds.put(key.toString(), tokenIds.size());
                    }
                }
            }
            return;
        }

        if (tokensObj != null) {
            for (String tokenName : tokensObj.keySet()) {
                if (tokenName != null && !tokenIds.containsKey(tokenName)) {
                    tokenIds.put(tokenName, tokenIds.size());
                }
            }
        }
    }

    @SuppressWarnings("unchecked")
    private static void parseRecoveryPolicy(Map<String, Object> root, Map<String, Integer> tokenIds, GrammarDefinition grammarDef) {
        if (!(root.get("recovery") instanceof Map<?, ?> recMap)) {
            return;
        }
        RecoveryPolicy policy = grammarDef.recoveryPolicy;
        for (Map.Entry<?, ?> entry : recMap.entrySet()) {
            String key = entry.getKey().toString();
            Object val = entry.getValue();
            switch (key) {
                case "maximumDiagnostics":
                    if (!(val instanceof Number num) || num.intValue() <= 0) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid maximumDiagnostics");
                    }
                    policy.maximumDiagnostics = num.intValue();
                    break;
                case "maximumSkippedTokens":
                    if (!(val instanceof Number num) || num.intValue() <= 0) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid maximumSkippedTokens");
                    }
                    policy.maximumSkippedTokens = num.intValue();
                    break;
                case "maximumRecoveryAttempts":
                    if (!(val instanceof Number num) || num.intValue() <= 0) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid maximumRecoveryAttempts");
                    }
                    policy.maximumRecoveryAttempts = num.intValue();
                    break;
                case "synchronizationTokens":
                    if (!(val instanceof List<?> list) || list.isEmpty()) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid synchronizationTokens");
                    }
                    int[] syncTokens = new int[list.size()];
                    for (int i = 0; i < list.size(); i++) {
                        Object item = list.get(i);
                        if (!(item instanceof String name)) {
                            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Synchronization token must be string");
                        }
                        Integer id = tokenIds.get(name);
                        if (id == null) {
                            throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined synchronization token: " + name);
                        }
                        syncTokens[i] = id;
                    }
                    policy.synchronizationTokens = syncTokens;
                    break;
                default:
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unknown recovery policy key: " + key);
            }
        }
    }

    @SuppressWarnings("unchecked")
    private static void parseContextualLexer(Map<String, Object> root, Map<String, Integer> tokenIds, GrammarDefinition grammarDef) {
        if (!(root.get("lexer") instanceof Map<?, ?> lexerMap)) {
            return;
        }
        if (lexerMap.get("initialMode") instanceof String init) {
            grammarDef.initialLexerMode = init;
        }

        // Trivia tokens marked in lexerRules
        if (lexerMap.get("trivia") instanceof Map<?, ?> triviaMap) {
            for (Map.Entry<?, ?> entry : triviaMap.entrySet()) {
                String tokenName = entry.getKey().toString();
                Integer id = tokenIds.get(tokenName);
                if (id == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined trivia token: " + tokenName);
                }
                ContextualLexerRule rule = grammarDef.lexerRules.computeIfAbsent(id, k -> new ContextualLexerRule());
                rule.isTrivia = true;
            }
        }

        // Token rules
        if (lexerMap.get("tokens") instanceof Map<?, ?> tokensMap) {
            for (Map.Entry<?, ?> entry : tokensMap.entrySet()) {
                String tokenName = entry.getKey().toString();
                Integer id = tokenIds.get(tokenName);
                if (id == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token in lexer: " + tokenName);
                }
                if (entry.getValue() instanceof Map<?, ?> rMap) {
                    ContextualLexerRule rule = grammarDef.lexerRules.computeIfAbsent(id, k -> new ContextualLexerRule());
                    if (rMap.get("priority") instanceof Number num) rule.priority = num.intValue();
                    if (rMap.get("pushMode") instanceof String push) rule.pushMode = push;
                    if (rMap.get("popMode") instanceof Boolean pop) rule.popMode = pop;
                    if (rMap.get("validator") instanceof String val) rule.validator = val;
                    if (rMap.get("capture") instanceof Number num) rule.capture = num.intValue();
                    if (rMap.get("captureFlag") instanceof Number num) rule.captureFlag = num.intValue();
                    if (rMap.get("dynamic") instanceof Number num) rule.dynamic = num.intValue();
                    if (rMap.get("dynamicTrigger") instanceof Number num) rule.dynamicTrigger = num.intValue();
                }
            }
        }

        // Lexer Modes
        if (lexerMap.get("modes") instanceof Map<?, ?> modesMap) {
            for (Map.Entry<?, ?> entry : modesMap.entrySet()) {
                String modeName = entry.getKey().toString();
                if (!(entry.getValue() instanceof Map<?, ?> mObj)) {
                    throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "Invalid mode object: " + modeName);
                }
                LexerMode mode = new LexerMode();
                mode.name = modeName;
                if (mObj.get("tokens") instanceof List<?> tList) {
                    mode.tokens = resolveTokenList(tList, tokenIds);
                }
                if (mObj.get("trivia") instanceof List<?> trList) {
                    mode.trivia = resolveTokenList(trList, tokenIds);
                }
                grammarDef.lexerModes.put(modeName, mode);
            }

            if (!grammarDef.lexerModes.isEmpty()) {
                if (!grammarDef.lexerModes.containsKey(grammarDef.initialLexerMode)) {
                    throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "Initial mode not found in modes: " + grammarDef.initialLexerMode);
                }
                for (ContextualLexerRule rule : grammarDef.lexerRules.values()) {
                    if (rule.pushMode != null && !grammarDef.lexerModes.containsKey(rule.pushMode)) {
                        throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "pushMode not defined in modes: " + rule.pushMode);
                    }
                }
            }
        }

        // Lexer Goals
        if (lexerMap.get("goals") instanceof Map<?, ?> goalsMap) {
            for (Map.Entry<?, ?> entry : goalsMap.entrySet()) {
                String goalName = entry.getKey().toString();
                if (!(entry.getValue() instanceof Map<?, ?> gObj)) {
                    throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "Invalid goal object: " + goalName);
                }
                LexerGoal goal = new LexerGoal(goalName);
                for (Map.Entry<?, ?> mapEntry : gObj.entrySet()) {
                    String srcToken = mapEntry.getKey().toString();
                    Object dstVal = mapEntry.getValue();
                    if (!(dstVal instanceof String dstToken)) {
                        throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "Goal mapping target must be string");
                    }
                    Integer srcId = tokenIds.get(srcToken);
                    Integer dstId = tokenIds.get(dstToken);
                    if (srcId == null || dstId == null) {
                        throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token in goal mapping");
                    }
                    goal.addMapping(srcId, dstId);
                }
                grammarDef.lexerGoals.put(goalName, goal);
            }
        }
    }

    private static int[] resolveTokenList(List<?> list, Map<String, Integer> tokenIds) {
        int[] result = new int[list.size()];
        for (int i = 0; i < list.size(); i++) {
            Object item = list.get(i);
            if (!(item instanceof String name)) {
                throw new GrammarException(GrammarException.INVALID_TOKEN_TYPE, "Token name must be string");
            }
            Integer id = tokenIds.get(name);
            if (id == null) {
                throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token in mode: " + name);
            }
            result[i] = id;
        }
        return result;
    }

    @SuppressWarnings("unchecked")
    private static void parsePrattOperators(Map<String, Object> root, Map<String, Integer> tokenIds, GrammarDefinition grammarDef) {
        if (!(root.get("operators") instanceof List<?> opList)) {
            return;
        }
        for (Object item : opList) {
            if (!(item instanceof Map<?, ?> opMap)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Operator rule must be an object");
            }
            Object tokenObj = opMap.get("token");
            if (!(tokenObj instanceof String tokenName)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Operator token must be string");
            }
            Integer tokenId = tokenIds.get(tokenName);
            if (tokenId == null) {
                throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined operator token: " + tokenName);
            }

            List<String> roles = new ArrayList<>();
            if (opMap.get("roles") instanceof List<?> rList) {
                if (rList.isEmpty()) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "roles array cannot be empty");
                }
                for (Object r : rList) {
                    if (r instanceof String s) roles.add(s);
                    else throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "roles item must be string");
                }
            } else if (opMap.get("role") instanceof String rStr) {
                roles.add(rStr);
            } else {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Operator requires role or roles");
            }

            Object leftValidator = opMap.get("leftValidator");
            if (leftValidator != null && !(leftValidator instanceof String)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "leftValidator must be string");
            }
            Object operandValidator = opMap.get("operandValidator");
            if (operandValidator != null && !(operandValidator instanceof String)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "operandValidator must be string");
            }

            Object middle = opMap.get("middleTerminator");

            for (String roleStr : roles) {
                OperatorRole role = OperatorRole.fromString(roleStr);
                if (role == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid operator role: " + roleStr);
                }

                OperatorDef def = new OperatorDef();
                def.tokenId = tokenId;
                def.role = role;

                String specific = role == OperatorRole.PREFIX ? "prefixPrecedence" :
                                 role == OperatorRole.POSTFIX ? "postfixPrecedence" : "infixPrecedence";
                Object precVal = opMap.containsKey(specific) ? opMap.get(specific) : opMap.get("precedence");
                if (!(precVal instanceof Number num)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Operator missing precedence");
                }
                def.precedence = num.intValue();

                if (opMap.get("associativity") instanceof String assocStr) {
                    def.associativity = OperatorAssociativity.fromString(assocStr);
                }

                if (leftValidator instanceof String lv) def.leftValidator = lv;
                if (operandValidator instanceof String ov) def.operandValidator = ov;

                if (role == OperatorRole.TERNARY) {
                    if (!(middle instanceof String midName)) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Ternary operator requires string middleTerminator");
                    }
                    Integer midId = tokenIds.get(midName);
                    if (midId == null) {
                        throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined middleTerminator token: " + midName);
                    }
                    def.secondaryTokenId = midId;
                }

                grammarDef.operators.add(def);
            }
        }
    }

    @SuppressWarnings("unchecked")
    private static void parseGrammar(Map<String, Object> root, Map<String, Integer> tokenIds, GrammarDefinition grammarDef) {
        Object grammarObj = root.get("grammar");
        if (grammarObj == null) {
            return;
        }
        if (!(grammarObj instanceof Map<?, ?> gMap)) {
            throw new GrammarException(GrammarException.GRAMMAR_NOT_OBJECT, "Grammar section must be an object");
        }

        Object startObj = gMap.get("start");
        if (!(startObj instanceof String startName)) {
            throw new GrammarException(GrammarException.GRAMMAR_START_NOT_FOUND, "Grammar start production missing or not string");
        }

        Object prodsObj = gMap.get("productions");
        if (!(prodsObj instanceof Map<?, ?> pMap) || pMap.isEmpty()) {
            throw new GrammarException(GrammarException.GRAMMAR_PRODUCTIONS_NOT_OBJECT, "Grammar productions must be non-empty object");
        }

        // Parse sourceFileKinds if present
        if (gMap.get("sourceFileKinds") instanceof Map<?, ?> sfkMap) {
            for (Map.Entry<?, ?> entry : sfkMap.entrySet()) {
                String kindName = entry.getKey().toString();
                if (kindName.isEmpty() || !(entry.getValue() instanceof List<?> sList) || sList.isEmpty()) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid sourceFileKind array");
                }
                List<String> suffixes = new ArrayList<>();
                for (Object item : sList) {
                    if (!(item instanceof String suffix) || !suffix.startsWith(".") || suffix.length() < 2 ||
                        suffix.contains("/") || suffix.contains("\\")) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid sourceFileKind suffix: " + item);
                    }
                    suffixes.add(suffix);
                }
                grammarDef.sourceFileKinds.put(kindName, suffixes);
            }
        }

        // Parse grammar lifecycle events
        if (gMap.get("events") instanceof Map<?, ?> evMap) {
            if (evMap.size() != 1 || !evMap.containsKey("onSourceComplete")) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid grammar events");
            }
            parseEventBinding(evMap.get("onSourceComplete"), (handler, config) -> {
                grammarDef.sourceCompleteHandler = handler;
                grammarDef.sourceCompleteConfiguration = config;
            });
        }

        // Builder to collect productions
        GrammarBuilder builder = new GrammarBuilder();
        builder.tokenIds = tokenIds;
        builder.globalSyncTokens = grammarDef.recoveryPolicy.synchronizationTokens;
        builder.sourceFileKinds = grammarDef.sourceFileKinds;

        // Allocate slots for all named productions
        Map<String, Integer> namedIds = new LinkedHashMap<>();
        int index = 0;
        for (Object key : pMap.keySet()) {
            String name = key.toString();
            namedIds.put(name, index);
            Production p = new Production();
            p.id = index;
            p.name = name;
            builder.productions.add(p);
            index++;
        }
        builder.namedCount = builder.productions.size();
        builder.namedIds = namedIds;

        Integer startId = namedIds.get(startName);
        if (startId == null) {
            throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_REFERENCE, "Start production undefined: " + startName);
        }
        grammarDef.startProduction = startId;

        // Parse each named production construct
        for (Map.Entry<?, ?> entry : pMap.entrySet()) {
            int prodId = namedIds.get(entry.getKey().toString());
            parseConstruct(builder, entry.getValue(), prodId);
        }

        // Validate graph integrity: left recursion, nullable repeat loops
        validateGrammar(builder);

        grammarDef.productions = builder.productions;
    }

    private static class GrammarBuilder {
        Map<String, Integer> tokenIds;
        Map<String, Integer> namedIds;
        int namedCount;
        List<Production> productions = new ArrayList<>();
        int[] globalSyncTokens;
        Map<String, List<String>> sourceFileKinds;

        int appendAnonymous() {
            int id = productions.size();
            Production p = new Production();
            p.id = id;
            productions.add(p);
            return id;
        }
    }

    @SuppressWarnings("unchecked")
    private static void parseConstruct(GrammarBuilder builder, Object constructObj, int productionId) {
        if (!(constructObj instanceof Map<?, ?> constructMap)) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Production construct must be an object");
        }

        // Validate supported keys
        for (Object keyObj : constructMap.keySet()) {
            String key = keyObj.toString();
            if (!ALLOWED_CONSTRUCT_KEYS.contains(key)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unsupported key in construct: " + key);
            }
        }

        // Separate core construct from metadata
        Map<String, Object> coreMap = new LinkedHashMap<>();
        for (String key : CONSTRUCT_KEYS) {
            if (constructMap.containsKey(key)) {
                coreMap.put(key, constructMap.get(key));
            }
        }

        if (coreMap.size() != 1) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Construct must have exactly one core key, found: " + coreMap.size());
        }

        Production production = builder.productions.get(productionId);

        // astKind metadata
        if (constructMap.get("astKind") instanceof String ak) {
            production.astKind = ak;
        } else if (constructMap.containsKey("astKind")) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "astKind must be a string");
        }

        // Parse core construct
        String selectedKey = coreMap.keySet().iterator().next();
        Object selectedVal = coreMap.get(selectedKey);

        parseConstructCore(builder, selectedKey, selectedVal, productionId);

        // Parse metadata: category, diagnostics, events, expect, allowASI, recover, recoverUntil
        if (constructMap.containsKey("category")) {
            Object catObj = constructMap.get("category");
            if (!(catObj instanceof String catStr)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "category must be a string");
            }
            CstCategory cat = CstCategory.fromString(catStr);
            if (cat == null) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unknown CST category: " + catStr);
            }
            production.category = cat;
        }

        if (constructMap.get("diagnostics") instanceof Map<?, ?> diagMap) {
            parseDiagnosticTemplates(diagMap, production.diagnostics, 1 | 4);
        }

        if (constructMap.get("events") instanceof Map<?, ?> eventsMap) {
            parseProductionEvents(eventsMap, production);
        }

        if (constructMap.containsKey("expect")) {
            Object expObj = constructMap.get("expect");
            if (!(expObj instanceof String exp) || exp.isEmpty()) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "expect must be non-empty string");
            }
            production.expectedDescription = exp;
        }

        if (constructMap.containsKey("allowASI")) {
            Object asiObj = constructMap.get("allowASI");
            if (!(asiObj instanceof Boolean b)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "allowASI must be boolean");
            }
            production.allowAutomaticSemicolon = b;
            if (production.allowAutomaticSemicolon && production.kind != ProductionKind.TOKEN) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "allowASI only permitted on token productions");
            }
        }

        if (constructMap.get("recoverUntil") instanceof List<?> ruList) {
            production.recoverySyncTokens = parseTokenArray(builder, ruList);
            production.recoverySkip = true;
        }

        if (constructMap.get("recover") instanceof Map<?, ?> recMap) {
            for (Object k : recMap.keySet()) {
                String sk = k.toString();
                if (!sk.equals("insert") && !sk.equals("skip") && !sk.equals("synchronize")) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid recover action: " + sk);
                }
            }
            if (recMap.get("insert") instanceof String insertToken) {
                Integer tId = builder.tokenIds.get(insertToken);
                if (tId == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined recovery insert token: " + insertToken);
                }
                production.recoveryInsertToken = tId;
                production.recoveryInsertEnabled = true;
            }
            if (recMap.get("skip") instanceof Boolean b) {
                production.recoverySkip = b;
            }
            if (recMap.get("synchronize") instanceof List<?> sList) {
                production.recoverySyncTokens = parseTokenArray(builder, sList);
            }
            if (!production.recoveryInsertEnabled && !production.recoverySkip) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Recover must enable insert or skip");
            }
            int syncCount = production.recoverySyncTokens != null ? production.recoverySyncTokens.length : 0;
            int globalSyncCount = builder.globalSyncTokens != null ? builder.globalSyncTokens.length : 0;
            if (production.recoverySkip && syncCount == 0 && globalSyncCount == 0) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Skip recovery requires synchronization tokens");
            }
        }
    }

    private static int[] parseTokenArray(GrammarBuilder builder, List<?> list) {
        if (list.isEmpty()) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Recovery tokens array cannot be empty");
        }
        int[] result = new int[list.size()];
        for (int i = 0; i < list.size(); i++) {
            Object item = list.get(i);
            if (!(item instanceof String name)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Recovery token must be string");
            }
            Integer id = builder.tokenIds.get(name);
            if (id == null) {
                throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined recovery token: " + name);
            }
            result[i] = id;
        }
        return result;
    }

    @SuppressWarnings("unchecked")
    private static void parseConstructCore(GrammarBuilder builder, String key, Object val, int productionId) {
        Production production = builder.productions.get(productionId);

        switch (key) {
            case "token": {
                if (!(val instanceof String tokenName)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "token must be string");
                }
                Integer tId = builder.tokenIds.get(tokenName);
                if (tId == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token reference: " + tokenName);
                }
                production.kind = ProductionKind.TOKEN;
                production.tokenId = tId;
                break;
            }
            case "ref": {
                if (!(val instanceof String refName)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "ref must be string");
                }
                Integer rId = builder.namedIds.get(refName);
                if (rId == null) {
                    throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_REFERENCE, "Undefined production reference: " + refName);
                }
                production.kind = ProductionKind.REF;
                production.referencedProduction = rId;
                break;
            }
            case "sequence":
            case "choice": {
                if (!(val instanceof List<?> list)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, key + " must be array");
                }
                production.kind = key.equals("sequence") ? ProductionKind.SEQUENCE : ProductionKind.CHOICE;
                production.children = new int[list.size()];
                for (int i = 0; i < list.size(); i++) {
                    int childId = builder.appendAnonymous();
                    production.children[i] = childId;
                    parseConstruct(builder, list.get(i), childId);
                }
                break;
            }
            case "optional":
            case "repeat":
            case "lookahead":
            case "not": {
                if (!(val instanceof Map<?, ?>)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, key + " must be object");
                }
                production.kind = key.equals("optional") ? ProductionKind.OPTIONAL :
                                  key.equals("repeat") ? ProductionKind.REPEAT :
                                  key.equals("lookahead") ? ProductionKind.LOOKAHEAD : ProductionKind.NOT;
                int childId = builder.appendAnonymous();
                production.children = new int[]{childId};
                parseConstruct(builder, val, childId);
                break;
            }
            case "when": {
                parseGuard(builder, val, production);
                break;
            }
            case "commit": {
                if (!(val instanceof Boolean b) || !b) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "commit must be true");
                }
                production.kind = ProductionKind.COMMIT;
                break;
            }
            case "pratt": {
                if (!(val instanceof Map<?, ?> prattMap)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "pratt must be object");
                }
                Object primary = prattMap.get("primary");
                if (!(primary instanceof Map<?, ?>) || prattMap.size() > 3) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid pratt construct");
                }
                int minPrec = 0;
                if (prattMap.containsKey("minimumPrecedence")) {
                    Object min = prattMap.get("minimumPrecedence");
                    if (!(min instanceof Number num)) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "minimumPrecedence must be integer");
                    }
                    minPrec = num.intValue();
                }
                Object postfix = prattMap.get("postfix");
                if (postfix != null && !(postfix instanceof Map<?, ?>)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "postfix must be object");
                }

                production.kind = ProductionKind.PRATT;
                production.minimumPrecedence = minPrec;
                int childCount = postfix != null ? 2 : 1;
                production.children = new int[childCount];

                int primaryId = builder.appendAnonymous();
                production.children[0] = primaryId;
                parseConstruct(builder, primary, primaryId);

                if (postfix != null) {
                    int postfixId = builder.appendAnonymous();
                    production.children[1] = postfixId;
                    parseConstruct(builder, postfix, postfixId);
                }
                break;
            }
            case "withGoal": {
                if (!(val instanceof Map<?, ?> goalMap) || goalMap.size() != 2) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withGoal must have name and production");
                }
                Object nameObj = goalMap.get("name");
                Object innerProd = goalMap.get("production");
                if (!(nameObj instanceof String goalName) || goalName.isEmpty() || !(innerProd instanceof Map<?, ?>)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid withGoal definition");
                }
                production.kind = ProductionKind.LEXICAL_GOAL;
                production.lexicalGoal = goalName;
                int childId = builder.appendAnonymous();
                production.children = new int[]{childId};
                parseConstruct(builder, innerProd, childId);
                break;
            }
            case "capture":
            case "matchCapture": {
                if (!(val instanceof Map<?, ?> capMap)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, key + " must be object");
                }
                boolean isCapture = key.equals("capture");
                if (capMap.size() != (isCapture ? 3 : 2)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid " + key + " field count");
                }
                Object nameObj = capMap.get("name");
                Object innerProd = capMap.get("production");
                Object thenProd = capMap.get("then");
                if (!(nameObj instanceof String capName) || capName.isEmpty() || !(innerProd instanceof Map<?, ?>) ||
                    (isCapture && !(thenProd instanceof Map<?, ?>))) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid " + key + " definition");
                }
                production.kind = isCapture ? ProductionKind.CAPTURE : ProductionKind.MATCH_CAPTURE;
                production.captureName = capName;

                int child1 = builder.appendAnonymous();
                parseConstruct(builder, innerProd, child1);

                if (isCapture) {
                    int child2 = builder.appendAnonymous();
                    parseConstruct(builder, thenProd, child2);
                    production.children = new int[]{child1, child2};
                } else {
                    production.children = new int[]{child1};
                }
                break;
            }
            case "oneOrMore": {
                if (!(val instanceof Map<?, ?>)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "oneOrMore must be object");
                }
                // Desugar: sequence [child1, repeat(child1_copy)]
                int child1 = builder.appendAnonymous();
                parseConstruct(builder, val, child1);

                int repeatChild = builder.appendAnonymous();
                parseConstruct(builder, val, repeatChild);

                int child2 = builder.appendAnonymous();
                Production repeatProd = builder.productions.get(child2);
                repeatProd.kind = ProductionKind.REPEAT;
                repeatProd.children = new int[]{repeatChild};

                production.kind = ProductionKind.SEQUENCE;
                production.children = new int[]{child1, child2};
                break;
            }
            case "withContext": {
                if (!(val instanceof Map<?, ?> ctxMap)) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withContext must be object");
                }
                Object setObj = ctxMap.get("set");
                if (!(setObj instanceof Map<?, ?> setMap) || setMap.isEmpty()) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withContext set must be non-empty object");
                }
                String innerKey = null;
                Object innerVal = null;
                if (ctxMap.containsKey("ref")) {
                    innerKey = "ref";
                    innerVal = ctxMap.get("ref");
                }
                if (ctxMap.containsKey("sequence")) {
                    if (innerKey != null) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withContext cannot have both ref and sequence");
                    }
                    innerKey = "sequence";
                    innerVal = ctxMap.get("sequence");
                }
                if (innerKey == null || ctxMap.size() != 2) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withContext must contain exactly set and (ref or sequence)");
                }

                int currentId = productionId;
                for (Map.Entry<?, ?> entry : setMap.entrySet()) {
                    String settingName = entry.getKey().toString();
                    long settingValue;
                    if (entry.getValue() instanceof Boolean b) {
                        settingValue = b ? 1L : 0L;
                    } else if (entry.getValue() instanceof Number num) {
                        settingValue = num.longValue();
                    } else {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "withContext value must be boolean or integer");
                    }

                    int childId = builder.appendAnonymous();
                    Production currProd = builder.productions.get(currentId);
                    currProd.kind = ProductionKind.CONTEXT;
                    currProd.contextName = settingName;
                    currProd.contextValue = settingValue;
                    currProd.children = new int[]{childId};
                    currentId = childId;
                }

                Map<String, Object> innerConstruct = new LinkedHashMap<>();
                innerConstruct.put(innerKey, innerVal);
                parseConstruct(builder, innerConstruct, currentId);
                break;
            }
            default:
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unknown construct: " + key);
        }
    }

    @SuppressWarnings("unchecked")
    private static void parseGuard(GrammarBuilder builder, Object guardObj, Production production) {
        if (!(guardObj instanceof Map<?, ?> gMap) || gMap.isEmpty()) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "when must be non-empty object");
        }
        production.kind = ProductionKind.PREDICATE;

        if (gMap.containsKey("native")) {
            if (gMap.size() != 1 || !(gMap.get("native") instanceof String natName) || natName.isEmpty()) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "native predicate must be non-empty string as sole member");
            }
            production.predicateName = natName;
            return;
        }

        Guard guard = new Guard();
        production.guard = guard;

        boolean hasLine = false;
        boolean hasEof = false;

        for (Map.Entry<?, ?> entry : gMap.entrySet()) {
            String key = entry.getKey().toString();
            Object val = entry.getValue();

            switch (key) {
                case "lineTerminatorBefore":
                case "noLineTerminatorBefore":
                    if (hasLine || !(val instanceof Boolean b)) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid line terminator condition");
                    }
                    hasLine = true;
                    boolean req = b;
                    if (key.equals("noLineTerminatorBefore")) req = !req;
                    guard.lineTerminatorBefore = req ? 1 : -1;
                    break;
                case "nextToken":
                case "nextTokenIn":
                    boolean single = key.equals("nextToken");
                    if (guard.nextTokens != null) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Multiple nextToken conditions");
                    }
                    if (single) {
                        if (!(val instanceof String tokenName)) {
                            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "nextToken must be string");
                        }
                        Integer id = builder.tokenIds.get(tokenName);
                        if (id == null) {
                            throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token in guard: " + tokenName);
                        }
                        guard.nextTokens = new int[]{id};
                    } else {
                        if (!(val instanceof List<?> list) || list.isEmpty()) {
                            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "nextTokenIn must be non-empty array");
                        }
                        guard.nextTokens = new int[list.size()];
                        for (int i = 0; i < list.size(); i++) {
                            Object item = list.get(i);
                            if (!(item instanceof String tName)) {
                                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "nextTokenIn item must be string");
                            }
                            Integer id = builder.tokenIds.get(tName);
                            if (id == null) {
                                throw new GrammarException(GrammarException.GRAMMAR_UNDEFINED_TOKEN, "Undefined token in guard: " + tName);
                            }
                            guard.nextTokens[i] = id;
                        }
                    }
                    break;
                case "allowEOF":
                    if (!(val instanceof Boolean b)) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "allowEOF must be boolean");
                    }
                    hasEof = true;
                    guard.allowEof = b;
                    break;
                case "nextTokenText":
                    if (!(val instanceof String text) || text.isEmpty()) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "nextTokenText must be non-empty string");
                    }
                    guard.nextTokenText = text;
                    break;
                case "sourceFileKind":
                    if (!(val instanceof String kindName) || builder.sourceFileKinds == null ||
                        !builder.sourceFileKinds.containsKey(kindName)) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Undefined or invalid sourceFileKind: " + val);
                    }
                    List<String> suffixes = builder.sourceFileKinds.get(kindName);
                    guard.fileSuffixes = suffixes.toArray(new String[0]);
                    break;
                default:
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unknown guard member: " + key);
            }
        }

        if (hasEof && guard.nextTokens == null) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "allowEOF requires nextToken or nextTokenIn");
        }
    }

    private static void parseDiagnosticTemplates(Map<?, ?> diagMap, DiagnosticTemplates target, int allowed) {
        if (diagMap.isEmpty()) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "diagnostics object cannot be empty");
        }
        for (Map.Entry<?, ?> entry : diagMap.entrySet()) {
            String key = entry.getKey().toString();
            int kind = key.equals("expected") ? 1 :
                       key.equals("tokenExpected") ? 2 :
                       key.equals("recovered") ? 4 : 0;
            if ((kind & allowed) == 0 || !(entry.getValue() instanceof Map<?, ?> tMap) || tMap.size() != 2) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid diagnostic template: " + key);
            }
            Object code = tMap.get("code");
            Object msg = tMap.get("message");
            if (!(code instanceof String cStr) || cStr.isEmpty() ||
                !(msg instanceof String mStr) || mStr.isEmpty() || !isDiagnosticMessageValid(mStr)) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid diagnostic template code or message");
            }
            DiagnosticTemplate tmpl = new DiagnosticTemplate(cStr, mStr);
            if (kind == 1) target.expected = tmpl;
            else if (kind == 2) target.tokenExpected = tmpl;
            else target.recovered = tmpl;
        }
    }

    private static boolean isDiagnosticMessageValid(String message) {
        int substitutions = 0;
        for (int i = 0; i < message.length(); i++) {
            if (message.charAt(i) != '%') continue;
            i++;
            if (i >= message.length()) return false;
            if (message.charAt(i) == '%') continue;
            if (message.charAt(i) != 's' || ++substitutions > 1) return false;
        }
        return true;
    }

    private interface EventBindingConsumer {
        void accept(String handler, String configuration);
    }

    @SuppressWarnings("unchecked")
    private static void parseEventBinding(Object binding, EventBindingConsumer consumer) {
        if (binding instanceof String h) {
            if (h.isEmpty()) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Event handler name cannot be empty");
            }
            consumer.accept(h, null);
        } else if (binding instanceof Map<?, ?> map) {
            for (Object k : map.keySet()) {
                String sk = k.toString();
                if (!sk.equals("handler") && !sk.equals("configuration")) {
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid event binding key: " + sk);
                }
            }
            Object handlerObj = map.get("handler");
            if (!(handlerObj instanceof String h) || h.isEmpty()) {
                throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Event binding handler must be non-empty string");
            }
            Object configObj = map.get("configuration");
            String serialized = configObj != null ? GSON.toJson(configObj) : null;
            consumer.accept(h, serialized);
        } else {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Invalid event binding");
        }
    }

    private static void parseProductionEvents(Map<?, ?> eventsMap, Production production) {
        if (eventsMap.isEmpty()) {
            throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "events object cannot be empty");
        }
        for (Map.Entry<?, ?> entry : eventsMap.entrySet()) {
            String key = entry.getKey().toString();
            switch (key) {
                case "onValidate":
                    if (production.validateHandler != null) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Duplicate onValidate event");
                    }
                    parseEventBinding(entry.getValue(), (h, c) -> {
                        production.validateHandler = h;
                        production.validateConfiguration = c;
                    });
                    break;
                case "onCommit":
                    if (production.commitHandler != null) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Duplicate onCommit event");
                    }
                    parseEventBinding(entry.getValue(), (h, c) -> {
                        production.commitHandler = h;
                        production.commitConfiguration = c;
                    });
                    break;
                case "onRecovery":
                    if (production.recoveryHandler != null) {
                        throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Duplicate onRecovery event");
                    }
                    parseEventBinding(entry.getValue(), (h, c) -> {
                        production.recoveryHandler = h;
                        production.recoveryConfiguration = c;
                    });
                    break;
                default:
                    throw new GrammarException(GrammarException.GRAMMAR_INVALID_PRODUCTION, "Unknown production event: " + key);
            }
        }
    }

    private static void validateGrammar(GrammarBuilder builder) {
        int count = builder.productions.size();
        boolean[] nullable = new boolean[count];
        computeNullable(builder, nullable);

        // 1. Reject nullable repeat loops
        for (int i = 0; i < count; i++) {
            Production p = builder.productions.get(i);
            if (p.kind == ProductionKind.REPEAT && nullable[p.children[0]]) {
                throw new GrammarException(GrammarException.GRAMMAR_NULLABLE_REPEAT,
                    "Grammar contains repeat over nullable production at id: " + i);
            }
            if (p.kind == ProductionKind.PRATT && p.children != null && p.children.length == 2 &&
                nullable[p.children[1]]) {
                throw new GrammarException(GrammarException.GRAMMAR_NULLABLE_REPEAT,
                    "Grammar contains nullable PRATT postfix production at id: " + i);
            }
        }

        // 2. Reject left recursion cycles
        int[] colors = new int[count];
        for (int i = 0; i < count; i++) {
            Arrays.fill(colors, 0);
            if (isLeftRecursive(builder, i, nullable, colors)) {
                throw new GrammarException(GrammarException.GRAMMAR_LEFT_RECURSION,
                    "Grammar contains left recursion cycle starting at production: " +
                    (builder.productions.get(i).name != null ? builder.productions.get(i).name : String.valueOf(i)));
            }
        }
    }

    private static void computeNullable(GrammarBuilder builder, boolean[] nullable) {
        int count = builder.productions.size();
        for (int pass = 0; pass < count; pass++) {
            boolean changed = false;
            for (int i = 0; i < count; i++) {
                Production p = builder.productions.get(i);
                boolean value = false;
                switch (p.kind) {
                    case TOKEN:
                        value = false;
                        break;
                    case REF:
                        value = p.referencedProduction >= 0 && nullable[p.referencedProduction];
                        break;
                    case SEQUENCE:
                        value = true;
                        if (p.children != null) {
                            for (int childId : p.children) {
                                value = value && nullable[childId];
                            }
                        }
                        break;
                    case CHOICE:
                        value = false;
                        if (p.children != null) {
                            for (int childId : p.children) {
                                value = value || nullable[childId];
                            }
                        }
                        break;
                    case OPTIONAL:
                    case REPEAT:
                    case LOOKAHEAD:
                    case NOT:
                    case PREDICATE:
                    case COMMIT:
                        value = true;
                        break;
                    case CONTEXT:
                    case LEXICAL_GOAL:
                    case MATCH_CAPTURE:
                        value = p.children != null && p.children.length == 1 && nullable[p.children[0]];
                        break;
                    case CAPTURE:
                        value = p.children != null && p.children.length == 2 && nullable[p.children[0]] && nullable[p.children[1]];
                        break;
                    case PRATT:
                        value = p.children != null && p.children.length >= 1 && nullable[p.children[0]];
                        break;
                }
                if (value && !nullable[i]) {
                    nullable[i] = true;
                    changed = true;
                }
            }
            if (!changed) break;
        }
    }

    private static boolean isLeftRecursive(GrammarBuilder builder, int productionId, boolean[] nullable, int[] colors) {
        if (colors[productionId] == 1) return true;
        if (colors[productionId] == 2) return false;
        colors[productionId] = 1;

        Production p = builder.productions.get(productionId);
        boolean cycle = false;

        switch (p.kind) {
            case REF:
                if (p.referencedProduction >= 0) {
                    cycle = isLeftRecursive(builder, p.referencedProduction, nullable, colors);
                }
                break;
            case SEQUENCE:
                if (p.children != null) {
                    for (int childId : p.children) {
                        cycle = isLeftRecursive(builder, childId, nullable, colors);
                        if (cycle || !nullable[childId]) break;
                    }
                }
                break;
            case CHOICE:
                if (p.children != null) {
                    for (int childId : p.children) {
                        cycle = isLeftRecursive(builder, childId, nullable, colors);
                        if (cycle) break;
                    }
                }
                break;
            case OPTIONAL:
            case REPEAT:
            case LOOKAHEAD:
            case NOT:
            case CONTEXT:
            case LEXICAL_GOAL:
            case PRATT:
            case MATCH_CAPTURE:
                if (p.children != null && p.children.length > 0) {
                    cycle = isLeftRecursive(builder, p.children[0], nullable, colors);
                }
                break;
            case CAPTURE:
                if (p.children != null && p.children.length > 0) {
                    cycle = isLeftRecursive(builder, p.children[0], nullable, colors);
                    if (!cycle && nullable[p.children[0]] && p.children.length > 1) {
                        cycle = isLeftRecursive(builder, p.children[1], nullable, colors);
                    }
                }
                break;
            case TOKEN:
            case PREDICATE:
            case COMMIT:
                break;
        }

        colors[productionId] = 2;
        return cycle;
    }
}
