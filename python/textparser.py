import sys
import json
import re

class TextParser:
    def __init__(self, definitionFile):
        
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

    def __skipWhitespace(self, text, pos):
        while pos < len(text) and (text[pos].isspace() or text[pos] == '\t' or text[pos] == "\n" or text[pos] == "\r"):
            pos += 1
        return pos

    def __parseTokenGroupOneChildOnly(self, text, token_id, token, parentToken, pos):
        raise Exception("Not implemented __parseTokenGroupOneChildOnly")

    def __parseGroup(self, text, token_id, token, parentRegex, pos):

        pos = self.__skipWhitespace(text, pos)

        ret = {}
        ret['id'] = token_id
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        while True:
            endTokenPos = sys.maxsize
            pos = self.__skipWhitespace(text, pos)

            closestchildTokenPos = sys.maxsize
            closestchildTokenName = None

            if ((token.get('searchParentEndTokenLast') is None)or(token.get('searchParentEndTokenLast') is not None and token['searchParentEndTokenLast'] == False)):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[0][0]
                        endTokenLength = endRegex.regs[0][1] - endTokenPos
                        if (endTokenPos == 0):
                            ret['length'] = pos - ret['position']
                            break
                
            for childTokenName in token['nestedTokens']:
                childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], self.definition['otherTextInside'])
                if (childTokenPos is not None):
                    if (childTokenPos < closestchildTokenPos):
                        closestchildTokenPos = childTokenPos
                        closestchildTokenName = childTokenName
                        if (closestchildTokenPos == 0):
                            break
            
            if (closestchildTokenPos > 0) and (token.get('searchParentEndTokenLast') is not None and token['searchParentEndTokenLast'] == True):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[0][0]
                        endTokenLength = endRegex.regs[0][1] - endTokenPos
                        if (endTokenPos == 0):
                            ret['length'] = pos - ret['position']
                            break

            if (endTokenPos < closestchildTokenPos):
                ret['length'] = pos + endTokenPos - ret['position']
                break

            if (closestchildTokenPos == sys.maxsize):
                break

            pos += closestchildTokenPos

            child = self.__parseToken(text, closestchildTokenName, self.definition['tokens'][closestchildTokenName], parentRegex, pos)
            if (child["length"] == 0):
                raise Exception("Child token " + closestchildTokenName + " has no length!")

            ret['length'] = child['position'] + child['length'] - ret['position']

            ret['children'].append(child)

            pos = child['position'] + child['length']

        return ret

    def __parseGroupAllChildrenInSameOrder(self, text, token_id, token, parentRegex, pos):
        
        pos = self.__skipWhitespace(text, pos)

        ret = {}
        ret['id'] = token_id
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        for c in range(0, len(token['nestedTokens'])):
            pos = self.__skipWhitespace(text, pos)

            childTokenName = token['nestedTokens'][c]
            childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], self.definition['otherTextInside'])
            if (childTokenPos is None):
                raise Exception("Expected " + childTokenName + " at position: " + str(pos))

            pos += childTokenPos

            if c == len(token['nestedTokens']) - 1:
                currentparentRegex = parentRegex
            else:
                nextChildTokenName = token['nestedTokens'][c + 1]
                if (self.definition['tokens'][nextChildTokenName].get('startRegex') is not None):
                    currentparentRegex = self.definition['tokens'][nextChildTokenName]['startRegex']
                else:
                    currentparentRegex = None

            child = self.__parseToken(text, childTokenName, self.definition['tokens'][childTokenName], currentparentRegex, pos)
            
            ret['length'] = child['position'] + child['length'] - ret['position']

            ret['children'].append(child)
            
            pos = child['position'] + child['length']

        return ret

    def __parseSimpleToken(self, text, token_id, token, parentRegex, pos):

        pos = self.__skipWhitespace(text, pos)

        startRegex = re.match(token['startRegex'], text[pos:], flags=re.IGNORECASE)
        if (startRegex is None):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        if (len(startRegex.regs) < 1) or (len(startRegex.regs) > 2):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        ret = {}
        ret['id'] = token_id
        ret['position'] = pos + startRegex.regs[len(startRegex.regs) - 1][0]
        ret['length'] = startRegex.regs[len(startRegex.regs) - 1][1] - startRegex.regs[len(startRegex.regs) - 1][0]

        return ret

    def __parseStartStop(self, text, token_id, token, parentRegex, pos, endRequired):

        if (token.get('endRegex') is not None):
            parentRegex = token['endRegex']

        pos = self.__skipWhitespace(text, pos)

        startRegex = re.match(token['startRegex'], text[pos:], flags=re.IGNORECASE)
        if (startRegex is None):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        if (len(startRegex.regs) < 1) or (len(startRegex.regs) > 2):
            raise Exception("Expected " + token['startRegex'] + " at position: " + str(pos))

        pos += startRegex.regs[len(startRegex.regs) - 1][0]

        ret = {}
        ret['id'] = token_id
        ret['position'] = pos
        ret['length'] = 0
        ret['children'] = []

        pos += startRegex.regs[len(startRegex.regs) - 1][1]

        if ((token.get('nestedTokens') == False) or (token['nestedTokens'] is None)):
            raise Exception("Token " + token['name'] + " has no nestedTokens!")

        while True:
            endTokenPos = sys.maxsize
            pos = self.__skipWhitespace(text, pos)

            if ((token.get('searchParentEndTokenLast') is None)or(token.get('searchParentEndTokenLast') is not None and token['searchParentEndTokenLast'] == False)):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                        endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                        if (endTokenPos == 0):
                            ret['length'] = pos - ret['position'] + endTokenLength
                            break

            closestchildTokenPos = sys.maxsize
            closestchildTokenName = None
                
            for childTokenName in token['nestedTokens']:
                childTokenPos = self.__findToken(text, pos, self.definition['tokens'][childTokenName], self.definition['otherTextInside'])
                if (childTokenPos is not None):
                    if (childTokenPos < closestchildTokenPos):
                        closestchildTokenPos = childTokenPos
                        closestchildTokenName = childTokenName
                        if (closestchildTokenPos == 0):
                            break
            
            if (token.get('searchParentEndTokenLast') is not None and token['searchParentEndTokenLast'] == True):
                if parentRegex is not None:
                    endRegex = re.search(parentRegex, text[pos:], flags=re.IGNORECASE)
                    if (endRegex is not None):
                        endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                        endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                        if (endTokenPos == 0):
                            ret['length'] = pos - ret['position'] + endTokenLength
                            break

            if (endTokenPos < closestchildTokenPos):
                ret['length'] = pos - ret['position'] + endTokenPos + endTokenLength
                break

            if closestchildTokenPos == sys.maxsize:
                break

            pos += closestchildTokenPos

            child = self.__parseToken(text, closestchildTokenName, self.definition['tokens'][closestchildTokenName], parentRegex, pos)
            if (child["length"] == 0):
                raise Exception("Child token " + closestchildTokenName + " has no length!")

            ret['length'] = child['position'] + child['length'] - ret['position']

            ret['children'].append(child)

            pos = child['position'] + child['length']

            pos = self.__skipWhitespace(text, pos)

            endRegex = re.search(token['endRegex'], text[pos:], flags=re.IGNORECASE)

            if (endRegex is None) and (endRequired == True):
                raise Exception("Expected endtoken for " + token_id + " at position: " + str(pos))

            if (endRegex is not None):
                endTokenPos = endRegex.regs[len(endRegex.regs) - 1][0]
                endTokenLength = endRegex.regs[len(endRegex.regs) - 1][1] - endTokenPos
                if (endTokenPos == 0):
                    ret['length'] += endTokenLength
                    break

        return ret

    def __parseDualStartStop(self, text, token_id, token, parentRegex, pos):
        raise Exception("Not implemented __parseDualStartStop")

    def __parseToken(self, text, token_id, token, parentRegex, pos):

        if (token.get('endRegex') is not None):
            parentRegex = token['endRegex']

        pos = self.__skipWhitespace(text, pos)

        match token['type']:
            case "GroupOneChildOnly":
                return self.__parseTokenGroupOneChildOnly(text, token_id, token, parentRegex, pos)
            case "Group":
                return self.__parseGroup(text, token_id, token, parentRegex, pos)
            case "GroupAllChildrenInSameOrder":
                return self.__parseGroupAllChildrenInSameOrder(text, token_id, token, parentRegex, pos)
            case "SimpleToken":
                return self.__parseSimpleToken(text, token_id, token, parentRegex, pos)
            case "StartStop":
                return self.__parseStartStop(text, token_id, token, parentRegex, pos, True)
            case "StartOptStop":
                return self.__parseStartStop(text, token_id, token, parentRegex, pos, False)
            case "DualStartStop":
                return self.__parseDualStartStop(text, token_id, token, parentRegex, pos)

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
            case "SimpleToken" | "StartStop" | "StartOptStop" | "DualStartStop":
                res = re.search(token["startRegex"], text[pos:], flags=re.IGNORECASE)
                if (res is None):
                    return None            
                if (res.regs[0][1] == 0):
                    return None
                if ((otherTextInside == False)and(res.pos != 0)):
                    return None
                
                return res.regs[0][0]

        raise Exception("Fatal error: Unknown token type: " + token["type"])

    def parse(self, text):
        
        tokens = []

        pos = 0

        while pos < len(text):
            pos = self.__skipWhitespace(text, pos)
            closestTokenPos = sys.maxsize
            closestTokenName = None

            for token_name in self.definition['startTokens']:
                token = self.definition['tokens'][token_name]
                offset = self.__findToken(text, pos, token, self.definition['otherTextInside'])
                if (offset is not None):
                    if offset < closestTokenPos:
                        closestTokenPos = offset
                        closestTokenName = token_name

            if (closestTokenName is None):
                break

            pos += closestTokenPos
            
            child = self.__parseToken(text, closestTokenName, self.definition['tokens'][closestTokenName], None, pos)
            if (child["length"] == 0):
                raise Exception("Child token " + closestTokenName + " has no length!")

            tokens.append(child)        
            pos = child['position'] + child['length']

        return tokens
