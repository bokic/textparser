#!/usr/bin/env python3

from textparser import TextParser
import json
import sys

def main(args):

    if len(args) < 2:
        print("Usage: parse.py <definition_file> <text_file>")
        sys.exit(1)

    parser = TextParser(args[0])

    with open(args[1], "r") as f:
        text = f.read()
    tree = parser.parse(text)

    print(json.dumps(tree, indent=4))

if __name__ == "__main__":
    main(sys.argv[1:])
