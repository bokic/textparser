using System.Text.RegularExpressions;

namespace TextParser
{
    public class Parser(Language lang)
    {
        private Language m_lang = lang;

        private static int SkipWhitespace(string text, int pos)
        {
            var maxPos = text.Length;

            for (int c = pos; c < maxPos; c++)
            {
                var ch = text[c];

                if ((ch != ' ') && (ch != '\t') && (ch != '\n') && (ch != '\r'))
                    return c;
            }

            return maxPos;
        }

        private int FindToken(string text, string tokenName, int pos, bool otherTextInside)
        {
            int ret = -1;

            var token = m_lang.tokens[tokenName];

            switch (token.type)
            {
                case Type.GroupOneChildOnly:
                case Type.Group:
                    int lowest = int.MaxValue;
                    foreach (var child in token.nestedTokens)
                    {
                        int tmp = FindToken(text, child, pos, token.otherTextInside);
                        if (tmp == pos)
                            return pos;
                        if ((tmp > pos) && (tmp < lowest))
                            lowest = tmp;
                    }
                    break;

                case Type.GroupAllChildrenInSameOrder:
                    return FindToken(text, token.nestedTokens[0], pos, token.otherTextInside);

                case Type.SimpleToken:
                case Type.StartStop:
                case Type.StartOptStop:
                case Type.DialStartAndStop:
                    int size = text.IndexOf('\n', pos);
                    if (size < pos)
                        size = text.Length - pos;
                    else
                        size = size - pos;

                    Match match = token.startRegex.Match(text, pos, size);
                    if ((match.Success)&&(match.Length > 0))
                        return match.Index;
                    break;
            }

            return ret;
        }

