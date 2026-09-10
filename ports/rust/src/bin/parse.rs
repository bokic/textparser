#[path = "../common.rs"]
mod common;

use std::env;
use std::path::Path;
use std::process;

fn main() {
    let args: Vec<String> = env::args().skip(1).collect();
    if args.is_empty() {
        eprintln!("{}", common::PARSE_USAGE);
        process::exit(2);
    }

    let definition = Path::new(&args[0]);
    let parser = match common::load_parser(definition) {
        Ok(parser) => parser,
        Err(error) => {
            eprintln!("Error loading definition: {error}");
            process::exit(1);
        }
    };

    let result = match args.as_slice() {
        [_, flag] if flag == "--stdinformat" => {
            match common::read_input(None).and_then(|input| parser.parse_format(&input)) {
                Ok(bytes) => {
                    print!("{}", common::format_bytes(&bytes));
                    return;
                }
                Err(error) => Err(error),
            }
        }
        [_, text_file] => common::parse_file(&parser, Path::new(text_file))
            .and_then(|tokens| common::tree_json(&tokens)),
        [_, text_file, flag] if flag == "--format" => {
            common::read_input(Some(Path::new(text_file)))
                .and_then(|input| parser.parse_format(&input))
                .map(|bytes| common::format_bytes(&bytes))
        }
        _ => {
            eprintln!("{}", common::PARSE_USAGE);
            process::exit(2);
        }
    };

    match result {
        Ok(output) => print!("{output}"),
        Err(error) => {
            eprintln!("{error}");
            process::exit(1);
        }
    }
}
