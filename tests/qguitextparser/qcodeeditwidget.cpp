#include "qcodeeditwidget.h"

#include <algorithm>

#include <QPainter>
#include <QApplication>
#include <QFontMetrics>
#include <QTimerEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QKeyEvent>
#include <QEvent>
#include <QImage>
#include <QColor>

#include <math.h>


enum QJavaCFParserTokenTypes {
    _EOF = 0,
    STAGO = 1,
    ETAGO = 2,
    EXTAGC = 3,
    EXTAGCEMPTY = 4,
    END_CFSCRIPT = 5,
    COMPONENT = 6,
    INTERFACE = 7,
    PCDATA = 8,
    PCDATA_NEWLINE = 9,
    HASHMARK = 10,
    ESCAPED_HASHMARK = 11,
    CFML_COMMENT_START = 12,
    CFML_COMMENT_END = 13,
    CLOSURE_OPEN_BRACE = 15,
    CLOSURE_CLOSE_BRACE = 16,
    JAVA_OPEN_BRACE = 18,
    JAVA_CLOSE_BRACE = 19,
    JAVA_DOUBLE_QUOTE = 20,
    CFIF = 22,
    CFELSEIF = 23,
    CFSET = 24,
    CFRETURN = 25,
    CFELSE = 26,
    CFLOOP = 27,
    CFBREAK = 28,
    CFCONTINUE = 29,
    CFTRY = 30,
    CFCATCH = 31,
    CFFINALLY = 32,
    CFRETHROW = 33,
    CFIMPORT = 34,
    CFARGUMENT = 35,
    CFFUNCTION = 36,
    CFCOMPONENT = 37,
    CFPROPERTY = 38,
    CFSWITCH = 39,
    CFINTERFACE = 40,
    CFCASE = 41,
    CFDEFAULTCASE = 42,
    CFPROCESSINGDIRECTIVE = 43,
    CFBODYTAG = 44,
    CFSCRIPT_STMT = 45,
    CFTAG = 46,
    MISC_HTML_ELEMENT = 47,
    EX_CFSCRIPT_TAGC = 48,
    TagAttrName = 49,
    TagAttrEquals = 50,
    BogusOctothorpe = 51,
    TagAttrOther = 52,
    TagAttrHash = 53,
    TagAttrDoubleQuote = 54,
    TagAttrSingleQuote = 55,
    StartDoubleQuoteStringLiteral = 56,
    StartSingleQuoteStringLiteral = 57,
    CFS_SINGLE_LINE_COMMENT_START = 58,
    CFS_FORMAL_COMMENT_START = 59,
    CFS_MULTI_LINE_COMMENT_START = 60,
    SINGLE_LINE_COMMENT = 61,
    FORMAL_COMMENT = 63,
    SINGLE_LINE_COMMENT_CLOSURE = 69,
    FORMAL_COMMENT_CLOSURE = 70,
    MULTI_LINE_COMMENT_CLOSURE = 71,
    COMP_CFSCRIT_STAGO = 72,
    COMP_CFSCRIPT = 73,
    COMP_CFSCRIPT_ETAGO = 74,
    COMP_CFSCRIT_END_CFSCRIPT = 75,
    OPEN_BRACE = 76,
    CLOSE_BRACE = 77,
    LT_OP = 78,
    LTE_OP = 79,
    GT_OP = 80,
    GTE_OP = 81,
    ELSE_STMT = 82,
    IF_STMT = 83,
    FOR_STMT = 84,
    WHILE_STMT = 85,
    DO_STMT = 86,
    SWITCH_STMT = 87,
    BREAK_STMT = 88,
    CONTINUE_STMT = 89,
    IMPORT_STMT = 90,
    CFIMPORT_STMT = 91,
    CASE_SUBSTMT = 92,
    DEFAULT_SUBSTMT = 93,
    PROCESSINGDIRECTIVE_STMT = 94,
    IN = 95,
    FINALLY = 96,
    RETURN = 97,
    TRY_STMT = 98,
    CATCH_STMT = 99,
    _NULL = 100,
    IMP = 101,
    EQV = 102,
    XOR = 103,
    OR = 104,
    AND = 105,
    NOT = 106,
    EQ = 107,
    NEQ = 108,
    EQ_STRICT_OP = 109,
    NEQ_STRICT_OP = 110,
    EQ_OP = 111,
    NEQ_OP = 112,
    LT = 113,
    LTE = 114,
    GT = 115,
    GTE = 116,
    CONTAINS = 117,
    DOES_NOT_CONTAIN = 118,
    CONCAT = 119,
    ADD = 120,
    SUBTRACT = 121,
    CARET = 122,
    MOD = 123,
    INT_DIV = 124,
    MULTIPLY = 125,
    DIVIDE = 126,
    TRUE = 127,
    FALSE = 128,
    DOT = 129,
    LEFT_BRACKET = 130,
    RIGHT_BRACKET = 131,
    COMMA = 132,
    SEMICOLON = 133,
    HOOK = 134,
    COLON = 135,
    ASSIGN = 136,
    MULT_ASSIGN = 137,
    DIV_ASSIGN = 138,
    PLUS_ASSIGN = 139,
    MINUS_ASSIGN = 140,
    CONCAT_ASSIGN = 141,
    MOD_ASSIGN = 142,
    CARET_ASSIGN = 143,
    INCREMENT = 144,
    DECREMENT = 145,
    ARROW = 146,
    FINAL = 147,
    ABSTRACT = 148,
    PARAMETEREXISTS = 149,
    PRESERVESINGLEQUOTES = 150,
    QUOTEDVALUELIST = 151,
    VALUELIST = 152,
    FUNCTION = 153,
    CLIENTSETTINGS = 154,
    INTEGER = 155,
    DOUBLE = 156,
    Variable = 157,
    PIPE = 158,
    TAG_ATTRIBUTE_PIPE = 159,
    Whitespace = 160,
    AnyWS = 161,
    SomeWS = 162,
    EscapedHashMark = 163,
    LiveHashMark = 164,
    HASH = 165,
    CRLF = 166,
    CR = 167,
    LF = 168,
    OPEN_PARENTHESIS = 169,
    CLOSE_PARENTHESIS = 170,
    EndDoubleQuoteString = 171,
    DoubleQuoteString = 172,
    DoubleQuoteEscape = 173,
    EndSingleQuoteString = 174,
    SingleQuoteString = 175,
    SingleQuoteEscape = 176,
};

