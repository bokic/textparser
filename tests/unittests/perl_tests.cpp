#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <perl_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Perl, basic_perl_program) {
    auto tokens = TextParser(R"(
#!/usr/bin/perl
# Perl package with OO support
package MyApp::Greeter;

use strict;
use warnings;
use Moose;

has name => (is => 'ro', required => 1);

sub greet {
    my ($self) = @_;
    my $msg = "Hello, " . $self->name . "!";
    print "$msg\n";
    return $msg;
}

sub format_message {
    my ($self, $text) = @_;
    chomp $text;
    $text =~ s/^\s+|\s+$//g;
    return ucfirst(lc($text));
}

1;
)", &perl_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
}
