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

def recursiveParseDirectory(definition_file, target_path):
    parser = TextParser(definition_file)
    exts = tuple("." + ext for ext in parser.definition.get("defaultFileExtensions", ["cfm", "cfc"]))

    def process_file(fullPathName):
        print("Comparing " + fullPathName + "...", end="")
        with open(fullPathName, "rb") as f:
            text = f.read().decode('latin-1', errors='ignore')

        pythonTree = parser.parse(text)
        cTree = json.loads(subprocess.run(["bin/textparser", fullPathName, "--definition", definition_file, "--json"], capture_output=True, text=True).stdout)

        same = compareTrees(pythonTree, cTree)
        if not same:
            sys.exit(1)

        print(" done")

    if os.path.isfile(target_path):
        process_file(target_path)
        return

    for file in os.listdir(target_path):
        fullPathName = os.path.join(target_path, file)
        if file.endswith(exts) or file.endswith(".py"):
            process_file(fullPathName)
        elif os.path.isdir(fullPathName):
            recursiveParseDirectory(definition_file, fullPathName)

def main(args):

    if len(args) < 2:
        print("Usage: validate.py <definition_file> <directory>")
        sys.exit(1)

    definition_file = args[0]
    directory = args[1]

    recursiveParseDirectory(definition_file, directory)

if __name__ == "__main__":
    main(sys.argv[1:])