static const quint8 breakpoint_png[] =
{
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
    ,0x52,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x08,0x06,0x00,0x00,0x00,0x1F
    ,0xF3,0xFF,0x61,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
    ,0xE9,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0xFF,0x00,0xFF,0x00,0xFF
    ,0xA0,0xBD,0xA7,0x93,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0D
    ,0xD7,0x00,0x00,0x0D,0xD7,0x01,0x42,0x28,0x9B,0x78,0x00,0x00,0x00,0x07,0x74
    ,0x49,0x4D,0x45,0x07,0xDA,0x03,0x05,0x0C,0x0A,0x3A,0x3A,0xF6,0xD7,0x57,0x00
    ,0x00,0x02,0x7D,0x49,0x44,0x41,0x54,0x38,0xCB,0xAD,0x93,0x4B,0x6B,0x14,0x51
    ,0x10,0x85,0xBF,0x7B,0xBB,0x27,0xF3,0x1E,0xC9,0x28,0x26,0xC1,0x5D,0x10,0x23
    ,0x88,0x42,0x20,0x18,0x89,0x20,0xE6,0x17,0x88,0x3B,0x83,0x3B,0x37,0x8A,0xAE
    ,0xFC,0x27,0x82,0x28,0x82,0x20,0xB8,0x12,0xC1,0xAD,0x0B,0x71,0x11,0x54,0x8C
    ,0x11,0xC1,0x07,0x8A,0x31,0x24,0x18,0x92,0x18,0x86,0x19,0x0D,0xB1,0xA7,0x9F
    ,0xD3,0xDD,0xF7,0x96,0x8B,0xD6,0x10,0xC1,0xA5,0x07,0x0E,0x05,0x17,0xEE,0xA1
    ,0xEA,0x9C,0x2A,0xC5,0x1E,0xCC,0x83,0x5B,0x83,0x8B,0xD5,0xC9,0xC9,0x2B,0xE5
    ,0xF1,0xF1,0xE3,0xA5,0x76,0xBB,0x9A,0x77,0xBB,0xC1,0x60,0x65,0x65,0x31,0x5A
    ,0x5A,0xBA,0x79,0x12,0x1E,0x2B,0x10,0xFE,0x85,0x37,0x30,0xF1,0x69,0x7A,0x7A
    ,0x73,0xFB,0xE1,0xC3,0x3C,0x5D,0x5F,0x97,0x6C,0x73,0xB3,0xE0,0xC6,0x86,0x0C
    ,0xD6,0xD6,0xA4,0x7B,0xE7,0x8E,0xF9,0x70,0xF4,0xE8,0xDB,0x97,0x70,0x70,0xEF
    ,0x3F,0x05,0xB0,0x00,0x87,0xEB,0x47,0x8E,0xBC,0x1B,0xBF,0x75,0xAB,0x3E,0xD4
    ,0x6C,0x2A,0xB4,0x06,0xA5,0x0A,0x8A,0x80,0x08,0x62,0x2D,0x49,0xA7,0x63,0xBF
    ,0x5E,0xBE,0xFC,0xCD,0xF6,0x7A,0x27,0xA6,0xC0,0x03,0x70,0x04,0x54,0x47,0xEB
    ,0xF9,0xB1,0xB9,0xB9,0x43,0xD5,0xB1,0x31,0x2D,0x71,0xCC,0x2E,0xA3,0x08,0x09
    ,0x43,0x24,0x08,0x90,0x7E,0x1F,0x6D,0x8C,0x32,0xBE,0x5F,0x8B,0x96,0x97,0x0F
    ,0xDF,0x15,0x79,0x04,0xE0,0x2E,0xC2,0xF9,0x21,0xD7,0x1D,0xAF,0x38,0x8E,0x36
    ,0xDD,0x2E,0xBA,0x5C,0x86,0x52,0x09,0x5C,0xB7,0xE8,0xC0,0x5A,0xC8,0x73,0x24
    ,0xCB,0x90,0xC1,0x80,0x66,0xA3,0x51,0xFA,0x2E,0x72,0x6E,0x01,0x8E,0xCF,0xC0
    ,0x47,0xD7,0x71,0x9C,0xAB,0x8D,0x91,0x91,0x46,0xBE,0xBA,0x8A,0xDE,0xB7,0x0F
    ,0x69,0x34,0x50,0x95,0x0A,0x38,0x0E,0x4A,0x6B,0xE4,0x8F,0x40,0x1C,0x63,0x7D
    ,0x1F,0xD9,0xDA,0xA2,0x3E,0x3C,0xAC,0xA3,0xED,0xED,0x4B,0xC0,0x75,0x17,0x6B
    ,0xA7,0xCA,0xAD,0x16,0x92,0xA6,0xE4,0x2B,0x2B,0xE8,0xD1,0x51,0x54,0xAB,0x85
    ,0xAE,0x54,0x10,0xC7,0x81,0x3C,0xC7,0x0E,0x06,0xC8,0xCE,0x0E,0x79,0xA7,0x83
    ,0x88,0x50,0x6E,0xB5,0x86,0xA2,0x9D,0x9D,0x33,0x58,0x8B,0x6B,0x45,0x1A,0x8E
    ,0xEB,0x82,0xD6,0x48,0x9E,0x63,0x7D,0x1F,0x6D,0x0C,0xB6,0x56,0x43,0x55,0xAB
    ,0x85,0x0F,0x51,0x84,0x0D,0x43,0x94,0xB5,0x88,0xD6,0x38,0xA5,0x12,0x02,0xC3
    ,0x00,0x5A,0x29,0xE5,0x99,0x34,0x2D,0x66,0xFD,0xCD,0xDD,0xB6,0xA3,0x08,0xC9
    ,0x73,0xC4,0x18,0x30,0xA6,0x78,0xB7,0x16,0x93,0xA6,0xA0,0x54,0x0F,0x40,0x03
    ,0xCF,0x13,0xCF,0x93,0x5D,0xA3,0x92,0x04,0xD2,0x14,0xFB,0x27,0x85,0x38,0x46
    ,0xD2,0x14,0x49,0x53,0xC8,0x32,0x24,0xCB,0x88,0x3D,0x2F,0x11,0x63,0x9E,0x00
    ,0x68,0x23,0x72,0xC3,0xEF,0xF5,0x62,0x1B,0xC7,0x48,0x92,0x14,0x66,0x05,0x01
    ,0x12,0x86,0xD8,0x30,0x2C,0x62,0x0C,0xC3,0x42,0x2C,0x49,0xC8,0x83,0x80,0xD8
    ,0xF3,0xC4,0xC0,0x3D,0x00,0x3D,0x03,0xCF,0x44,0xE4,0xC5,0x8F,0xE5,0xE5,0x44
    ,0xA2,0xA8,0xC8,0xDC,0xF7,0xB1,0xFD,0x3E,0xE2,0x79,0x45,0xF5,0xFD,0x42,0x30
    ,0x08,0xE8,0x2D,0x2D,0xC5,0x0A,0x6E,0x9F,0x86,0xF5,0xBD,0x9B,0xD8,0x76,0x94
    ,0x7A,0x5F,0x69,0x36,0x0F,0x1E,0x98,0x98,0x28,0xBB,0xF5,0x7A,0xB1,0x07,0x5A
    ,0x17,0x9E,0x64,0x19,0x59,0xBF,0x4F,0xEF,0xCB,0x97,0x24,0x4F,0x92,0xB7,0x0D
    ,0x91,0xD9,0x63,0x90,0xEE,0x0A,0x00,0xBC,0x86,0xFD,0x4A,0xA9,0x07,0xC0,0xD9
    ,0x5A,0xBB,0x2D,0xE5,0x56,0x6B,0xC8,0x29,0x95,0x30,0x69,0x4A,0xFC,0xF3,0xE7
    ,0x20,0xF2,0x3C,0x17,0xA5,0xEE,0xC7,0xD6,0x5E,0x9B,0x85,0xE4,0xAF,0x5B,0xD8
    ,0x8B,0x57,0x30,0xAD,0xE0,0x82,0xD6,0xFA,0x8C,0x85,0xB6,0x52,0xAA,0x2B,0xC6
    ,0x3C,0x05,0x1E,0x9C,0x82,0xCF,0xFC,0x6F,0xFC,0x02,0x62,0x4A,0x95,0x5A,0x65
    ,0x04,0x91,0x38,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
};

