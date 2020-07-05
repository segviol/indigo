#pragma once
#include <string>
#include <ctype.h>
#include <iostream>
#include <vector>
#include <unordered_map>

#ifndef COMPILER_FRONT_WORD_H_
#define COMPILER_FRONT_WORD_H_

namespace front::word {
    using std::string;
    using std::vector;
    using std::hash;
    using std::isalpha;
    using std::isdigit;

    enum class Token
    {
        IDENFR, INTCON, STRCON,
        CONSTTK, INTTK, VOIDTK,
        IFTK, ELSETK, WHILETK,
        BREAK, CONTINUETK, RETURNTK,
        PLUS, MINU, MULT,
        DIV, MOD, LSS,
        LEQ, GRE, GEQ,
        EQL, NEQ, AND,
        OR, NOT, ASSIGN,
        SEMICN, COMMA, LPARENT,
        RPARENT, LBRACK, RBRACK,
        LBRACE, RBRACE
    };

    const string Token_strs[] = {
        "IDENFR", "INTCON", "STRCON",
        "CONSTTK","INTTK", "VOIDTK",
        "IFTK", "ELSETK", "WHILETK",
        "BREAK", "CONTINUETK", "RETURNTK",
        "PLUS", "MINU", "MULT",
        "DIV", "MOD", "LSS",
        "LEQ", "GRE", "GEQ",
        "EQL", "NEQ", "AND",
        "OR", "NOT", "ASSIGN",
        "SEMICN", "COMMA", "LPARENT",
        "RPARENT", "LBRACK", "RBRACK",
        "LBRACE", "RBRACE"
    };

    inline bool octal_digit(char ch);
    inline bool hex_digit(char ch);
    inline bool is_nodigit(char c);
    inline bool have_body_char(char c);
    inline void get_char(char& ch, size_t& index, const string& str);
    inline void retract(char& ch, size_t& index, const string& str);
    inline bool is_illegel_char(char ch);
    int stringToInt(string s);
    Token get_token(const string& buf);

    #define VECTOR_SIZE 1024

    class Word {
    public:
        Word();
        Word(Token in_token_value, string in_self, unsigned int in_line_num);
        Token get_token_value();
        string get_token_string();
        string get_self() const;
        bool match_token(Token tk) const;
        unsigned int get_line_num() const;
    private:
        Token token_value;
        string self;
        unsigned int line_num;
    };

    struct WordHash
    {
        size_t operator()(const Word& word) const {
            return hash<string>()(word.get_self());
        }
    };

    struct WordCmp
    {
        bool operator()(const Word& a, const Word& b) const {
            return a.get_self() == b.get_self();
        }
    };

    void word_analyse(string& input_str, vector<Word>& word_arr);
}

#endif // !COMPILER_FRONT_WORD_H_