        private TokenResultTree ParseToken(string text, string tokenName, int pos, bool otherTextInside, string? parentEndTokenName, string? nextTokenName)
        {
            TokenResultTree ret = new();
            string? childTokenName = null;
            Match match;

            var token = m_lang.tokens[tokenName];

            //Console.WriteLine(tokenName + " at pos " + pos);

            ret.name = tokenName;

            switch (token.type)
            {
                case Type.GroupOneChildOnly:
                    foreach (var nestedTokenStr in token.nestedTokens)
                    {
                        pos = SkipWhitespace(text, pos);

                        if (FindToken(text, nestedTokenStr, pos, otherTextInside) >= pos)
                        {
                            string? localParentEndTokenName;
                            if (token.endRegex != null)
                                localParentEndTokenName = tokenName;
                            else
                                localParentEndTokenName = parentEndTokenName;

                            var child = ParseToken(text, nestedTokenStr, pos, otherTextInside, localParentEndTokenName, nextTokenName);

                            if (ret.children == null)
                                ret.children = new();
                            ret.children.Add(child);

                            ret.size = child.position + child.size - ret.position;
                            pos = child.position + child.size;
                            break;
                        }
                    }
                    break;

                case Type.Group:
                    pos = SkipWhitespace(text, pos);
                    ret.position = pos;

                    do
                    {
                        int parentTokenPos = int.MaxValue;
                        int nextTokenPos = int.MaxValue;

                        pos = SkipWhitespace(text, pos);

                        if (parentEndTokenName != null)
                        {
                            var parentEndToken = m_lang.tokens[parentEndTokenName];
                            match = parentEndToken.endRegex.Match(text, pos);
                            if ((match.Success) && (match.Length > 0))
                            {
                                if (match.Index == pos)
                                {
                                    break;
                                }
                                parentTokenPos = match.Index;
                            }
                        }

                        if (nextTokenName != null)
                        {
                            int tmpPos = FindToken(text, nextTokenName, pos, token.otherTextInside);
                            if (tmpPos == pos)
                            {
                                break;
                            }

                            if (tmpPos >= 0)
                            {
                                nextTokenPos = tmpPos;
                            }
                        }

                        int closestChildToken = int.MaxValue;
                        childTokenName = null;

                        foreach (var nestedTokenStr in token.nestedTokens)
                        {
                            int childrenPos = FindToken(text, nestedTokenStr, pos, token.otherTextInside);
                            if (childrenPos == -1)
                                continue;

                            if (childrenPos == pos)
                            {
                                childTokenName = nestedTokenStr;
                                closestChildToken = childrenPos;
                                break;
                            }

                            if ((childrenPos < closestChildToken) && (childrenPos < parentTokenPos))
                            {
                                childTokenName = nestedTokenStr;
                                closestChildToken = childrenPos;
                            }
                        }

                        if ((childTokenName == null) || (parentTokenPos < closestChildToken) || (nextTokenPos < closestChildToken))
                            break;

                        if (token.otherTextInside == true)
                            pos = closestChildToken;

                        string? localParentEndTokenName;
                        if (token.endRegex != null)
                            localParentEndTokenName = tokenName;
                        else
                            localParentEndTokenName = parentEndTokenName;

                        var child = ParseToken(text, childTokenName, pos, otherTextInside, localParentEndTokenName, nextTokenName);

                        if (ret.children == null)
                            ret.children = new();
                        ret.children.Add(child);
                        pos = child.position + child.size;
                    } while (true);
                    if (ret.children != null)
                        ret.size = ret.children.Last().position + ret.children.Last().size - ret.position;
                    break;

                case Type.GroupAllChildrenInSameOrder:
                    int c = 0;
                    foreach (var nestedTokenStr in token.nestedTokens)
                    {
                        pos = SkipWhitespace(text, pos);

                        if (c < token.nestedTokens.Length - 1)
                        {
                            nextTokenName = token.nestedTokens[c + 1];
                        }
                        else
                        {
                            nextTokenName = null;
                        }

                        string? localParentEndTokenName;
                        if (token.endRegex != null)
                            localParentEndTokenName = tokenName;
                        else
                            localParentEndTokenName = parentEndTokenName;

                        var child = ParseToken(text, nestedTokenStr, pos, otherTextInside, localParentEndTokenName, nextTokenName);

                        if (ret.children == null)
                            ret.children = new();
                        ret.children.Add(child);

                        ret.size = child.position + child.size - ret.position;
                        pos = child.position + child.size;
                        c++;
                    }
                    break;

                case Type.SimpleToken:
                    match = token.startRegex.Match(text, pos);
                    if ((match.Success == false)||(match.Length <= 0))
                        throw new Exception("Could not match " + tokenName + " token of type SimpleToken at position " + pos);
                    if ((match.Index > pos) && (token.otherTextInside == false))
                        throw new Exception("Found junk at " + pos);
                    ret.position = match.Index;
                    ret.size = match.Length;
                    break;

                case Type.StartStop:
                case Type.StartOptStop:
                    match = token.startRegex.Match(text, pos);
                    if ((match.Success == false)||(match.Length <= 0))
                        throw new Exception("Could not match start of " + tokenName + " token of type SimpleToken at position " + pos);
                    if (((match.Index - pos) > pos) && (token.otherTextInside == false))
                        throw new Exception("Found junk at " + pos);
                    ret.position = match.Index;
                    pos = match.Index + match.Length;

                    if (token.nestedTokens != null)
                    {
                        do
                        {
                            pos = SkipWhitespace(text, pos);

                            int closestTokenPos = pos;

                            var endTokenMatch = token.endRegex.Match(text, pos);
                            if ((endTokenMatch.Success == true) && (endTokenMatch.Index == pos) && (endTokenMatch.Length > 0))
                                break;

                            int closestChildToken = int.MaxValue;
                            childTokenName = null;

                            foreach (var nestedTokenStr in token.nestedTokens)
                            {
                                int childrenPos = FindToken(text, nestedTokenStr, pos, token.otherTextInside);
                                if (childrenPos == -1)
                                    continue;

                                if (childrenPos == pos)
                                {
                                    childTokenName = nestedTokenStr;
                                    closestChildToken = childrenPos;
                                    break;
                                }

                                if ((token.otherTextInside == true)&&(childrenPos < closestChildToken)&&(childrenPos < endTokenMatch.Index))
                                {
                                    childTokenName = nestedTokenStr;
                                    closestChildToken = childrenPos;
                                }
                            }

                            if (childTokenName == null)
                                break;

                            string? localParentEndTokenName;
                            if (token.endRegex != null)
                                localParentEndTokenName = tokenName;
                            else
                                localParentEndTokenName = parentEndTokenName;

                            var child = ParseToken(text, childTokenName, closestChildToken, token.otherTextInside, localParentEndTokenName, null);

                            if (ret.children == null)
                                ret.children = new();
                            ret.children.Add(child);
                            ret.size = child.position + child.size - ret.position;

                            pos = child.position + child.size;
                        } while (true);
                    }

                    pos = SkipWhitespace(text, pos);

                    match = token.endRegex.Match(text, pos);
                    if ((match.Success == false)||(match.Length <= 0))
                    {
                        if (token.type == Type.StartOptStop)
                            break;

                        throw new Exception("Could not match end of " + tokenName + " token of type SimpleToken at position " + pos);
                    }

                    if ((match.Index > pos) && (token.otherTextInside == false))
                        throw new Exception("Found junk at " + pos);

                    ret.size = match.Index + match.Length - ret.position;
                    break;

                case Type.DialStartAndStop:
                    throw new Exception("DialStartAndStop token type not implemented yet!");
            }

            Console.WriteLine("Found " + ret.name + ",at " + ret.position + ",size " + ret.size);

            return ret;
        }

        public List<TokenResultTree> ParseText(string text)
        {
            List<TokenResultTree> ret = [];

            var maxPos = text.Length;
            int pos = 0;

            do
            {
                pos = SkipWhitespace(text, pos);

                int closestTokenPos = int.MaxValue;
                string? closestTokenName = null;

                foreach (var tokenName in m_lang.startTokens)
                {
                    int foundAt = FindToken(text, tokenName, pos, m_lang.otherTextInside);
                    if ((foundAt >= pos) && (foundAt < closestTokenPos))
                    {
                        closestTokenPos = foundAt;
                        closestTokenName = tokenName;
                        if (foundAt == pos)
                            break;
                    }
                }

                if (pos == maxPos)
                    break;

                if (((closestTokenPos > pos) || (closestTokenName == null)) && (m_lang.otherTextInside == false))
                    throw new Exception("Junk found at pos " + pos + "!");

                if (closestTokenName == null)
                    break;

                var newToken = ParseToken(text, closestTokenName, closestTokenPos, m_lang.otherTextInside, null, null);

                if (newToken.size == 0)
                    throw new Exception("newToken.size!");

                ret.Add(newToken);

                pos = newToken.position + newToken.size;

            } while (pos < maxPos);

            return ret;
        }

        public void ParseLine()
        {
            // TODO: Not Implemented ParseLine()!

            //var lines = Regex.Split(text, "\r\n|\r|\n");
        }
    }
}