static const quint8 breakpoint_pending_png[] =
{
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
    ,0x52,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x08,0x06,0x00,0x00,0x00,0x1F
    ,0xF3,0xFF,0x61,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
    ,0xE9,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0xFF,0x00,0xFF,0x00,0xFF
    ,0xA0,0xBD,0xA7,0x93,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0D
    ,0xD7,0x00,0x00,0x0D,0xD7,0x01,0x42,0x28,0x9B,0x78,0x00,0x00,0x00,0x07,0x74
    ,0x49,0x4D,0x45,0x07,0xDA,0x03,0x05,0x0C,0x0A,0x0F,0x6C,0x45,0x13,0x74,0x00
    ,0x00,0x02,0xE8,0x49,0x44,0x41,0x54,0x38,0xCB,0x8D,0x93,0xDF,0x6B,0x1C,0x75
    ,0x14,0xC5,0x3F,0xDF,0xEF,0xCC,0xEC,0xAF,0x6E,0xB7,0xB8,0x34,0x69,0x36,0x29
    ,0x82,0x6B,0xDA,0x46,0xC4,0xB4,0x4B,0x53,0x53,0x22,0x48,0xAA,0xD2,0x67,0x8B
    ,0x20,0x8D,0xFA,0x58,0x88,0xF8,0xE4,0x3F,0x20,0xF8,0x22,0x88,0x2F,0x0A,0x1A
    ,0x08,0xF8,0x20,0x42,0x31,0x54,0x8A,0x2F,0x52,0x0A,0x5A,0x28,0x2A,0x6A,0x6D
    ,0x8B,0x92,0xA0,0x29,0xEB,0xDA,0x9F,0x69,0x88,0x93,0x36,0xDD,0xEC,0xCC,0xCE
    ,0xCE,0xEC,0xEC,0xCC,0xF7,0xFA,0xB0,0xB4,0x09,0xE2,0x83,0x07,0x0E,0x17,0x2E
    ,0xF7,0xC0,0x3D,0xF7,0x70,0x15,0xDB,0x70,0x11,0xEC,0x02,0xBC,0x9E,0xAF,0xD5
    ,0xDE,0xCC,0x56,0xAB,0xCF,0x38,0xE5,0x72,0x3E,0x71,0xDD,0x76,0xB7,0xD1,0xB8
    ,0xD4,0xB9,0x76,0xED,0xE3,0x67,0xE1,0x9C,0x02,0xE1,0xBF,0x70,0x05,0x0E,0xFC
    ,0x3E,0x39,0xB9,0xB2,0x71,0xE6,0x4C,0x12,0xDF,0xBE,0x2D,0xBD,0x95,0x95,0x3E
    ,0xEF,0xDC,0x91,0xEE,0xCD,0x9B,0xE2,0xCE,0xCF,0xA7,0x8B,0x63,0x63,0xBF,0xFE
    ,0x08,0x83,0xDB,0x75,0x0A,0xE0,0x27,0x18,0xDD,0xB1,0x7F,0xFF,0x6F,0xD5,0xB9
    ,0xB9,0x1D,0x99,0x9D,0x3B,0x15,0x5A,0x83,0x52,0x7D,0x8A,0x80,0x08,0x62,0x0C
    ,0xD1,0xDA,0x9A,0xB9,0x31,0x3B,0x7B,0xD7,0xAC,0xAF,0x8F,0x4F,0x40,0x0B,0xC0
    ,0x12,0x50,0x6B,0x5A,0x5F,0xAC,0xCC,0xCC,0x8C,0xE4,0x2B,0x15,0x2D,0x61,0xC8
    ,0x23,0x76,0x3A,0x48,0x10,0x20,0xED,0x36,0xE2,0x79,0xE8,0x34,0x55,0xA9,0xEF
    ,0x17,0x3A,0xF5,0xFA,0xE8,0xA7,0x22,0x67,0x01,0xEC,0x4B,0x70,0x22,0x63,0xDB
    ,0xD5,0x9C,0x65,0xE9,0xD4,0x75,0xD1,0xD9,0x2C,0x38,0x0E,0xD8,0x76,0x7F,0x03
    ,0x63,0xD8,0xBC,0x97,0x67,0xD5,0x1D,0x46,0x29,0x43,0x5A,0x78,0xD5,0xD9,0x78
    ,0x6A,0xDF,0x2B,0x9F,0x0D,0x9C,0xFC,0xE0,0x86,0x3B,0xBE,0xA0,0x2E,0x5B,0xD6
    ,0x85,0x5D,0xC3,0xC3,0x2F,0x3E,0x76,0xF0,0x20,0x99,0xC3,0x87,0x51,0xC5,0x22
    ,0x2A,0x97,0x03,0xCB,0x42,0x69,0x8D,0x18,0xC3,0xC2,0xD9,0x13,0x3C,0x7E,0x7C
    ,0x08,0x27,0xBB,0xE5,0x3D,0x89,0xE1,0xF2,0x69,0x36,0x6C,0x8C,0x99,0xC8,0x96
    ,0x4A,0x48,0x1C,0x93,0x34,0x1A,0xE8,0xA1,0x21,0x54,0xA9,0x84,0xCE,0xE5,0x10
    ,0xCB,0x82,0x24,0x61,0x74,0x68,0x89,0x6F,0x3E,0xEF,0x62,0xA1,0xC9,0x5B,0x16
    ,0xDD,0x5E,0x8F,0x20,0x35,0xB2,0x2B,0xFB,0xC4,0x69,0xDB,0x88,0x14,0x2D,0xDB
    ,0x06,0xAD,0x91,0x24,0xC1,0xF8,0x3E,0x3A,0x4D,0x31,0x85,0x02,0x2A,0x9F,0x47
    ,0x3A,0x1D,0x6A,0xBB,0xCF,0xF1,0x7E,0xFB,0x14,0x47,0x2B,0x15,0x5E,0x1E,0x19
    ,0xE1,0xC2,0xF2,0x32,0xF3,0xD7,0xFF,0x4C,0xEF,0x8B,0xBC,0xAD,0x95,0x52,0xAD
    ,0x34,0x8E,0xC1,0x98,0x47,0x14,0x63,0x20,0x49,0xFA,0x47,0x4C,0x12,0x24,0x4D
    ,0x99,0x1C,0xDC,0xC3,0x52,0x73,0x13,0x8C,0xA1,0x1E,0xF7,0x18,0xD7,0x7A,0x13
    ,0x40,0x03,0xDF,0x47,0xAD,0x96,0x90,0x24,0x48,0xAF,0x87,0x44,0x11,0xC4,0x31
    ,0xE6,0x61,0x0A,0x61,0x88,0xC4,0x31,0xA7,0x9E,0xAC,0x72,0xC5,0x75,0xF9,0xF2
    ,0xE9,0x09,0xBE,0x6D,0x79,0x32,0x93,0xA6,0x5F,0x00,0xE8,0x54,0xE4,0x23,0x7F
    ,0x7D,0x3D,0x34,0x61,0x88,0x44,0x11,0x12,0x86,0x98,0x76,0x1B,0x09,0x02,0x4C
    ,0x10,0xF4,0x63,0x0C,0x02,0x0A,0x71,0xCC,0x58,0xC6,0xC1,0xAC,0xDC,0x42,0x6D
    ,0x3E,0x90,0x51,0xF8,0x10,0x40,0x4F,0xC1,0x77,0x22,0xF2,0xC3,0xFD,0x7A,0x3D
    ,0x92,0x4E,0xA7,0x9F,0xB9,0xEF,0x63,0x3C,0x0F,0x69,0xB5,0xFA,0xD5,0xF7,0x59
    ,0x76,0x5D,0x9C,0xBD,0x7B,0x99,0x7E,0xE9,0x05,0x39,0x32,0x35,0xB5,0x76,0x0C
    ,0x0E,0x3D,0xB4,0x40,0x92,0xA6,0xAF,0x75,0x9A,0xCD,0x7B,0xEE,0xE2,0x62,0x37
    ,0x69,0x36,0x91,0xED,0x62,0xCF,0xC3,0x78,0x1E,0x9F,0xFC,0x75,0x9D,0x43,0x47
    ,0x8E,0xCA,0xDF,0xDA,0x51,0xFB,0x8E,0x1D,0xCF,0x14,0x8B,0xC5,0x77,0x01,0x6C
    ,0x80,0x29,0x78,0xF0,0x8B,0x48,0xAD,0xEB,0xFB,0x0B,0xAB,0x57,0xAF,0x4E,0x17
    ,0xCA,0x65,0xC9,0x96,0x4A,0x19,0xCB,0x71,0x48,0xE3,0x18,0xAF,0x69,0xBA,0xCD
    ,0xA0,0x96,0x69,0x2C,0xED,0x51,0x2A,0x32,0xAC,0xAE,0xE6,0x07,0x76,0x87,0xD3
    ,0xCE,0x1B,0x7C,0x3D,0xA8,0xFE,0xFD,0x54,0x3F,0xC3,0xA4,0x82,0x93,0x5A,0xEB
    ,0xE7,0x0D,0x94,0x95,0x52,0xEE,0x57,0xF2,0xC7,0xC0,0x73,0xEF,0x1C,0xA8,0xDA
    ,0x99,0xAD,0xB9,0x38,0x84,0xF3,0xEF,0x71,0x4B,0xF1,0x3F,0x30,0x8B,0xCC,0xD9
    ,0x79,0xDE,0xD2,0xD6,0x56,0xCF,0x24,0xD0,0x8B,0x38,0xFF,0x0F,0x1C,0xAE,0x98
    ,0x9F,0xE4,0x9C,0x3F,0x9D,0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42
    ,0x60,0x82
};

