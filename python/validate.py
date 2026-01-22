#!/usr/bin/env python3

from textparser import TextParser
import subprocess
import json
import sys
import os

def compareTrees(pythonTree, cTree):
    if len(pythonTree) != len(cTree):
        print("Length mismatch. Python: " + str(len(pythonTree)) + ", C: " + str(len(cTree)))
        return False
    for i in range(len(pythonTree)):
        if pythonTree[i]["id"] != cTree[i]["id"]:
            print("ID mismatch.")
            print("ID - Python: " + str(pythonTree[i]["id"]) + ", C: " + str(cTree[i]["id"]))
            print("Position - Python: " + str(pythonTree[i]["position"]) + ", C: " + str(cTree[i]["position"]))
            print("Length - Python: " + str(pythonTree[i]["length"]) + ", C: " + str(cTree[i]["length"]))
            return False
        if pythonTree[i]["position"] != cTree[i]["position"]:
            print("Position mismatch.")
            print("ID - Python: " + str(pythonTree[i]["id"]) + ", C: " + str(cTree[i]["id"]))
            print("Position - Python: " + str(pythonTree[i]["position"]) + ", C: " + str(cTree[i]["position"]))
            print("Length - Python: " + str(pythonTree[i]["length"]) + ", C: " + str(cTree[i]["length"]))
            return False
        if pythonTree[i]["length"] != cTree[i]["length"]:
            print("Length mismatch. Python: " + str(pythonTree[i]["length"]) + ", C: " + str(cTree[i]["length"]))
            print("ID - Python: " + str(pythonTree[i]["id"]) + ", C: " + str(cTree[i]["id"]))
            print("Position - Python: " + str(pythonTree[i]["position"]) + ", C: " + str(cTree[i]["position"]))
            print("Length - Python: " + str(pythonTree[i]["length"]) + ", C: " + str(cTree[i]["length"]))
            return False
        if len(cTree[i]["children"]) > 0:
            if not compareTrees(pythonTree[i]["children"], cTree[i]["children"]):
                return False
    return True

def recursiveParseDirectory(definition_file, dir):
    parser = TextParser(definition_file)
    for file in os.listdir(dir):
        if file.endswith(".cfm") or file.endswith(".cfc"):
            fullPathName = dir + "/" + file
            print("Comparing " + fullPathName + "...", end="")
            with open(fullPathName, "rb") as f:
                    text = f.read().decode('latin-1', errors='ignore')

            pythonTree = parser.parse(text)
            cTree = json.loads(subprocess.run(["bin/textparser", fullPathName, "--json"], capture_output=True, text=True).stdout)

            same = compareTrees(pythonTree, cTree)
            if not same:
                sys.exit(1)

            print(" done")
        elif os.path.isdir(dir + "/" + file):
            recursiveParseDirectory(definition_file, dir + "/" + file)

def main(args):

    if len(args) < 2:
        print("Usage: validate.py <definition_file> <directory>")
        sys.exit(1)

    definition_file = args[0]
    directory = args[1]

    recursiveParseDirectory(definition_file, directory)

if __name__ == "__main__":
    main(sys.argv[1:])
