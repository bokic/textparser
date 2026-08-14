import sys
import json
import re

class TextParser:
    def __init__(self, definitionFile: str):

        with open(definitionFile, "r") as f:
            fileContent = f.read()

        self.definition = json.loads(fileContent)

        if self.definition.get('version') is None:
            self.definition['version'] = '0.0'

        for token in self.definition['tokens']:
            if self.definition['tokens'][token].get('otherTextInside') is None:
                self.definition['tokens'][token]['otherTextInside'] = False

            if self.definition['tokens'][token].get('deleteIfOnlyOneChild') is None:
                self.definition['tokens'][token]['deleteIfOnlyOneChild'] = False

            if self.definition['tokens'][token].get('mustHaveOneChild') is None:
                self.definition['tokens'][token]['mustHaveOneChild'] = False

            if self.definition['tokens'][token].get('multiLine') is None:
                self.definition['tokens'][token]['multiLine'] = False

            if self.definition['tokens'][token].get('searchParentEndTokenLast') is None:
                self.definition['tokens'][token]['searchParentEndTokenLast'] = False

            if self.definition['tokens'][token].get('endRegex') is None:
                self.definition['tokens'][token]['endRegex'] = None

            if self.definition['tokens'][token].get('nestedTokens') is None:
                self.definition['tokens'][token]['nestedTokens'] = None

        self.sign_merge = self.definition.get('mergeSignIntoNumber')

    def __maybeMergeSign(self, tokenItem):
        if not self.sign_merge:
            return
        
        sign_tokens = self.sign_merge.get('signTokens', [])
        number_tokens = self.sign_merge.get('numberTokens', [])
        operand_tokens = self.sign_merge.get('operandTokens', [])

        if tokenItem['id'] not in number_tokens:
            return

        children = tokenItem.get('children')
        if not children:
            return

        i = 0
        while i < len(children):
            curr = children[i]
            if curr['id'] in number_tokens and i > 0:
                prev = children[i - 1]
                sign = None
                context = None

                if prev['id'] in sign_tokens:
                    sign = prev
                    context = children[i - 2] if i >= 2 else None
                elif prev.get('children') and prev['children'][-1]['id'] in sign_tokens:
                    sign = prev['children'][-1]
                    context = prev['children'][-2] if len(prev['children']) >= 2 else (children[i - 2] if i >= 2 else None)

                if sign and (sign['position'] + sign['length'] == curr['position']) and sign['length'] == 1:
                    # Unary context check: context must not be an operand
                    if context is None or context['id'] not in operand_tokens:
                        # Absorb sign into number
                        curr['length'] += curr['position'] - sign['position']
                        curr['position'] = sign['position']

                        # Remove sign token
                        if prev['id'] in sign_tokens:
                            children.pop(i - 1)
                            i -= 1
                        else:
                            prev['children'].pop()
                            if len(prev['children']) == 0:
                                children.pop(i - 1)
                                i -= 1

            if curr.get('children'):
                self.__maybeMergeSign(curr)
            i += 1

    def postProcess(self, tokens):
        i = 0
        while i < len(tokens):
            curr = tokens[i]
            if curr.get('children'):
                self.postProcess(curr['children'])

            tokenDef = self.definition['tokens'].get(curr['id'], {})
            if tokenDef.get('deleteIfOnlyOneChild') and len(curr.get('children', [])) == 1:
                only_child = curr['children'][0]
                tokens[i] = only_child
                curr = only_child
            i += 1

    def __skipWhitespace(self, text, pos):
        while pos < len(text) and (text[pos].isspace() or text[pos] == '\t' or text[pos] == "\n" or text[pos] == "\r"):
            pos += 1
        return pos

    def __parseGroup(self, text, tokenName, token, parentRegex, pos):

        pos = self.__skipWhitespace(text, pos)

        ret = {}
        ret['id'] = tokenName
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        while True:
            endTokenPos = sys.maxsize
            pos = self.__skipWhitespace(text, pos)

            closestChildTokenPos = sys.maxsize
            closestChildTokenName = None

            if (not token['searchParentEndTokenLast']):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]

            for childTokenName in token['nestedTokens']:
                childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], self.definition['otherTextInside'])
                if (childTokenPos is not None):
                    if (childTokenPos < closestChildTokenPos):
                        closestChildTokenPos = childTokenPos
                        closestChildTokenName = childTokenName
                        if (closestChildTokenPos == 0):
                            break

            if (closestChildTokenPos > 0) and (token['searchParentEndTokenLast']):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]

            shouldBreak = False
            if (endTokenPos != sys.maxsize) and (endTokenPos <= closestChildTokenPos):
                 shouldBreak = True

            if shouldBreak:
                ret['length'] = pos + endTokenPos - ret['position']
                break

            if ((closestChildTokenPos == sys.maxsize)or(closestChildTokenName is None)):
                break

            if (closestChildTokenPos > 0) and (self.definition['tokens'][tokenName]['otherTextInside'] == False):
                raise Exception("Child token " + closestChildTokenName + " has illegal position!")

            pos += closestChildTokenPos

            child = self.__parseToken(text, closestChildTokenName, self.definition['tokens'][closestChildTokenName], parentRegex, pos)
            if (child['position'] < pos):
                raise Exception("Child token " + closestChildTokenName + " has illegal position!")

            if (child['length'] <= 0):
                raise Exception("Child token " + closestChildTokenName + " has illegal length!")

            ret['length'] = child['position'] + child['length'] - ret['position']
            if (ret['length'] <= 0):
                raise Exception("Child token " + closestChildTokenName + " has illegal length!")

            ret['children'].append(child)

            pos = child['position'] + child['length']

        return ret

    def __parseGroupOneChildOnly(self, text, tokenName, token, parentRegex, pos):

        pos = self.__skipWhitespace(text, pos)

        ret = {}
        ret['id'] = tokenName
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        if (token['nestedTokens'] is None) or (len(token['nestedTokens']) == 0):
            raise Exception("GroupOneChildOnly token type nested_tokens list is empty!")

        closestChildTokenPos = sys.maxsize
        closestChildTokenName = None

        for childTokenName in token['nestedTokens']:
            childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], token['otherTextInside'])
            if (childTokenPos is not None) and (childTokenPos >= 0) and (childTokenPos < closestChildTokenPos):
                closestChildTokenPos = childTokenPos
                closestChildTokenName = childTokenName

        if (closestChildTokenName is None):
            raise Exception("Search for GroupOneChildOnly token type failed. Can't find one child.")

        child = self.__parseToken(text, closestChildTokenName, self.definition['tokens'][closestChildTokenName], parentRegex, pos)

        ret['position'] = child['position']
        ret['length'] = child['length']
        ret['children'].append(child)

        return ret

    def __parseGroupAllChildrenInSameOrder(self, text, tokenName, token, parentRegex, pos):

        pos = self.__skipWhitespace(text, pos)

        ret = {}
        ret['id'] = tokenName
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        if (len(token['nestedTokens']) != 3):
            raise Exception("GroupAllChildrenInSameOrder should have exactly 3 nested tokens, but " + str(len(token['nestedTokens'])) + " were found")

        startToken = token['nestedTokens'][0]
        innerToken = token['nestedTokens'][1]
        endToken = token['nestedTokens'][2]

        startTokenPos = self.__findToken(text, pos, self.definition['tokens'][startToken], self.definition['otherTextInside'])
        if (startTokenPos is None):
            raise Exception("Expected " + startToken + " at position: " + str(pos))

        child = self.__parseToken(text, startToken, self.definition['tokens'][startToken], parentRegex, pos)

        ret['length'] = child['position'] + child['length'] - ret['position']
        ret['children'].append(child)
        pos = child['position'] + child['length']

        parentRegex = self.definition['tokens'][endToken]['startRegex']

        while True:
            innerTokenPos = self.__findToken(text, pos, self.definition['tokens'][innerToken], self.definition['otherTextInside'])
            endTokenPos = self.__findToken(text, pos, self.definition['tokens'][endToken], self.definition['otherTextInside'])

            if (endTokenPos is None):
                raise Exception("GroupAllChildrenInSameOrder end token " + endToken + " not found")

            if (innerTokenPos is None):
                break

            if (endTokenPos < innerTokenPos):
                break

            if (endTokenPos == innerTokenPos) and (not token['searchParentEndTokenLast']):
                break

            pos += innerTokenPos

            child = self.__parseToken(text, innerToken, self.definition['tokens'][innerToken], parentRegex, pos)

            ret['length'] = child['position'] + child['length'] - ret['position']
            ret['children'].append(child)
            pos = child['position'] + child['length']

        pos += endTokenPos

        endToken = self.__parseToken(text, endToken, self.definition['tokens'][endToken], parentRegex, pos)

        ret['length'] = endToken['position'] + endToken['length'] - ret['position']
        ret['children'].append(endToken)

        return ret

    def __parseSimpleToken(self, text, tokenName, token, pos):

        pos = self.__skipWhitespace(text, pos)

        startRegex = re.match(token['startRegex'], text[pos:], flags=re.IGNORECASE)
        if (startRegex is None):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        if (len(startRegex.regs) < 1) or (len(startRegex.regs) > 2):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        ret = {}
        ret['id'] = tokenName
        ret['position'] = pos + startRegex.regs[len(startRegex.regs) - 1][0]
        ret['length'] = startRegex.regs[len(startRegex.regs) - 1][1] - startRegex.regs[len(startRegex.regs) - 1][0]

        return ret

    def __parseStartStop(self, text, tokenName, token, parentRegex, pos, endRequired):

        myEndRegex = token['endRegex']

        pos = self.__skipWhitespace(text, pos)

        startRegex = re.match(token['startRegex'], text[pos:], flags=re.IGNORECASE)
        if (startRegex is None):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        if (len(startRegex.regs) < 1) or (len(startRegex.regs) > 2):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        pos += startRegex.regs[len(startRegex.regs) - 1][0]

        ret = {}
        ret['id'] = tokenName
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        pos += startRegex.regs[len(startRegex.regs) - 1][1]

        if (token['nestedTokens'] is None):
            endRegex = re.search(myEndRegex, text[pos:], flags=re.IGNORECASE)
            if (endRegex is None):
                raise Exception("Expected " + myEndRegex + " at position: " + str(pos))
            endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
            endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
            ret['length'] = pos + endTokenPos + endTokenLength - ret['position']
            return ret

        while True:
            endTokenPos = sys.maxsize
            endTokenLength = 0
            pos = self.__skipWhitespace(text, pos)

            checkRegex = parentRegex if token['searchParentEndTokenLast'] and parentRegex is not None else myEndRegex

            if (not token['searchParentEndTokenLast']):
                if checkRegex is not None:
                    endRegex = re.search(checkRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                        endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                        if (endTokenPos == 0):
                            ret['length'] = pos - ret['position'] + endTokenLength
                            break

            closestChildTokenPos = sys.maxsize
            closestChildTokenName = None

            for childTokenName in token['nestedTokens']:
                childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], token['otherTextInside'])
                if (childTokenPos is not None):
                    if (childTokenPos < closestChildTokenPos):
                        closestChildTokenPos = childTokenPos
                        closestChildTokenName = childTokenName
                        if (closestChildTokenPos == 0):
                            break

            if (token['searchParentEndTokenLast']):
                if checkRegex is not None:
                    endRegex = re.search(checkRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                        endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                        if (endTokenPos < closestChildTokenPos):
                            ret['length'] = pos - ret['position'] + endTokenPos + endTokenLength
                            break

            if (endTokenPos < closestChildTokenPos):
                ret['length'] = pos - ret['position'] + endTokenPos + endTokenLength
                break

            if (closestChildTokenPos == sys.maxsize) or (closestChildTokenName is None):
                if endRequired and checkRegex is not None:
                    endRegex = re.search(checkRegex, text[pos:], flags=re.IGNORECASE)
                    if endRegex is not None:
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                        endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                        ret['length'] = pos - ret['position'] + endTokenPos + endTokenLength
                break

            if (closestChildTokenPos > 0) and (self.definition['tokens'][tokenName]['otherTextInside'] == False):
                raise Exception("Child token " + closestChildTokenName + " has illegal position!")

            pos += closestChildTokenPos

            child = self.__parseToken(text, closestChildTokenName, self.definition['tokens'][closestChildTokenName], myEndRegex, pos)
            if (child["length"] == 0):
                raise Exception("Child token " + closestChildTokenName + " has no length!")

            ret['length'] = child['position'] + child['length'] - ret['position']

            ret['children'].append(child)

            pos = child['position'] + child['length']

        return ret

    def __parseToken(self, text: str, tokenName: str, token: dict, parentRegex: str | None, pos: int):

        pos = self.__skipWhitespace(text, pos)

        match token['type']:
            case "Group":
                return self.__parseGroup(text, tokenName, token, parentRegex, pos)
            case "GroupOneChildOnly":
                return self.__parseGroupOneChildOnly(text, tokenName, token, parentRegex, pos)
            case "GroupAllChildrenInSameOrder":
                return self.__parseGroupAllChildrenInSameOrder(text, tokenName, token, parentRegex, pos)
            case "SimpleToken":
                return self.__parseSimpleToken(text, tokenName, token, pos)
            case "StartStop":
                return self.__parseStartStop(text, tokenName, token, parentRegex, pos, True)
            case "StartOptStop":
                return self.__parseStartStop(text, tokenName, token, parentRegex, pos, False)

        raise Exception("Unknown token type: " + token["type"])

    def __findToken(self, text, pos, token, otherTextInside):
        match token["type"]:
            case "GroupOneChildOnly" | "Group":
                closestChildPos = sys.maxsize
                for childTokenName in token['nestedTokens']:
                    childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], otherTextInside)
                    if (childTokenPos is not None):
                        if (childTokenPos < closestChildPos):
                            closestChildPos = childTokenPos
                if (closestChildPos == sys.maxsize):
                    return None
                return closestChildPos
            case "GroupAllChildrenInSameOrder":
                return self.__findToken(text, pos, self.definition['tokens'][token['nestedTokens'][0]], otherTextInside)
            case "SimpleToken" | "StartStop" | "StartOptStop":
                res = re.search(token["startRegex"], text[pos:], flags=re.IGNORECASE)
                if (res is None):
                    return None
                if (res.regs[len(res.regs) - 1][1] == 0):
                    return None
                if ((not otherTextInside)and(res.pos != 0)):
                    return None

                return res.regs[len(res.regs) - 1][0]

        raise Exception("Fatal error: Unknown token type: " + token["type"])

    def parse(self, text):

        tokens = []

        pos = 0

        while pos < len(text):
            pos = self.__skipWhitespace(text, pos)
            closestTokenPos = sys.maxsize
            closestTokenName = None

            for tokenName in self.definition['startTokens']:
                token = self.definition['tokens'][tokenName]
                offset = self.__findToken(text, pos, token, self.definition['otherTextInside'])
                if (offset is not None):
                    if offset < closestTokenPos:
                        closestTokenPos = offset
                        closestTokenName = tokenName

            if (closestTokenName is None):
                if self.definition.get('otherTextInside') and pos < len(text):
                    tokens.append({'id': '', 'position': pos, 'length': len(text) - pos})
                break

            if closestTokenPos > 0 and self.definition.get('otherTextInside'):
                errEnd = pos + closestTokenPos
                while errEnd > pos and text[errEnd - 1].isspace():
                    errEnd -= 1
                if errEnd > pos:
                    tokens.append({'id': '', 'position': pos, 'length': errEnd - pos})

            pos += closestTokenPos

            child = self.__parseToken(text, closestTokenName, self.definition['tokens'][closestTokenName], None, pos)
            if (child["length"] == 0):
                raise Exception("Child token " + closestTokenName + " has no length!")

            tokens.append(child)
            pos = child['position'] + child['length']

        if self.definition.get('mergeSignIntoNumber'):
            dummyRoot = {'id': 'ROOT', 'children': tokens}
            self.__maybeMergeSign(dummyRoot)

        self.postProcess(tokens)

        return tokens

    def __recursiveFormat(self, array, token):
        tokenId = list(self.definition['tokens'].keys()).index(token['id'])
        char = tokenId.to_bytes(1)
        array[token['position']:token['position'] + token['length']] = char * token['length']

        if ('children' in token):
            for child in token['children']:
                self.__recursiveFormat(array, child)

    def parseFormat(self, text):
        ret = bytearray()

        ret.extend(b'\x00' * len(text))

        tokens = self.parse(text)

        for token in tokens:
            self.__recursiveFormat(ret, token)

        return ret