static const quint8 breakpoint_disabled_png[] =
{
    0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44
    ,0x52,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x08,0x06,0x00,0x00,0x00,0x1F
    ,0xF3,0xFF,0x61,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xAE,0xCE,0x1C
    ,0xE9,0x00,0x00,0x00,0x06,0x62,0x4B,0x47,0x44,0x00,0xFF,0x00,0xFF,0x00,0xFF
    ,0xA0,0xBD,0xA7,0x93,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0D
    ,0xD7,0x00,0x00,0x0D,0xD7,0x01,0x42,0x28,0x9B,0x78,0x00,0x00,0x00,0x07,0x74
    ,0x49,0x4D,0x45,0x07,0xDA,0x03,0x05,0x0C,0x02,0x27,0x91,0x29,0x31,0x86,0x00
    ,0x00,0x02,0x65,0x49,0x44,0x41,0x54,0x38,0xCB,0x6D,0x93,0x3F,0x48,0x5B,0x51
    ,0x14,0xC6,0x7F,0xEF,0x4F,0x34,0x7F,0x1C,0x54,0x88,0xD8,0xD8,0x4A,0x30,0x25
    ,0xA4,0x53,0x45,0x02,0xDA,0x4E,0x3A,0x75,0x72,0x11,0xA9,0x85,0x82,0x73,0x07
    ,0xA1,0x9B,0x50,0xBA,0x6B,0xC5,0xA9,0x4B,0x33,0x3B,0x74,0x10,0xC4,0x45,0x28
    ,0x74,0xEA,0xA0,0x8D,0x0D,0x68,0xDB,0x2C,0xA6,0x0D,0x01,0x6D,0x9A,0x44,0x4C
    ,0xAA,0xC6,0x68,0x92,0x97,0xE4,0xBD,0x77,0x3B,0xE4,0xDA,0x3E,0xDB,0x5C,0xF8
    ,0xB8,0xDC,0x73,0xEF,0x77,0xCE,0xF9,0xCE,0x3D,0x47,0xA1,0xC3,0xDA,0x03,0x57
    ,0x13,0x1E,0x6A,0x10,0x04,0xB0,0xE0,0xA8,0x01,0x1F,0xA7,0xC0,0xFC,0xF7,0xAD
    ,0xE2,0x3C,0x7C,0x80,0x1E,0xAF,0xAE,0xBF,0xE8,0x9B,0x9D,0x7D,0xDE,0x3F,0x33
    ,0xA3,0x79,0xEF,0xDD,0xB3,0x00,0x6A,0xA9,0x94,0x76,0xB6,0xB9,0x69,0x9D,0x6F
    ,0x6C,0xBC,0x76,0x9B,0xE6,0xD2,0x7D,0xA8,0xFE,0x17,0x35,0x0E,0x43,0xC9,0x48
    ,0xE4,0xDB,0xE5,0xEE,0x6E,0xC5,0x6E,0x34,0xCA,0x42,0x88,0x53,0x27,0xEC,0x46
    ,0xA3,0x5C,0x89,0xC7,0x2B,0xC9,0x70,0xF8,0x20,0x0E,0x43,0x37,0xC8,0xEF,0xA0
    ,0xFB,0x4B,0x28,0x94,0xAC,0xA5,0x52,0x25,0x21,0xC4,0xA1,0x10,0x22,0x2B,0x84
    ,0xC8,0x09,0x21,0xF2,0x12,0x39,0x69,0x3B,0xAC,0xA5,0x52,0xA5,0xAF,0xA1,0xD0
    ,0xE7,0x3D,0x70,0x01,0xA8,0x00,0xFD,0xAA,0xBA,0x30,0xBC,0xB2,0x32,0xE0,0x89
    ,0x44,0x0A,0x80,0x01,0x34,0x69,0xEB,0xB5,0x25,0x4C,0x69,0x33,0x3C,0x91,0x48
    ,0xE1,0xF6,0xF2,0xF2,0x2D,0x4B,0x55,0x17,0x00,0x94,0x3D,0x70,0xB9,0xC7,0xC7
    ,0x4F,0xC2,0x5B,0x5B,0x87,0x2E,0xBF,0xDF,0x04,0x3C,0x80,0x1B,0xE8,0x02,0x74
    ,0x99,0xE4,0x1F,0x07,0x40,0xDD,0x2C,0x95,0xF4,0xEF,0xD3,0xD3,0xC1,0x83,0x44
    ,0x62,0x40,0xB5,0x60,0xC2,0x3B,0x36,0xA6,0xB9,0xFC,0xFE,0x13,0xA0,0xC1,0xDF
    ,0x4A,0xEB,0x40,0xB7,0x84,0xD3,0x51,0x43,0xF7,0xFB,0x4F,0xBC,0xA3,0xA3,0xAE
    ,0x3B,0x30,0xA1,0x0B,0xB8,0xDB,0x35,0x38,0x58,0xA6,0x5D,0xD9,0x6B,0x92,0x0D
    ,0x68,0x32,0x1B,0x1C,0x72,0x5A,0x32,0x8B,0x6A,0x57,0x20,0x50,0x56,0x20,0xAC
    ,0x03,0xD8,0x86,0x61,0x03,0x15,0x87,0x03,0x8F,0x24,0x89,0x0E,0x12,0xAA,0x40
    ,0x45,0xB4,0x39,0xE8,0x0A,0x64,0x8C,0x74,0xBA,0x0F,0xDB,0x2E,0xA3,0xAA,0x9A
    ,0x8C,0xAC,0xCB,0x02,0x5F,0xCB,0xA9,0x02,0x97,0xC0,0x05,0x70,0x81,0x6D,0x97
    ,0xEB,0xE9,0x74,0xAF,0x80,0xB4,0x5A,0x83,0x5D,0x23,0x93,0xB1,0x2B,0x3B,0x3B
    ,0xDD,0x40,0x09,0xF8,0x25,0xF7,0x22,0x70,0x2C,0x51,0x74,0xDE,0x55,0xB6,0xB7
    ,0xDD,0x46,0x3A,0x6D,0xFD,0x84,0x4F,0xEA,0x14,0x98,0xAD,0x5C,0xEE,0x55,0x31
    ,0x16,0x9B,0xC4,0x34,0x8F,0x81,0x02,0x90,0x03,0xB2,0xC0,0x0F,0x89,0xAC,0xB4
    ,0x15,0x30,0xCD,0xE3,0x62,0x2C,0x36,0x69,0xE4,0xF3,0x4B,0x8F,0xC1,0x52,0xAE
    ,0x7B,0x5F,0x0B,0x06,0x13,0x3D,0xD1,0xE8,0xF9,0xC8,0xDA,0xDA,0xBA,0xEA,0xF5
    ,0xF6,0x00,0x3E,0xF9,0x95,0x48,0xFD,0x55,0xBB,0x56,0xBB,0xCA,0xCC,0xCF,0x3F
    ,0xA9,0xED,0xEF,0xF7,0x5A,0x47,0x47,0xE3,0x51,0x68,0x29,0xCE,0x56,0x76,0x07
    ,0x02,0xEF,0x5D,0x7E,0xBF,0xD2,0x37,0x37,0xF7,0x76,0x68,0x71,0x31,0x83,0xA6
    ,0xB5,0xBF,0xAF,0xD9,0xB4,0xF2,0xAB,0xAB,0xA1,0xB3,0xF5,0xF5,0xA7,0xE6,0xE9
    ,0xA9,0x65,0x17,0x0A,0x8F,0xA2,0x6D,0x69,0x37,0x87,0x29,0x09,0xBE,0x96,0xCF
    ,0xF7,0x12,0x5D,0x7F,0x86,0xA6,0x59,0xAA,0xC7,0x93,0x05,0xB0,0xEB,0xF5,0x61
    ,0x2C,0x4B,0x13,0x96,0xF5,0x46,0xB9,0xBA,0x5A,0x8A,0x42,0xAD,0xE3,0x34,0x3A
    ,0xA6,0x52,0xF7,0xC2,0x03,0x60,0x84,0x76,0x03,0x64,0x0C,0x48,0x74,0x1A,0xE7
    ,0xDF,0xC8,0x8D,0x30,0x4C,0x9E,0x93,0x90,0x83,0x00,0x00,0x00,0x00,0x49,0x45
    ,0x4E,0x44,0xAE,0x42,0x60,0x82
};

QCodeEditWidget::QCodeEditWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
    , m_LineNumbersNormal(QPen(QColor(172, 168, 153)))
    , m_LineNumbersCurrent(QPen(QColor(128, 128, 128)))
    , m_LineModifiedAndNotSavedBackground(QColor(128, 0, 0))
    , m_LineModifiedAndSavedBackground(QColor(0, 128, 0))
    , m_CurrentLineBackground(QColor(224, 233, 247))
#ifdef Q_OS_WIN
    , m_TextFont(QFont("Courier", m_FontSize, QFont::Normal))
    , m_TextFontBold(QFont("Courier", m_FontSize, QFont::Bold))
#elif defined Q_OS_LINUX
    , m_TextFont(QFont("Droid Sans Mono", m_FontSize, QFont::Normal))
    , m_TextFontBold(QFont("Droid Sans Mono", m_FontSize, QFont::Bold))
#elif defined Q_OS_MACOS
    , m_TextFont(QFont("Monaco", m_FontSize, QFont::Normal))
    , m_TextFontBold(QFont("Monaco", m_FontSize, QFont::Bold))
#else
#error Unsupported OS.
#endif
{
    QFontMetrics l_fm = QFontMetrics(m_TextFont);
    setAutoFillBackground(false);

    m_TextFont.setStyleHint(QFont::Courier, QFont::PreferAntialias);

    m_FontHeight = l_fm.height();
    m_LineHeight = m_FontHeight + 1;

    m_LineYOffset = m_LineHeight - l_fm.descent() - 1;

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setText("");

    m_BreakPointPixmap = QPixmap::fromImage(QImage::fromData(breakpoint_png, sizeof(breakpoint_png)));
    m_BreakPointPixmapPending = QPixmap::fromImage(QImage::fromData(breakpoint_pending_png, sizeof(breakpoint_pending_png)));
    m_BreakPointPixmapDisabled = QPixmap::fromImage(QImage::fromData(breakpoint_disabled_png, sizeof(breakpoint_disabled_png)));

    setFocusPolicy(Qt::WheelFocus);

    m_CursorTimerID = startTimer(500);
}

QCodeEditWidget::~QCodeEditWidget()
{
    killTimer(m_CursorTimerID);
}

void QCodeEditWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() ==  m_CursorTimerID)
    {
        m_currentlyBlinkCursorShowen ^= 1;

        //viewport()->update(0, 0, ceil(m_CursorHeight * 0.05) + 1, m_CursorHeight + 1);
        viewport()->update(); // TODO: optimize this call, by updating the affected region only!!
    }
}

