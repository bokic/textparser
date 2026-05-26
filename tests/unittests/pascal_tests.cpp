#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <pascal_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Pascal, basic_delphi_program) {
    auto tokens = TextParser(R"(
// Delphi unit with class definition
{$HINT ON}
{ Helper comment }
(* Another comment *)
unit MyUnit;

interface

uses
  SysUtils, Classes;

type
  TMyClass = class(TObject)
  private
    FName: string;
    FCount: Integer;
  public
    constructor Create(const AName: string);
    destructor Destroy; override;
    property Name: string read FName write FName;
    property Count: Integer read FCount write FCount;
    function GetStatus: Boolean;
  end;

implementation

constructor TMyClass.Create(const AName: string);
begin
  FName := AName;
  FCount := 0;
end;

destructor TMyClass.Destroy;
begin
  inherited;
end;

function TMyClass.GetStatus: Boolean;
var
  code: Integer;
begin
  code := $FF;
  if FCount > 100 then
    Result := True
  else
    Result := False;
end;

const
  AppName = 'MyApp';

initialization
  WriteLn(#65#66#67);
finalization
end.
)", &pascal_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("CompilerDirective"));
    EXPECT_TRUE(found.contains("BlockCommentCurly"));
    EXPECT_TRUE(found.contains("BlockCommentParen"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("CharLiteral"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
}
