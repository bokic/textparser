#!/usr/bin/env python3

from textparser import TextParser
import json
import sys

def __printUsage():
    print("Usage: parse.py <definition_file> [<text_file> --format] | [--stdinformat]")
    sys.exit(1)

def main(args):

    if len(args) < 2:
        __printUsage()

    parser = TextParser(args[0])

    if len(args) == 2 and args[1] == "--stdinformat":
        text = sys.stdin.read()
        chars = parser.parseFormat(text)
        print(chars.hex())
    elif len(args) == 2:
        with open(args[1], "r") as f:
            text = f.read()
        tree = parser.parse(text)
        print(json.dumps(tree, indent=4))
    elif len(args) == 3 and args[2] == "--format":
        with open(args[1], "r") as f:
            text = f.read()
        chars = parser.parseFormat(text)
        print(chars.hex())
    else:
        __printUsage()

if __name__ == "__main__":
    main(sys.argv[1:])