void QCodeEditWidget::keyPressEvent(QKeyEvent *event)
{
    bool l_TextUpdated = false;
    auto modifiers = event->modifiers();

    if (modifiers & Qt::ControlModifier)
    {
        return;
    }

    if ((modifiers == Qt::ControlModifier)&&(event->text() == "0"))
    {
        m_FontSize = 10;

        m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
        m_TextFontBold = QFont("Source Code Pro", m_FontSize, QFont::Bold, false);
        viewport()->update();

        return;
    }

    switch(event->key())
    {
    case Qt::Key_Home:
        if ((modifiers == Qt::NoModifier)||(modifiers == Qt::ShiftModifier))
        {
            m_CarretPosition.m_Column = 1;
        }
        else if (modifiers == Qt::ControlModifier)
        {
            m_CarretPosition.m_Column = 1;
            m_CarretPosition.m_Row = 1;
        }
        break;

    case Qt::Key_End:
        if ((modifiers == Qt::NoModifier)||(modifiers == Qt::ShiftModifier))
        {
            m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
        }
        else if (modifiers == Qt::ControlModifier)
        {
            m_CarretPosition.m_Row = m_Lines.count();
            m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
        }
        break;

    case Qt::Key_Up:
        if (m_CarretPosition.m_Row <= 1)
        {
            return;
        }

        m_CarretPosition.m_Row--;
        break;

    case Qt::Key_Down:
        if (m_CarretPosition.m_Row < m_Lines.count())
        {
            m_CarretPosition.m_Row++;
        }
        break;

    case Qt::Key_Left:
        if (m_CarretPosition.m_Column <= 1)
        {
            if (m_CarretPosition.m_Row > 1)
            {
                m_CarretPosition.m_Row--;
                m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
            }
            else
            {
                return;
            }
        }
        else
        {
            auto line_size = m_Lines.at(m_CarretPosition.m_Row - 1).text.length();
            if (line_size < m_CarretPosition.m_Column)
                m_CarretPosition.m_Column = std::max(line_size, static_cast<qsizetype>(1));
            else
                m_CarretPosition.m_Column--;
        }
        break;

    case Qt::Key_Right:
        if (m_CarretPosition.m_Column <= m_Lines.at(m_CarretPosition.m_Row - 1).text.length())
        {
            m_CarretPosition.m_Column++;
        }
        else
        {
            if (m_CarretPosition.m_Row < m_Lines.count())
            {
                m_CarretPosition.m_Row++;
                m_CarretPosition.m_Column = 1;
            }
        }
        break;

    case Qt::Key_Return:
        if (modifiers == 0)
        {
            auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
            QString l_NewLineText = line.text.right(line.text.length() - m_CarretPosition.m_Column + 1);
            line.text.remove(m_CarretPosition.m_Column - 1, line.text.length() - m_CarretPosition.m_Column + 1);
            line.lineStatus = QLineStatusTypeLineModified;

            QCodeEditWidgetLine line2;
            line2.text = l_NewLineText;
            line2.type = line.type;
            line2.lineStatus = QLineStatusTypeLineModified;
            line2.breakpointType = QBreakpointTypeNoBreakpoint;

            line.type = QTextParserLine::QTextParserLineTypeLFEndLine;

            m_Lines.insert(m_CarretPosition.m_Row - 1, line);
            m_Lines.insert(m_CarretPosition.m_Row, line2);

            m_CarretPosition.m_Row++;
            m_CarretPosition.m_Column = 1;

            l_TextUpdated = true;
        }
        break;

    case Qt::Key_Backspace:
        if (modifiers == 0)
        {
            if ((m_CarretPosition.m_Row == 1)&&(m_CarretPosition.m_Column == 1))
            {
                break;
            }

            if (m_CarretPosition == m_SelectionPosition)
            {
                if (m_CarretPosition.m_Column > 1)
                {
                    auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
                    auto lineString = line.text;
                    lineString = lineString.left(m_CarretPosition.m_Column - 2) + lineString.right(line.text.length() - m_CarretPosition.m_Column + 1);
                    line.text = lineString;
                    m_Lines.insert(m_CarretPosition.m_Row - 1, line);

                    m_CarretPosition.m_Column--;
                    m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
                }
                else
                {
                    auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
                    auto line2 = m_Lines.takeAt(m_CarretPosition.m_Row - 2);

                    int l_oldPos = line2.text.length();

                    line2.text.append(line.text);

                    m_Lines.insert(m_CarretPosition.m_Row - 2, line2);

                    m_CarretPosition.m_Row--;
                    m_SelectionPosition.m_Row = m_CarretPosition.m_Row;
                    m_CarretPosition.m_Column = l_oldPos + 1;
                    m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
                }

                l_TextUpdated = true;
            }
            else
            {
                deleteSelectedBlock();
            }
        }
        break;

    case Qt::Key_Delete:
        {
            if (m_CarretPosition == m_SelectionPosition)
            {
                auto line = m_Lines.at(m_CarretPosition.m_Row - 1);

                if (m_CarretPosition.m_Column - 1 == line.text.length())
                {
                    if (m_Lines.length() > m_CarretPosition.m_Row)
                    {
                        auto line2 = m_Lines.takeAt(m_CarretPosition.m_Row);

                        line.text.append(line2.text);
                    }
                }
                else
                {
                    line.text.remove(m_CarretPosition.m_Column - 1, 1);
                }
                m_Lines.replace(m_CarretPosition.m_Row - 1, line);

                l_TextUpdated = true;
            }
            else
            {
                deleteSelectedBlock();
            }
        }
        break;

    case Qt::Key_Insert:
    case Qt::Key_C:
        {
            if (modifiers == Qt::ControlModifier)
            {
                QString selectedText = "";

                QClipboard *clipboard = QApplication::clipboard();

                clipboard->setText(selectedText);
            }
        }
        break;
    }

    auto l_EventText = event->text();

    if ((!l_EventText.isEmpty())&&((modifiers & (Qt::ControlModifier | Qt::AltModifier)) == 0))
    {
        deleteSelectedBlock();

        for(const auto &ch: std::as_const(l_EventText))
        {
            if (ch.isPrint())
            {
                auto line = m_Lines.at(m_CarretPosition.m_Row - 1);
                line.text.insert(m_CarretPosition.m_Column - 1, l_EventText);
                line.lineStatus = QLineStatusTypeLineModified;

                m_Lines.replace(m_CarretPosition.m_Row - 1, line);
                m_CarretPosition.m_Column += l_EventText.size();
                m_SelectionPosition.m_Column = m_CarretPosition.m_Column;

                l_TextUpdated = true;
            }
        }
    }

    if ((modifiers & Qt::ShiftModifier) == 0)
    {
        m_SelectionPosition.m_Row = m_CarretPosition.m_Row;
        m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
    }

    if (l_TextUpdated) {
        updatePanelWidth();

        emit textChanged();
        emit parserUpdate();
    } else {
        emit key_press(event);
    }

    m_currentlyBlinkCursorShowen = 1;

    killTimer(m_CursorTimerID);
    m_CursorTimerID = startTimer(500);

    ensureCaretIsVisible();

    viewport()->update(); // TODO: optimize this call, by updating the affected region only!!
}

void QCodeEditWidget::updatePanelWidth()
{
    int l_tmpWidth = 0;

    int l_tmpCount = m_Lines.count();

    while(l_tmpCount > 0)
    {
        l_tmpCount = l_tmpCount / 10;
        l_tmpWidth++;
    }

    if (l_tmpWidth < 2)
    {
        l_tmpWidth = 2;
    }

    m_LineNumbersPanelWidth = l_tmpWidth;
}

void QCodeEditWidget::ensureCaretIsVisible()
{
    if (m_CarretPosition.m_Row <= m_ScrollYLinePos)
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Row - 1);
    }
    else if (m_CarretPosition.m_Row > m_ScrollYLinePos + (viewport()->height() / m_LineHeight))
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Row - (viewport()->height() / m_LineHeight));
    }

    if (m_CarretPosition.m_Column <= m_ScrollXCharPos)
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Column - 1);
    }
    else if (m_CarretPosition.m_Column > m_ScrollXCharPos + (viewport()->width() / m_LineYOffset))
    {
        verticalScrollBar()->setValue(m_ScrollXCharPos + (viewport()->width() / m_LineYOffset));
    }

    viewport()->update();
}

void QCodeEditWidget::deleteSelectedBlock()
{
    if (m_CarretPosition.m_Row == m_SelectionPosition.m_Row)
    {
        auto row = m_CarretPosition.m_Row - 1;

        if (m_CarretPosition.m_Column > m_SelectionPosition.m_Column)
        {
            auto line = m_Lines[row].text;

            m_Lines[row].text = line.left(m_SelectionPosition.m_Column - 1) + line.right(line.length() - m_CarretPosition.m_Column + 1);

            m_CarretPosition.m_Column = m_SelectionPosition.m_Column;
        }
        else if (m_CarretPosition.m_Column < m_SelectionPosition.m_Column)
        {
            auto line = m_Lines[row].text;

            m_Lines[row].text = line.left(m_CarretPosition.m_Column - 1) + line.right(line.length() - m_SelectionPosition.m_Column + 1);
        }
    }
    else
    {
        // TODO: Implement multiline selection delete block.
    }
}

