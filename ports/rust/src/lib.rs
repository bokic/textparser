use fancy_regex::Regex;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::path::Path;
use std::sync::RwLock;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Token {
    pub id: String,
    pub position: usize,
    pub length: usize,
    #[serde(default)]
    pub children: Vec<Token>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MergeSignConfig {
    #[serde(default, rename = "signTokens")]
    pub sign_tokens: Vec<String>,
    #[serde(default, rename = "numberTokens")]
    pub number_tokens: Vec<String>,
    #[serde(default, rename = "operandTokens")]
    pub operand_tokens: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TokenDef {
    #[serde(rename = "type")]
    pub token_type: String,
    #[serde(default, rename = "otherTextInside")]
    pub other_text_inside: Option<bool>,
    #[serde(default, rename = "deleteIfOnlyOneChild")]
    pub delete_if_only_one_child: Option<bool>,
    #[serde(default, rename = "mustHaveOneChild")]
    pub must_have_one_child: Option<bool>,
    #[serde(default, rename = "multiLine")]
    pub multi_line: Option<bool>,
    #[serde(default, rename = "searchParentEndTokenLast")]
    pub search_parent_end_token_last: Option<bool>,
    #[serde(default, rename = "endRegex")]
    pub end_regex: Option<String>,
    #[serde(default, rename = "startRegex")]
    pub start_regex: Option<String>,
    #[serde(default, rename = "nestedTokens")]
    pub nested_tokens: Option<Vec<String>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Definition {
    pub name: Option<String>,
    #[serde(default)]
    pub version: serde_json::Value,
    #[serde(default, rename = "otherTextInside")]
    pub other_text_inside: Option<bool>,
    #[serde(default, rename = "mergeSignIntoNumber")]
    pub merge_sign_into_number: Option<MergeSignConfig>,
    #[serde(default, rename = "defaultFileExtensions")]
    pub default_file_extensions: Option<Vec<String>>,
    #[serde(default, rename = "startTokens")]
    pub start_tokens: Vec<String>,
    #[serde(default)]
    pub tokens: serde_json::Map<String, serde_json::Value>,
}

pub struct TextParser {
    pub definition: Definition,
    pub tokens: HashMap<String, TokenDef>,
    pub token_order: Vec<String>,
    pub definition_other_text_inside: bool,
    regex_cache: RwLock<HashMap<String, Regex>>,
}

struct MatchResult {
    start: usize,
    end: usize,
}

fn compile_regex(pattern: &str) -> Result<Regex, String> {
    let pat = if pattern.starts_with("(?i)") {
        pattern.to_string()
    } else {
        format!("(?i){pattern}")
    };
    Regex::new(&pat).map_err(|e| format!("Regex compile error for '{pattern}': {e}"))
}

fn regex_search(regex: &Regex, slice: &str) -> Option<MatchResult> {
    if let Ok(Some(captures)) = regex.captures(slice) {
        let last_idx = captures.len() - 1;
        if let Some(m) = captures.get(last_idx) {
            return Some(MatchResult {
                start: m.start(),
                end: m.end(),
            });
        }
    }
    None
}

fn regex_match(regex: &Regex, slice: &str) -> Option<MatchResult> {
    if let Ok(Some(captures)) = regex.captures(slice) {
        if let Some(g0) = captures.get(0) {
            if g0.start() == 0 {
                let last_idx = captures.len() - 1;
                if let Some(m) = captures.get(last_idx) {
                    return Some(MatchResult {
                        start: m.start(),
                        end: m.end(),
                    });
                }
            }
        }
    }
    None
}

fn skip_whitespace(text: &str, mut pos: usize) -> usize {
    let bytes = text.as_bytes();
    while pos < bytes.len() {
        let b = bytes[pos];
        if b == b' ' || b == b'\t' || b == b'\n' || b == b'\r' {
            pos += 1;
        } else {
            break;
        }
    }
    pos
}

impl TextParser {
    pub fn from_file<P: AsRef<Path>>(definition_file: P) -> Result<Self, String> {
        let content = fs::read_to_string(definition_file)
            .map_err(|e| format!("Failed to read definition file: {e}"))?;
        Self::from_json_str(&content)
    }

    pub fn from_json_str(json_str: &str) -> Result<Self, String> {
        let definition: Definition = serde_json::from_str(json_str)
            .map_err(|e| format!("Failed to parse definition JSON: {e}"))?;

        let mut tokens = HashMap::new();
        let mut token_order = Vec::new();

        for (k, v) in &definition.tokens {
            token_order.push(k.clone());
            let mut tok_def: TokenDef = serde_json::from_value(v.clone())
                .map_err(|e| format!("Failed to parse token definition for '{k}': {e}"))?;

            if tok_def.other_text_inside.is_none() {
                tok_def.other_text_inside = Some(false);
            }
            if tok_def.delete_if_only_one_child.is_none() {
                tok_def.delete_if_only_one_child = Some(false);
            }
            if tok_def.must_have_one_child.is_none() {
                tok_def.must_have_one_child = Some(false);
            }
            if tok_def.multi_line.is_none() {
                tok_def.multi_line = Some(false);
            }
            if tok_def.search_parent_end_token_last.is_none() {
                tok_def.search_parent_end_token_last = Some(false);
            }

            tokens.insert(k.clone(), tok_def);
        }

        let definition_other_text_inside = definition.other_text_inside.unwrap_or(false);

        Ok(Self {
            definition,
            tokens,
            token_order,
            definition_other_text_inside,
            regex_cache: RwLock::new(HashMap::new()),
        })
    }

    fn get_regex(&self, pattern: &str) -> Result<Regex, String> {
        {
            let cache = self.regex_cache.read().unwrap();
            if let Some(r) = cache.get(pattern) {
                return Ok(r.clone());
            }
        }

        let compiled = compile_regex(pattern)?;
        let mut cache = self.regex_cache.write().unwrap();
        cache.insert(pattern.to_string(), compiled.clone());
        Ok(compiled)
    }

    fn find_token(&self, text: &str, pos: usize, token_name: &str, other_text_inside: bool) -> Result<Option<usize>, String> {
        let token_def = match self.tokens.get(token_name) {
            Some(t) => t,
            None => return Err(format!("Token definition for '{token_name}' not found")),
        };

        match token_def.token_type.as_str() {
            "GroupOneChildOnly" | "Group" => {
                let mut closest_child_pos = usize::MAX;
                if let Some(nested) = &token_def.nested_tokens {
                    for child_name in nested {
                        if let Some(child_pos) = self.find_token(text, pos, child_name, other_text_inside)? {
                            if child_pos < closest_child_pos {
                                closest_child_pos = child_pos;
                            }
                        }
                    }
                }
                if closest_child_pos == usize::MAX {
                    Ok(None)
                } else {
                    Ok(Some(closest_child_pos))
                }
            }
            "GroupAllChildrenInSameOrder" => {
                if let Some(nested) = &token_def.nested_tokens {
                    if !nested.is_empty() {
                        return self.find_token(text, pos, &nested[0], other_text_inside);
                    }
                }
                Ok(None)
            }
            "SimpleToken" | "StartStop" | "StartOptStop" => {
                let start_regex_str = match &token_def.start_regex {
                    Some(r) => r,
                    None => return Ok(None),
                };
                let regex = self.get_regex(start_regex_str)?;
                let slice = if pos <= text.len() { &text[pos..] } else { "" };
                if let Some(m) = regex_search(&regex, slice) {
                    Ok(Some(m.start))
                } else {
                    Ok(None)
                }
            }
            other => Err(format!("Unknown token type: {other}")),
        }
    }

    fn parse_group(&self, text: &str, token_name: &str, token_def: &TokenDef, parent_regex: Option<&str>, mut pos: usize) -> Result<Token, String> {
        pos = skip_whitespace(text, pos);

        let mut ret = Token {
            id: token_name.to_string(),
            position: pos,
            length: 0,
            children: Vec::new(),
        };

        let nested_tokens = token_def.nested_tokens.as_deref().unwrap_or(&[]);
        let search_parent_end_token_last = token_def.search_parent_end_token_last.unwrap_or(false);
        let token_other_text_inside = token_def.other_text_inside.unwrap_or(false);

        loop {
            let mut end_token_pos = usize::MAX;
            pos = skip_whitespace(text, pos);

            let mut closest_child_token_pos = usize::MAX;
            let mut closest_child_token_name: Option<&str> = None;

            if !search_parent_end_token_last {
                if let Some(parent_pat) = parent_regex {
                    let reg = self.get_regex(parent_pat)?;
                    let slice = if pos <= text.len() { &text[pos..] } else { "" };
                    if let Some(m) = regex_search(&reg, slice) {
                        end_token_pos = m.start;
                    }
                }
            }

            for child_name in nested_tokens {
                if let Some(child_pos) = self.find_token(text, pos, child_name, self.definition_other_text_inside)? {
                    if child_pos < closest_child_token_pos {
                        closest_child_token_pos = child_pos;
                        closest_child_token_name = Some(child_name);
                        if closest_child_token_pos == 0 {
                            break;
                        }
                    }
                }
            }

            if closest_child_token_pos > 0 && search_parent_end_token_last {
                if let Some(parent_pat) = parent_regex {
                    let reg = self.get_regex(parent_pat)?;
                    let slice = if pos <= text.len() { &text[pos..] } else { "" };
                    if let Some(m) = regex_search(&reg, slice) {
                        end_token_pos = m.start;
                    }
                }
            }

            let mut should_break = false;
            if end_token_pos != usize::MAX && end_token_pos <= closest_child_token_pos {
                should_break = true;
            }

            if should_break {
                ret.length = pos + end_token_pos - ret.position;
                break;
            }

            if closest_child_token_pos == usize::MAX || closest_child_token_name.is_none() {
                break;
            }

            let closest_name = closest_child_token_name.unwrap();

            if closest_child_token_pos > 0 && !token_other_text_inside {
                return Err(format!("Child token {closest_name} has illegal position!"));
            }

            pos += closest_child_token_pos;

            let child_def = self.tokens.get(closest_name).ok_or_else(|| format!("Token {closest_name} not found"))?;
            let child = self.parse_token(text, closest_name, child_def, parent_regex, pos)?;

            if child.position < pos {
                return Err(format!("Child token {closest_name} has illegal position!"));
            }

            if child.length == 0 {
                return Err(format!("Child token {closest_name} has illegal length!"));
            }

            ret.length = child.position + child.length - ret.position;
            if ret.length == 0 {
                return Err(format!("Child token {closest_name} has illegal length!"));
            }

            pos = child.position + child.length;
            ret.children.push(child);
        }

        Ok(ret)
    }

    fn parse_group_one_child_only(&self, text: &str, token_name: &str, token_def: &TokenDef, parent_regex: Option<&str>, mut pos: usize) -> Result<Token, String> {
        pos = skip_whitespace(text, pos);

        let nested_tokens = match &token_def.nested_tokens {
            Some(n) if !n.is_empty() => n,
            _ => return Err("GroupOneChildOnly token type nested_tokens list is empty!".to_string()),
        };

        let token_other_text_inside = token_def.other_text_inside.unwrap_or(false);

        let mut closest_child_token_pos = usize::MAX;
        let mut closest_child_token_name: Option<&str> = None;

        for child_name in nested_tokens {
            if let Some(child_pos) = self.find_token(text, pos, child_name, token_other_text_inside)? {
                if child_pos < closest_child_token_pos {
                    closest_child_token_pos = child_pos;
                    closest_child_token_name = Some(child_name);
                }
            }
        }

        let closest_name = match closest_child_token_name {
            Some(n) => n,
            None => return Err("Search for GroupOneChildOnly token type failed. Can't find one child.".to_string()),
        };

        let child_def = self.tokens.get(closest_name).ok_or_else(|| format!("Token {closest_name} not found"))?;
        let child = self.parse_token(text, closest_name, child_def, parent_regex, pos)?;

        let ret = Token {
            id: token_name.to_string(),
            position: child.position,
            length: child.length,
            children: vec![child],
        };

        Ok(ret)
    }

    fn parse_group_all_children_in_same_order(&self, text: &str, token_name: &str, token_def: &TokenDef, parent_regex: Option<&str>, mut pos: usize) -> Result<Token, String> {
        pos = skip_whitespace(text, pos);

        let nested_tokens = token_def.nested_tokens.as_deref().unwrap_or(&[]);
        if nested_tokens.len() != 3 {
            return Err(format!("GroupAllChildrenInSameOrder should have exactly 3 nested tokens, but {} were found", nested_tokens.len()));
        }

        let start_token_name = &nested_tokens[0];
        let inner_token_name = &nested_tokens[1];
        let end_token_name = &nested_tokens[2];

        let start_token_pos = self.find_token(text, pos, start_token_name, self.definition_other_text_inside)?;
        if start_token_pos.is_none() {
            return Err(format!("Expected {start_token_name} at position: {pos}"));
        }

        let mut ret = Token {
            id: token_name.to_string(),
            position: pos,
            length: 0,
            children: Vec::new(),
        };

        let start_def = self.tokens.get(start_token_name).ok_or_else(|| format!("Token {start_token_name} not found"))?;
        let child = self.parse_token(text, start_token_name, start_def, parent_regex, pos)?;

        ret.length = child.position + child.length - ret.position;
        pos = child.position + child.length;
        ret.children.push(child);

        let end_def = self.tokens.get(end_token_name).ok_or_else(|| format!("Token {end_token_name} not found"))?;
        let new_parent_regex = end_def.start_regex.as_deref();

        let search_parent_end_token_last = token_def.search_parent_end_token_last.unwrap_or(false);

        loop {
            let inner_token_pos = self.find_token(text, pos, inner_token_name, self.definition_other_text_inside)?;
            let end_token_pos = self.find_token(text, pos, end_token_name, self.definition_other_text_inside)?;

            let end_pos = match end_token_pos {
                Some(p) => p,
                None => return Err(format!("GroupAllChildrenInSameOrder end token {end_token_name} not found")),
            };

            let inner_pos = match inner_token_pos {
                Some(p) => p,
                None => break,
            };

            if end_pos < inner_pos {
                break;
            }

            if end_pos == inner_pos && !search_parent_end_token_last {
                break;
            }

            pos += inner_pos;

            let inner_def = self.tokens.get(inner_token_name).ok_or_else(|| format!("Token {inner_token_name} not found"))?;
            let child = self.parse_token(text, inner_token_name, inner_def, new_parent_regex, pos)?;

            ret.length = child.position + child.length - ret.position;
            pos = child.position + child.length;
            ret.children.push(child);
        }

        let end_token_pos = self.find_token(text, pos, end_token_name, self.definition_other_text_inside)?;
        let end_pos = match end_token_pos {
            Some(p) => p,
            None => return Err(format!("GroupAllChildrenInSameOrder end token {end_token_name} not found")),
        };

        pos += end_pos;

        let end_token_obj = self.parse_token(text, end_token_name, end_def, new_parent_regex, pos)?;

        ret.length = end_token_obj.position + end_token_obj.length - ret.position;
        ret.children.push(end_token_obj);

        Ok(ret)
    }

    fn parse_simple_token(&self, text: &str, token_name: &str, token_def: &TokenDef, mut pos: usize) -> Result<Token, String> {
        pos = skip_whitespace(text, pos);

        let start_regex_str = token_def.start_regex.as_deref().ok_or_else(|| format!("SimpleToken {token_name} missing startRegex"))?;
        let reg = self.get_regex(start_regex_str)?;

        let slice = if pos <= text.len() { &text[pos..] } else { "" };
        let m = regex_match(&reg, slice).ok_or_else(|| format!("Expected {start_regex_str} at position: {pos}"))?;

        let tok_pos = pos + m.start;
        let tok_len = m.end - m.start;

        Ok(Token {
            id: token_name.to_string(),
            position: tok_pos,
            length: tok_len,
            children: Vec::new(),
        })
    }

    fn parse_start_stop(&self, text: &str, token_name: &str, token_def: &TokenDef, parent_regex: Option<&str>, mut pos: usize, end_required: bool) -> Result<Token, String> {
        let my_end_regex_str = token_def.end_regex.as_deref();

        pos = skip_whitespace(text, pos);

        let start_regex_str = token_def.start_regex.as_deref().ok_or_else(|| format!("Token {token_name} missing startRegex"))?;
        let start_reg = self.get_regex(start_regex_str)?;

        let slice = if pos <= text.len() { &text[pos..] } else { "" };
        let start_m = regex_match(&start_reg, slice).ok_or_else(|| format!("Expected {start_regex_str} at position: {pos}"))?;

        let start_group_start = start_m.start;

        let token_start_pos = pos + start_group_start;

        let mut ret = Token {
            id: token_name.to_string(),
            position: token_start_pos,
            length: 0,
            children: Vec::new(),
        };

        pos += start_m.end;

        let nested_tokens = match &token_def.nested_tokens {
            Some(n) => n,
            None => {
                let end_pat = my_end_regex_str.ok_or_else(|| format!("Token {token_name} missing endRegex"))?;
                let end_reg = self.get_regex(end_pat)?;
                let slice = if pos <= text.len() { &text[pos..] } else { "" };
                let end_m = regex_search(&end_reg, slice).ok_or_else(|| format!("Expected {end_pat} at position: {pos}"))?;

                let end_token_pos = end_m.start;
                let end_token_length = end_m.end - end_m.start;

                ret.length = pos + end_token_pos + end_token_length - ret.position;
                return Ok(ret);
            }
        };

        let search_parent_end_token_last = token_def.search_parent_end_token_last.unwrap_or(false);
        let token_other_text_inside = token_def.other_text_inside.unwrap_or(false);

        loop {
            let mut end_token_pos = usize::MAX;
            let mut end_token_length = 0;
            pos = skip_whitespace(text, pos);

            let check_regex_str = if search_parent_end_token_last && parent_regex.is_some() {
                parent_regex
            } else {
                my_end_regex_str
            };

            if !search_parent_end_token_last {
                if let Some(check_pat) = check_regex_str {
                    let end_reg = self.get_regex(check_pat)?;
                    let slice = if pos <= text.len() { &text[pos..] } else { "" };
                    if let Some(end_m) = regex_search(&end_reg, slice) {
                        end_token_pos = end_m.start;
                        end_token_length = end_m.end - end_m.start;
                        if end_token_pos == 0 {
                            ret.length = pos - ret.position + end_token_length;
                            break;
                        }
                    }
                }
            }

            let mut closest_child_token_pos = usize::MAX;
            let mut closest_child_token_name: Option<&str> = None;

            for child_name in nested_tokens {
                if let Some(child_pos) = self.find_token(text, pos, child_name, token_other_text_inside)? {
                    if child_pos < closest_child_token_pos {
                        closest_child_token_pos = child_pos;
                        closest_child_token_name = Some(child_name);
                        if closest_child_token_pos == 0 {
                            break;
                        }
                    }
                }
            }

            if search_parent_end_token_last {
                if let Some(check_pat) = check_regex_str {
                    let end_reg = self.get_regex(check_pat)?;
                    let slice = if pos <= text.len() { &text[pos..] } else { "" };
                    if let Some(end_m) = regex_search(&end_reg, slice) {
                        end_token_pos = end_m.start;
                        end_token_length = end_m.end - end_m.start;
                        if end_token_pos < closest_child_token_pos {
                            ret.length = pos - ret.position + end_token_pos + end_token_length;
                            break;
                        }
                    }
                }
            }

            if end_token_pos < closest_child_token_pos {
                ret.length = pos - ret.position + end_token_pos + end_token_length;
                break;
            }

            if closest_child_token_pos == usize::MAX || closest_child_token_name.is_none() {
                if end_required {
                    if let Some(check_pat) = check_regex_str {
                        let end_reg = self.get_regex(check_pat)?;
                        let slice = if pos <= text.len() { &text[pos..] } else { "" };
                        if let Some(end_m) = regex_search(&end_reg, slice) {
                            end_token_pos = end_m.start;
                            end_token_length = end_m.end - end_m.start;
                            ret.length = pos - ret.position + end_token_pos + end_token_length;
                        }
                    }
                }
                break;
            }

            let closest_name = closest_child_token_name.unwrap();

            if closest_child_token_pos > 0 && !token_other_text_inside {
                return Err(format!("Child token {closest_name} has illegal position!"));
            }

            pos += closest_child_token_pos;

            let child_def = self.tokens.get(closest_name).ok_or_else(|| format!("Token {closest_name} not found"))?;
            let child = self.parse_token(text, closest_name, child_def, my_end_regex_str, pos)?;

            if child.length == 0 {
                return Err(format!("Child token {closest_name} has no length!"));
            }

            ret.length = child.position + child.length - ret.position;
            pos = child.position + child.length;
            ret.children.push(child);
        }

        Ok(ret)
    }

    fn parse_token(&self, text: &str, token_name: &str, token_def: &TokenDef, parent_regex: Option<&str>, pos: usize) -> Result<Token, String> {
        let pos = skip_whitespace(text, pos);

        match token_def.token_type.as_str() {
            "Group" => self.parse_group(text, token_name, token_def, parent_regex, pos),
            "GroupOneChildOnly" => self.parse_group_one_child_only(text, token_name, token_def, parent_regex, pos),
            "GroupAllChildrenInSameOrder" => self.parse_group_all_children_in_same_order(text, token_name, token_def, parent_regex, pos),
            "SimpleToken" => self.parse_simple_token(text, token_name, token_def, pos),
            "StartStop" => self.parse_start_stop(text, token_name, token_def, parent_regex, pos, true),
            "StartOptStop" => self.parse_start_stop(text, token_name, token_def, parent_regex, pos, false),
            other => Err(format!("Unknown token type: {other}")),
        }
    }

    fn maybe_merge_sign(&self, token_item: &mut Token) {
        let sign_merge = match &self.definition.merge_sign_into_number {
            Some(sm) => sm,
            None => return,
        };

        let sign_tokens = &sign_merge.sign_tokens;
        let number_tokens = &sign_merge.number_tokens;
        let operand_tokens = &sign_merge.operand_tokens;

        if token_item.children.is_empty() {
            return;
        }

        let mut i = 0;
        while i < token_item.children.len() {
            let is_num = number_tokens.contains(&token_item.children[i].id);
            if is_num && i > 0 {
                let mut sign_opt: Option<(usize, usize, usize)> = None;

                let prev = &token_item.children[i - 1];
                if sign_tokens.contains(&prev.id) {
                    let context = if i >= 2 { Some(&token_item.children[i - 2]) } else { None };
                    let is_operand_ctx = context.map_or(false, |ctx| operand_tokens.contains(&ctx.id));
                    if !is_operand_ctx {
                        sign_opt = Some((prev.position, prev.length, 1));
                    }
                } else if !prev.children.is_empty() && sign_tokens.contains(&prev.children.last().unwrap().id) {
                    let last_sign = prev.children.last().unwrap();
                    let context = if prev.children.len() >= 2 {
                        Some(&prev.children[prev.children.len() - 2])
                    } else if i >= 2 {
                        Some(&token_item.children[i - 2])
                    } else {
                        None
                    };
                    let is_operand_ctx = context.map_or(false, |ctx| operand_tokens.contains(&ctx.id));
                    if !is_operand_ctx {
                        sign_opt = Some((last_sign.position, last_sign.length, 2));
                    }
                }

                if let Some((sign_pos, sign_len, kind)) = sign_opt {
                    let curr_pos = token_item.children[i].position;
                    if sign_pos + sign_len == curr_pos && sign_len == 1 {
                        token_item.children[i].length += curr_pos - sign_pos;
                        token_item.children[i].position = sign_pos;

                        if kind == 1 {
                            token_item.children.remove(i - 1);
                            i -= 1;
                        } else {
                            token_item.children[i - 1].children.pop();
                            if token_item.children[i - 1].children.is_empty() {
                                token_item.children.remove(i - 1);
                                i -= 1;
                            }
                        }
                    }
                }
            }

            if !token_item.children[i].children.is_empty() {
                self.maybe_merge_sign(&mut token_item.children[i]);
            }
            i += 1;
        }
    }

    pub fn post_process(&self, tokens: &mut Vec<Token>) {
        let mut i = 0;
        while i < tokens.len() {
            if !tokens[i].children.is_empty() {
                self.post_process(&mut tokens[i].children);
            }

            if let Some(token_def) = self.tokens.get(&tokens[i].id) {
                if token_def.delete_if_only_one_child.unwrap_or(false) && tokens[i].children.len() == 1 {
                    let only_child = tokens[i].children.remove(0);
                    tokens[i] = only_child;
                    continue;
                }
            }
            i += 1;
        }
    }

    pub fn parse(&self, text: &str) -> Result<Vec<Token>, String> {
        let mut tokens = Vec::new();
        let mut pos = 0;

        while pos < text.len() {
            pos = skip_whitespace(text, pos);
            let mut closest_token_pos = usize::MAX;
            let mut closest_token_name: Option<&str> = None;

            for token_name in &self.definition.start_tokens {
                if let Some(offset) = self.find_token(text, pos, token_name, self.definition_other_text_inside)? {
                    if offset < closest_token_pos {
                        closest_token_pos = offset;
                        closest_token_name = Some(token_name);
                    }
                }
            }

            let closest_name = match closest_token_name {
                Some(n) => n,
                None => {
                    if self.definition_other_text_inside && pos < text.len() {
                        tokens.push(Token {
                            id: "".to_string(),
                            position: pos,
                            length: text.len() - pos,
                            children: Vec::new(),
                        });
                    }
                    break;
                }
            };

            if closest_token_pos > 0 && self.definition_other_text_inside {
                let mut err_end = pos + closest_token_pos;
                let text_bytes = text.as_bytes();
                while err_end > pos && text_bytes[err_end - 1].is_ascii_whitespace() {
                    err_end -= 1;
                }
                if err_end > pos {
                    tokens.push(Token {
                        id: "".to_string(),
                        position: pos,
                        length: err_end - pos,
                        children: Vec::new(),
                    });
                }
            }

            pos += closest_token_pos;

            let token_def = self.tokens.get(closest_name).ok_or_else(|| format!("Token {closest_name} not found"))?;
            let child = self.parse_token(text, closest_name, token_def, None, pos)?;

            if child.length == 0 {
                return Err(format!("Child token {closest_name} has no length!"));
            }

            pos = child.position + child.length;
            tokens.push(child);
        }

        if self.definition.merge_sign_into_number.is_some() {
            let mut dummy_root = Token {
                id: "ROOT".to_string(),
                position: 0,
                length: 0,
                children: tokens,
            };
            self.maybe_merge_sign(&mut dummy_root);
            tokens = dummy_root.children;
        }

        self.post_process(&mut tokens);

        Ok(tokens)
    }

    pub fn parse_format(&self, text: &str) -> Result<Vec<u8>, String> {
        let mut ret = vec![0u8; text.len()];
        let tokens = self.parse(text)?;

        for token in &tokens {
            self.recursive_format(&mut ret, token);
        }

        Ok(ret)
    }

    fn recursive_format(&self, array: &mut [u8], token: &Token) {
        if let Some(token_id) = self.token_order.iter().position(|name| name == &token.id) {
            let char_val = token_id as u8;
            let start = token.position;
            let end = (token.position + token.length).min(array.len());
            if start < array.len() {
                for b in &mut array[start..end] {
                    *b = char_val;
                }
            }
        }

        for child in &token.children {
            self.recursive_format(array, child);
        }
    }
}
