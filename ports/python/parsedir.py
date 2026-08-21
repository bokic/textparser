#!/usr/bin/env python3

from textparser import TextParser
import time
import sys
import os

def recursiveParseDirectory(definition_file, dir):
    parser = TextParser(definition_file)
    for file in os.listdir(dir):
        if file.endswith(".cfm") or file.endswith(".cfc"):
            print("Parsing " + dir + "/" + file + "...", end="")
            with open(dir + "/" + file, "r", encoding='latin-1') as f:
                text = f.read()
            start_ns = time.perf_counter_ns()
            tree = parser.parse(text)
            end_ns = time.perf_counter_ns()
            elapsed_ns = end_ns - start_ns
            print(f" done in {elapsed_ns / 1e6:.3f} ms")
        elif os.path.isdir(dir + "/" + file):
            recursiveParseDirectory(definition_file, dir + "/" + file)

def main(args):

    if len(args) < 2:
        print("Usage: parse.py <definition_file> <directory>")
        sys.exit(1)

    definition_file = args[0]
    directory = args[1]

    recursiveParseDirectory(definition_file, directory)

if __name__ == "__main__":
    main(sys.argv[1:])