void QCodeEditWidget::scrollContentsBy(int dx, int dy)
{
    if ((dx == 0)&&(dy == 0))
    {
        return;
    }

    m_ScrollYLinePos -= dy;
    m_ScrollXCharPos -= dx;

    viewport()->update();
}

void QCodeEditWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(viewport());

    bool has_selection;
    int selection_start_row;
    int selection_end_row;

    if ((m_SelectionPosition.m_Row == m_CarretPosition.m_Row)&&(m_SelectionPosition.m_Column == m_CarretPosition.m_Column))
    {
        has_selection = false;

        selection_start_row = 0;
        selection_end_row = 0;
    }
    else
    {
        has_selection = true;

        if (m_SelectionPosition.m_Row > m_CarretPosition.m_Row)
        {
            selection_start_row = m_CarretPosition.m_Row;
            selection_end_row = m_SelectionPosition.m_Row;
        }
        else
        {
            selection_start_row = m_SelectionPosition.m_Row;
            selection_end_row = m_CarretPosition.m_Row;
        }
    }

    QFontMetrics l_fm(m_TextFont);
    int l_fontHeight = l_fm.height();
    int l_fontWidth = l_fm.horizontalAdvance(" ");
    int l_LinesToDraw = (viewport()->height() / l_fontHeight) + 1;
    int l_LineNumbersPanelWidth = 16 + (l_fm.horizontalAdvance(" ") * m_LineNumbersPanelWidth) + 4;
    int l_CharsToDraw = (((viewport()->width() - l_LineNumbersPanelWidth) / l_fontWidth) + 1);

    painter.fillRect(QRect(0, 0, l_LineNumbersPanelWidth, viewport()->height()), palette().dark());

    for(int l_line = 0; l_line < l_LinesToDraw; l_line++)
    {
    	if (m_ScrollYLinePos + l_line >= m_Lines.count())
    	{
    		break;
        }

        if (m_ScrollYLinePos + l_line == m_debuggerAtLine)
        {
            painter.fillRect(l_LineNumbersPanelWidth, l_line * l_fontHeight, viewport()->width(), l_fontHeight, QColorConstants::Red);
        }

        switch(m_Lines.at(m_ScrollYLinePos + l_line).breakpointType)
        {
        case QBreakpointTypeBreakpoint:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmap);
            break;
        case QBreakpointTypeBreakpointPending:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmapPending);
            break;
        case QBreakpointTypeDisabled:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmapDisabled);
            break;
        default:
            break;
        }

    	if ((m_ScrollYLinePos + l_line + 1) == m_CarretPosition.m_Row)
    	{
            painter.setFont(m_TextFontBold);
    		painter.setPen(m_LineNumbersCurrent);
    	}
    	else
    	{
            painter.setFont(m_TextFont);
    		painter.setPen(m_LineNumbersNormal);
    	}

        int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
        QTextOption option;
        option.setTabStopDistance(distance);
        option.setAlignment(Qt::AlignRight);
        painter.drawText(QRect(0, l_line * l_fontHeight, l_LineNumbersPanelWidth - 4, l_fontHeight), QString::number(m_ScrollYLinePos + l_line + 1), option);
        if ((m_ScrollYLinePos + l_line + 1) == m_CarretPosition.m_Row)
    	{
    		painter.setFont(m_TextFont);
        }

        painter.setPen(palette().text().color());

        int l_HorizontalValue = horizontalScrollBar()->value();

        QString l_Line = m_Lines.at(m_ScrollYLinePos + l_line).text;
        const auto &l_LineColors = m_Lines.at(m_ScrollYLinePos + l_line).colors;

        // Paint selection background
        if ((has_selection)&&(m_ScrollYLinePos + l_line >= (selection_start_row - 1))&&(m_ScrollYLinePos + l_line <= (selection_end_row - 1)))
        {
            if (selection_start_row == selection_end_row)
            {
                int from = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                int to = m_CarretPosition.m_Column - 1 - l_HorizontalValue;

                if (from < 0) from = 0;
                if (to < 0) to = 0;

                if (from != to)
                {
                    if (from > to)
                    {
                        int tmp = from;
                        from = to;
                        to = tmp;
                    }

                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else if (m_ScrollYLinePos + l_line == (selection_start_row - 1))
            {
                int from;
                int to;

                if (m_SelectionPosition.m_Row < m_CarretPosition.m_Row)
                {
                    from = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                    to = l_Line.length() - l_HorizontalValue;
                }
                else
                {
                    from = m_CarretPosition.m_Column - 1 - l_HorizontalValue;
                    to = l_Line.length() - l_HorizontalValue;
                }

                if (from < 0) from = 0;
                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else if (m_ScrollYLinePos + l_line == (selection_end_row - 1))
            {
                int from = 0;
                int to = 0;

                if (m_SelectionPosition.m_Row > m_CarretPosition.m_Row)
                {
                    to = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                }
                else
                {
                    to = m_CarretPosition.m_Column - 1 - l_HorizontalValue;
                }

                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else
            {
                int from = 0;
                int to = l_Line.length() - l_HorizontalValue;

                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
        }

        l_Line = l_Line.left(l_HorizontalValue + l_CharsToDraw);

        //l_Line = l_Line.replace("\t", QString(m_tabSize, ' ')); // TODO: Properly align tabs.

        if (l_HorizontalValue > 0)
        {
            if (l_Line.length() - l_HorizontalValue > 0)
            {
                l_Line = l_Line.right(l_Line.length() - l_HorizontalValue);
            }
            else
            {
                l_Line.clear();
            }
        }

        l_Line = l_Line.left(l_CharsToDraw);

        if (!l_Line.isEmpty())
        {
            QString text = l_Line;
            int current_pos = 0;

            if (!l_LineColors.isEmpty())
            {
                for (auto colorFormat: l_LineColors)
                {
                    if (colorFormat.index > current_pos)
                    {
                        int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(current_pos)) - (m_ScrollXCharPos * l_fontWidth);

                        painter.setPen(palette().text().color());

                        auto textChunk = text.mid(current_pos, colorFormat.index - current_pos);

                        int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                        QTextOption option;
                        option.setTabStopDistance(distance);
                        option.setAlignment(Qt::AlignLeft);
                        painter.drawText(QRect(xPosStart, l_line * l_fontHeight, viewport()->width(), l_fontHeight), textChunk, option);
                        current_pos  = colorFormat.index;
                    }

                    if (colorFormat.backgroundColor.isValid())
                    {
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(colorFormat.backgroundColor);

                        int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(colorFormat.index)) - (m_ScrollXCharPos * l_fontWidth);
                        int xPosLen = l_fm.horizontalAdvance(text.mid(colorFormat.index, colorFormat.length)) - (m_ScrollXCharPos * l_fontWidth);

                        painter.drawRect(xPosStart, l_line * l_fontHeight, xPosLen, l_fontHeight);
                    }

                    if (colorFormat.foregroundColor.isValid())
                    {
                        painter.setPen(colorFormat.foregroundColor);
                    }
                    else
                    {
                        painter.setPen(palette().text().color());
                    }

                    int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(colorFormat.index)) - (m_ScrollXCharPos * l_fontWidth);

                    auto textChunk = text.mid(colorFormat.index, colorFormat.length);
                    int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                    QTextOption option;
                    option.setTabStopDistance(distance);
                    option.setAlignment(Qt::AlignLeft);
                    painter.drawText(QRect(xPosStart, l_line * l_fontHeight, viewport()->width(), l_fontHeight), textChunk, option);
                    current_pos  = colorFormat.index + colorFormat.length;
                }
            }
            else
            {
                painter.setPen(palette().text().color());

                int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                QTextOption option;
                option.setTabStopDistance(distance);
                option.setAlignment(Qt::AlignLeft);
                painter.drawText(QRect(l_LineNumbersPanelWidth, l_line * l_fontHeight, viewport()->width(), l_fontHeight), text, option);
            }
        }
    }

    setFont(m_TextFont);

    if ((m_currentlyBlinkCursorShowen == 1)&&((int)m_CarretPosition.m_Column - (int)m_ScrollXCharPos - 1 >= 0))
    {
        int xpos = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(m_Lines.at(m_CarretPosition.m_Row - 1).text.left(m_CarretPosition.m_Column - 1)) - (m_ScrollXCharPos * l_fontWidth);
        const QBrush oldBrush = painter.brush();

        painter.setBrush(QColor(Qt::black));

        painter.drawRect(xpos, ((m_CarretPosition.m_Row - m_ScrollYLinePos - 1) * l_fontHeight), ceil(m_LineHeight * 0.05), l_fontHeight - 1);
        painter.setBrush(oldBrush);
    }

    if(m_Lines.count() - (viewport()->height() / l_fontHeight) + 1 > 1)
    {
        verticalScrollBar()->setMaximum(m_Lines.count() - (viewport()->height() / l_fontHeight));
    }
    else
    {
        verticalScrollBar()->setMaximum(0);
    }

    int max_LineSize = 0;
    for(const auto &l_Line: std::as_const(m_Lines))
    {
        int l_CurrentLineSize = l_Line.text.length();

        if (l_CurrentLineSize > max_LineSize)
        {
            max_LineSize = l_CurrentLineSize;
        }
    }

    if(max_LineSize - ((viewport()->width() - m_LineNumbersPanelWidth) / l_fontWidth) + 1 > 1)
    {
        horizontalScrollBar()->setMaximum(max_LineSize - ((viewport()->width() - m_LineNumbersPanelWidth) / l_fontWidth));
    }
    else
    {
        horizontalScrollBar()->setMaximum(0);
    }
}

void QCodeEditWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        if (m_SelectMouseDown)
        {
            m_SelectMouseDown = true;

            killTimer(m_CursorTimerID);
            m_CursorTimerID = startTimer(500);
            m_currentlyBlinkCursorShowen = 1;

            QFontMetrics l_fm(m_TextFont);
            int l_fontHeight = l_fm.height();
            int l_fontWidth = l_fm.horizontalAdvance(' ');

            m_CarretPosition.m_Row = m_ScrollYLinePos + (event->position().y() / l_fontHeight) + 1;

            if (m_CarretPosition.m_Row < 1)
            {
                m_CarretPosition.m_Row = 1;
            }

            if (m_CarretPosition.m_Row > (m_Lines.count()))
            {
                m_CarretPosition.m_Row = m_Lines.count();
            }

            m_CarretPosition.m_Column = (event->position().x() - 30 + (l_fontWidth / 2)) / l_fontWidth;

            if (m_CarretPosition.m_Column < 1)
            {
                m_CarretPosition.m_Column = 1;
            }

            if (m_CarretPosition.m_Column > m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1)
            {
                m_CarretPosition.m_Column = m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1;
            }

            viewport()->update();
        }
    }
}

void QCodeEditWidget::mousePressEvent(QMouseEvent *event)
{
    // TODO: unhardcode 30
    if (event->position().x() > 30)
    {
        m_SelectMouseDown = true;

        QFontMetrics l_fm(m_TextFont);
        int l_fontHeight = l_fm.height();
        int l_fontWidth = l_fm.horizontalAdvance(' ');

        m_CarretPosition.m_Row = m_ScrollYLinePos + (event->position().y() / l_fontHeight) + 1;

        if (m_CarretPosition.m_Row > (m_Lines.count()))
        {
            m_CarretPosition.m_Row = m_Lines.count();
        }

        m_CarretPosition.m_Column = (event->position().x() - 30 + (l_fontWidth / 2)) / l_fontWidth;

        if (m_CarretPosition.m_Column > m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1)
        {
            m_CarretPosition.m_Column = m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1;
        }

        if (m_CarretPosition.m_Column < 1)
        {
            m_CarretPosition.m_Column = 1;
        }

        m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
        m_SelectionPosition.m_Row = m_CarretPosition.m_Row;

        viewport()->update();
    }
}

void QCodeEditWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_SelectMouseDown = false;

    if ((event->button() == Qt::LeftButton)&&(event->position().x() < 16))
    {
        QFontMetrics l_fm(m_TextFont);
        int l_fontHeight = l_fm.height();

        int l_LineHited = m_ScrollYLinePos + (event->position().y() / l_fontHeight);

        if ((l_LineHited >= 0)&&(l_LineHited < m_Lines.count()))
        {
            emit breakpoint_change(l_LineHited + 1);
        }

        return;
    }
}

void QCodeEditWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier)
    {
        if ((event->angleDelta().ry() > 0)&&(m_FontSize < 48))
        {
            ++m_FontSize;
            m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
            m_TextFontBold = QFont("Source Code Pro", m_FontSize, QFont::Bold, false);
            viewport()->update();
        }
        else if ((event->angleDelta().ry() < 0)&&(m_FontSize > 3))
        {
            --m_FontSize;
            m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
            m_TextFontBold = QFont("Source Code Pro", m_FontSize, QFont::Bold, false);
            viewport()->update();
        }
    }
    else
    {
        QAbstractScrollArea::wheelEvent(event);
    }
}

void QCodeEditWidget::focusInEvent(QFocusEvent *event)
{
    m_currentlyBlinkCursorShowen = 1;
    if (m_CursorTimerID == 0)
    {
        m_CursorTimerID = startTimer(500);
    }
    viewport()->update();
}

void QCodeEditWidget::focusOutEvent(QFocusEvent *event)
{
    killTimer(m_CursorTimerID);
    m_CursorTimerID = 0;
    m_currentlyBlinkCursorShowen = 0;
    viewport()->update();
}

QString QCodeEditWidget::text() const
{
    QString ret;

    for(const auto &l_Line: m_Lines)
    {
        ret.append(l_Line.text);

        switch(l_Line.type)
        {
        case QTextParserLine::QTextParserLineTypeCREndLine:
            ret.append('\r');
            break;
        case QTextParserLine::QTextParserLineTypeLFEndLine:
            ret.append('\n');
            break;
        case QTextParserLine::QTextParserLineTypeCRLFEndLine:
            ret.append("\r\n");
            break;
        default:
            break;
        }
    }

    return ret;
}

int QCodeEditWidget::tabSize() const
{
    return m_tabSize;
}

void QCodeEditWidget::setFileExtension(const QString &extension)
{
}

void QCodeEditWidget::setText(QString text)
{
    QCodeEditWidgetLine l_Line;

    m_Lines.clear();
    m_ScrollXCharPos = 0;
    m_ScrollYLinePos = 0;

    int pos = 0;

    l_Line.lineStatus = QCodeEditWidget::QLineStatusTypeLineNotModified;
    l_Line.breakpointType = QCodeEditWidget::QBreakpointTypeNoBreakpoint;

    forever
    {
        int crindex = text.indexOf('\r', pos);
        int lfindex = text.indexOf('\n', pos);

        if ((crindex == -1)&&(lfindex == -1))
        {
            if(pos < text.length() - 1)
            {
                l_Line.text = text.right(text.length() - pos);
                l_Line.type = QTextParserLine::QTextParserLineTypeNoEndLine;

                m_Lines.push_back(l_Line);
            }

            break;
        }

        if ((crindex < lfindex)&&(crindex > -1))
        {
            l_Line.text = text.mid(pos, crindex - pos);
            l_Line.type = QTextParserLine::QTextParserLineTypeCREndLine;
            pos = crindex + 1;

            if((text.length() > pos)&&(text.at(pos) == '\n'))
            {
                l_Line.type = QTextParserLine::QTextParserLineTypeCRLFEndLine;
                pos++;
            }
        }
        else
        {
            l_Line.text = text.mid(pos, lfindex - pos);
            l_Line.type = QTextParserLine::QTextParserLineTypeLFEndLine;
            pos = lfindex + 1;
        }

        m_Lines.push_back(l_Line);
    }

    if ((m_Lines.isEmpty())||(m_Lines.last().type != QTextParserLine::QTextParserLineTypeNoEndLine))
    {
        l_Line.text = "";
        l_Line.type = QTextParserLine::QTextParserLineTypeNoEndLine;
        m_Lines.push_back(l_Line);
    }

    updatePanelWidth();

    emit parserUpdate();

    viewport()->update();
}

void QCodeEditWidget::setTabSize(int size)
{
    if ((size > 1)&&(m_tabSize != size))
    {
        m_tabSize = size;

        viewport()->update();
    }
}

void QCodeEditWidget::clearFormatting()
{
    for(int c = 0; c < m_Lines.count(); c++)
    {
        m_Lines[c].colors.clear();
    }

    viewport()->update();
}

void QCodeEditWidget::addFormat(int line, const QTextParserColorItem &item)
{
    m_Lines[line].colors.append(item);
}

