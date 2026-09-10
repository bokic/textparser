#[path = "../common.rs"]
mod common;

use std::env;
use std::path::Path;
use std::process;

fn main() {
    let args: Vec<String> = env::args().skip(1).collect();
    if args.len() != 2 {
        eprintln!("{}", common::VALIDATE_USAGE);
        process::exit(2);
    }

    let parser = match common::load_parser(Path::new(&args[0])) {
        Ok(parser) => parser,
        Err(error) => {
            eprintln!("Error loading definition: {error}");
            process::exit(1);
        }
    };
    let files = match common::collect_files(Path::new(&args[1])) {
        Ok(files) => files,
        Err(error) => {
            eprintln!("{error}");
            process::exit(1);
        }
    };

    let mut failed = false;
    for file in files {
        match common::parse_file(&parser, &file) {
            Ok(tokens) => println!("{}: valid ({} tokens)", file.display(), tokens.len()),
            Err(error) => {
                eprintln!("{}: invalid: {error}", file.display());
                failed = true;
            }
        }
    }
    if failed {
        process::exit(1);
    }
}
