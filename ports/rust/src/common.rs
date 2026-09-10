#![allow(dead_code)]

use std::fs;
use std::io::{self, Read};
use std::path::{Path, PathBuf};

use textparser::{TextParser, Token};

pub const PARSE_USAGE: &str =
    "Usage: parse <definition_file> [<text_file> --format] | [--stdinformat]";
pub const PARSEDIR_USAGE: &str = "Usage: parsedir <definition_file> <directory>";
pub const VALIDATE_USAGE: &str = "Usage: validate <definition_file> <directory_or_file>";

pub fn load_parser(path: &Path) -> Result<TextParser, String> {
    TextParser::from_file(path)
}

pub fn read_input(path: Option<&Path>) -> Result<String, String> {
    match path {
        Some(path) => fs::read_to_string(path)
            .map_err(|error| format!("Error reading {}: {error}", path.display())),
        None => {
            let mut input = String::new();
            io::stdin()
                .read_to_string(&mut input)
                .map_err(|error| format!("Error reading stdin: {error}"))?;
            Ok(input)
        }
    }
}

pub fn format_bytes(bytes: &[u8]) -> String {
    let mut output = String::with_capacity(bytes.len() * 2 + 1);
    for byte in bytes {
        output.push_str(&format!("{byte:02}"));
    }
    output.push('\n');
    output
}

pub fn parse_file(parser: &TextParser, path: &Path) -> Result<Vec<Token>, String> {
    let input = read_input(Some(path))?;
    parser.parse(&input)
}

pub fn collect_files(path: &Path) -> Result<Vec<PathBuf>, String> {
    if path.is_file() {
        return Ok(vec![path.to_path_buf()]);
    }
    if !path.is_dir() {
        return Err(format!("{} is not a file or directory", path.display()));
    }

    let mut files = Vec::new();
    let entries =
        fs::read_dir(path).map_err(|error| format!("Error reading {}: {error}", path.display()))?;
    for entry in entries {
        let entry = entry.map_err(|error| format!("Error reading directory entry: {error}"))?;
        let entry_path = entry.path();
        if entry_path.is_dir() {
            files.extend(collect_files(&entry_path)?);
        } else if entry_path.is_file() {
            files.push(entry_path);
        }
    }
    files.sort();
    Ok(files)
}

pub fn tree_json(tokens: &[Token]) -> Result<String, String> {
    serde_json::to_string_pretty(tokens)
        .map_err(|error| format!("Error serializing token tree: {error}"))
}

#[cfg(test)]
mod tests {
    use super::format_bytes;

    #[test]
    fn format_bytes_is_fixed_width_and_newline_terminated() {
        assert_eq!(format_bytes(&[0, 7, 12, 255]), "000712255\n");
    }
}