void QCodeEditWidget::setFormatFromJavaParser(const QByteArray format)
{
    qsizetype pos = 0;

    for(auto &line: m_Lines)
    {
        line.colors.clear();

        qsizetype lineLen = line.text.length();

        if (lineLen > 0)
        {
            QByteArray lineformat = format.mid(pos, lineLen);

            qDebug() << "line: " << lineformat.toHex();

            int current_type_index = 0;
            uint8_t current_type = lineformat.at(0);

            for(int c = 1; c <= lineformat.length(); c++)
            {
                uint8_t type = 0xff;

                if (c < lineformat.length())
                {
                    type = lineformat.at(c);
                }

                if ((type != current_type)||(c == lineformat.length()))
                {
                    int additional = 0;
                    if (type == current_type) additional++;

                    switch(current_type)
                    {
                    case QJavaCFParserTokenTypes::STAGO:
                    case QJavaCFParserTokenTypes::ETAGO:
                    case QJavaCFParserTokenTypes::EXTAGC:
                    case QJavaCFParserTokenTypes::EXTAGCEMPTY:
                    case QJavaCFParserTokenTypes::EX_CFSCRIPT_TAGC:
                    case QJavaCFParserTokenTypes::END_CFSCRIPT:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xab, 0xd9, 0x0c), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::CFIF:
                    case QJavaCFParserTokenTypes::CFELSEIF:
                    case QJavaCFParserTokenTypes::CFSET:
                    case QJavaCFParserTokenTypes::CFRETURN:
                    case QJavaCFParserTokenTypes::CFELSE:
                    case QJavaCFParserTokenTypes::CFLOOP:
                    case QJavaCFParserTokenTypes::CFBREAK:
                    case QJavaCFParserTokenTypes::CFCONTINUE:
                    case QJavaCFParserTokenTypes::CFTRY:
                    case QJavaCFParserTokenTypes::CFCATCH:
                    case QJavaCFParserTokenTypes::CFFINALLY:
                    case QJavaCFParserTokenTypes::CFRETHROW:
                    case QJavaCFParserTokenTypes::CFIMPORT:
                    case QJavaCFParserTokenTypes::CFARGUMENT:
                    case QJavaCFParserTokenTypes::CFFUNCTION:
                    case QJavaCFParserTokenTypes::CFCOMPONENT:
                    case QJavaCFParserTokenTypes::CFPROPERTY:
                    case QJavaCFParserTokenTypes::CFSWITCH:
                    case QJavaCFParserTokenTypes::CFINTERFACE:
                    case QJavaCFParserTokenTypes::CFCASE:
                    case QJavaCFParserTokenTypes::CFDEFAULTCASE:
                    case QJavaCFParserTokenTypes::CFPROCESSINGDIRECTIVE:
                    case QJavaCFParserTokenTypes::CFBODYTAG:
                    case QJavaCFParserTokenTypes::CFSCRIPT_STMT:
                    case QJavaCFParserTokenTypes::CFTAG:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0x56, 0x9c, 0xd6), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::TagAttrName:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xf5, 0xcd, 0x76), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::TagAttrEquals:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xcc, 0xcc, 0xcc), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::HASHMARK:
                    case QJavaCFParserTokenTypes::LiveHashMark:
                    case QJavaCFParserTokenTypes::HASH:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0x56, 0x9c, 0xd6), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::Variable:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0x9c, 0xdc, 0xfe), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::ELSE_STMT:
                    case QJavaCFParserTokenTypes::IF_STMT:
                    case QJavaCFParserTokenTypes::FOR_STMT:
                    case QJavaCFParserTokenTypes::WHILE_STMT:
                    case QJavaCFParserTokenTypes::DO_STMT:
                    case QJavaCFParserTokenTypes::SWITCH_STMT:
                    case QJavaCFParserTokenTypes::BREAK_STMT:
                    case QJavaCFParserTokenTypes::CONTINUE_STMT:
                    case QJavaCFParserTokenTypes::IMPORT_STMT:
                    case QJavaCFParserTokenTypes::CFIMPORT_STMT:
                    case QJavaCFParserTokenTypes::CASE_SUBSTMT:
                    case QJavaCFParserTokenTypes::DEFAULT_SUBSTMT:
                    case QJavaCFParserTokenTypes::PROCESSINGDIRECTIVE_STMT:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xc5, 0x86, 0xc0), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::IMP:
                    case QJavaCFParserTokenTypes::EQV:
                    case QJavaCFParserTokenTypes::XOR:
                    case QJavaCFParserTokenTypes::OR:
                    case QJavaCFParserTokenTypes::AND:
                    case QJavaCFParserTokenTypes::NOT:
                    case QJavaCFParserTokenTypes::EQ:
                    case QJavaCFParserTokenTypes::NEQ:
                    case QJavaCFParserTokenTypes::EQ_STRICT_OP:
                    case QJavaCFParserTokenTypes::NEQ_STRICT_OP:
                    case QJavaCFParserTokenTypes::EQ_OP:
                    case QJavaCFParserTokenTypes::NEQ_OP:
                    case QJavaCFParserTokenTypes::LT:
                    case QJavaCFParserTokenTypes::LTE:
                    case QJavaCFParserTokenTypes::GT:
                    case QJavaCFParserTokenTypes::GTE:
                    case QJavaCFParserTokenTypes::CONTAINS:
                    case QJavaCFParserTokenTypes::DOES_NOT_CONTAIN:
                    case QJavaCFParserTokenTypes::CONCAT:
                    case QJavaCFParserTokenTypes::ADD:
                    case QJavaCFParserTokenTypes::SUBTRACT:
                    case QJavaCFParserTokenTypes::CARET:
                    case QJavaCFParserTokenTypes::MOD:
                    case QJavaCFParserTokenTypes::INT_DIV:
                    case QJavaCFParserTokenTypes::MULTIPLY:
                    case QJavaCFParserTokenTypes::DIVIDE:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0x35, 0xb2, 0xdd), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::TRUE:
                    case QJavaCFParserTokenTypes::FALSE:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xb5, 0xce, 0xa8), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::FUNCTION:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xc5, 0xf5, 0x66), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::OPEN_PARENTHESIS:
                    case QJavaCFParserTokenTypes::CLOSE_PARENTHESIS:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xff, 0xd7, 0x00), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::OPEN_BRACE:
                    case QJavaCFParserTokenTypes::CLOSE_BRACE:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xda, 0x70, 0xd6), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::INTEGER:
                    case QJavaCFParserTokenTypes::DOUBLE:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xb5, 0xce, 0xa8), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::CFS_SINGLE_LINE_COMMENT_START:
                    case QJavaCFParserTokenTypes::CFS_FORMAL_COMMENT_START:
                    case QJavaCFParserTokenTypes::CFS_MULTI_LINE_COMMENT_START:
                    case QJavaCFParserTokenTypes::SINGLE_LINE_COMMENT:
                    case QJavaCFParserTokenTypes::FORMAL_COMMENT:
                    case QJavaCFParserTokenTypes::SINGLE_LINE_COMMENT_CLOSURE:
                    case QJavaCFParserTokenTypes::FORMAL_COMMENT_CLOSURE:
                    case QJavaCFParserTokenTypes::MULTI_LINE_COMMENT_CLOSURE:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0x6a, 0x99, 0x55), QColor()));
                        break;

                    case QJavaCFParserTokenTypes::TagAttrDoubleQuote:
                    case QJavaCFParserTokenTypes::DoubleQuoteString:
                    case QJavaCFParserTokenTypes::DoubleQuoteEscape:
                    case QJavaCFParserTokenTypes::StartDoubleQuoteStringLiteral:
                    case QJavaCFParserTokenTypes::EndDoubleQuoteString:
                    case QJavaCFParserTokenTypes::TagAttrSingleQuote:
                    case QJavaCFParserTokenTypes::SingleQuoteString:
                    case QJavaCFParserTokenTypes::SingleQuoteEscape:
                    case QJavaCFParserTokenTypes::StartSingleQuoteStringLiteral:
                    case QJavaCFParserTokenTypes::EndSingleQuoteString:
                        qDebug() << "current_type_index: "<< current_type_index << ", c: " << c << "additional: " << additional << ", len: " << c - current_type_index + additional;
                        line.colors.append(QTextParserColorItem(current_type_index, c - current_type_index + additional, QColor(0xce, 0x91, 0x78), QColor()));
                        break;
                    }

                    current_type = type;
                    current_type_index = c;
                }
            }
        }

        pos += lineLen;
        switch(line.type)
        {
        case QTextParserLine::QTextParserLineTypeCREndLine:   pos += 1; break;
        case QTextParserLine::QTextParserLineTypeLFEndLine:   pos += 1; break;
        case QTextParserLine::QTextParserLineTypeCRLFEndLine: pos += 2; break;
        default:;
        }
    }

    viewport()->update();
}

void QCodeEditWidget::setBreakpoint(int line, QBreakpointType type)
{
    auto l_line = m_Lines.at(line - 1);

    l_line.breakpointType = type;

    m_Lines.replace(line - 1, l_line);

    viewport()->update();
}

void QCodeEditWidget::setDebuggerAtLine(int line)
{
    m_debuggerAtLine = line - 1;

    update();
}

QCodeEditWidget::QBreakpointType QCodeEditWidget::breakpoint(int line) const
{
    return m_Lines.at(line - 1).breakpointType;
}

bool QCodeEditWidget::canUndo() const
{
    return false;
}

bool QCodeEditWidget::canRedo() const
{
    return false;
}

bool QCodeEditWidget::hasSelection() const
{
    return false;
}

void QCodeEditWidget::setLines(QList<QCodeEditWidgetLine> lines)
{
    m_Lines = lines;

    viewport()->update();
}

QList<QCodeEditWidgetLine> QCodeEditWidget::lines() const
{
    return m_Lines;
}
