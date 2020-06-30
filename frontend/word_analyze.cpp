#include "word_analyze.hpp"

using namespace front::word;

unsigned int line_num = 1;

void front::word::word_analyse(string& input_str, vector<Word>& word_arr)
{
    string buf;
    int index = 0;
    char ch = 0;
    input_str = '\0' + input_str;
    while (index < input_str.size())
    {
        get_char(ch, index, input_str);
        buf.clear();
        while ((!have_body_char(ch) || is_illegel_char(ch)) && index + 1 < input_str.size())
        {
            get_char(ch, index, input_str);
        }

        if (is_letter(ch))
        {
            do
            {
                buf += ch;
                get_char(ch, index, input_str);
            } while (is_letter(ch) || isdigit(ch));
            retract(ch, index, input_str);
            word_arr.push_back(Word(get_token(buf), buf, line_num));
        }
        else if (isdigit(ch))
        {
            if (ch != '0')
            {
                do
                {
                    buf += ch;
                    get_char(ch, index, input_str);
                } while (isdigit(ch));
                retract(ch, index, input_str);
            }
            else
            {
                buf += toupper(ch);
                get_char(ch, index, input_str);
                if (ch == 'x' || ch == 'X')
                {
                    buf += toupper(ch);
                    get_char(ch, index, input_str);
                    do
                    {
                        buf += toupper(ch);
                        get_char(ch, index, input_str);
                    } while (hex_digit(ch));
                    retract(ch, index, input_str);
                }
                else if (octal_digit(ch))
                {
                    do
                    {
                        buf += ch;
                        get_char(ch, index, input_str);
                    } while (isdigit(ch));
                    retract(ch, index, input_str);

                }
                else
                {
                    retract(ch, index, input_str);
                }
            }
            word_arr.push_back(Word(Token::INTCON, buf, line_num));
        }
        else if (ch == '\"')
        {
            get_char(ch, index, input_str);
            while (ch != '\"')
            {
                buf += ch;
                get_char(ch, index, input_str);
            }
            word_arr.push_back(Word(Token::STRCON, buf, line_num));
        }
        else if (ch == '+')
        {
            buf += ch;
            word_arr.push_back(Word(Token::PLUS, buf, line_num));
        }
        else if (ch == '-')
        {
            buf += ch;
            word_arr.push_back(Word(Token::MINU, buf, line_num));
        }
        else if (ch == '*')
        {
            buf += ch;
            word_arr.push_back(Word(Token::MULT, buf, line_num));
        }
        else if (ch == '/')
        {
            buf += ch;
            get_char(ch, index, input_str);
            if (ch == '/')
            {
                do
                {
                    get_char(ch, index, input_str);
                } while (ch != '\n');
            }
            else if (ch == '*')
            {
                int stt = 0;
                do
                {
                    get_char(ch, index, input_str);
                    if (stt == 0)
                    {
                        if (ch == '*')
                        {
                            stt = 1;
                        }
                    }
                    if (stt == 1)
                    {
                        if (ch == '/')
                        {
                            stt = 2;
                        }
                        else if (ch == '*')
                        {
                            stt = 1;
                        }
                        else
                        {
                            stt = 0;
                        }
                    }
                } while (stt < 2);
            }
            else
            {
                retract(ch, index, input_str);
                word_arr.push_back(Word(Token::DIV, buf, line_num));
            }
        }
        else if (ch == '%')
        {
            buf += ch;
            word_arr.push_back(Word(Token::MOD, buf, line_num));
        }
        else if (ch == '<')
        {
            buf += ch;
            get_char(ch, index, input_str);
            if (ch == '=')
            {
                buf += ch;
                word_arr.push_back(Word(Token::LEQ, buf, line_num));
            }
            else
            {
                retract(ch, index, input_str);
                word_arr.push_back(Word(Token::LSS, buf, line_num));
            }
        }
        else if (ch == '>')
        {
            buf += ch;
            get_char(ch, index, input_str);
            if (ch == '=')
            {
                buf += ch;
                word_arr.push_back(Word(Token::GEQ, buf, line_num));
            }
            else
            {
                retract(ch, index, input_str);
                word_arr.push_back(Word(Token::GRE, buf, line_num));
            }
        }
        else if (ch == '=')
        {
            buf += ch;
            get_char(ch, index, input_str);
            if (ch == '=')
            {
                buf += ch;
                word_arr.push_back(Word(Token::EQL, buf, line_num));
            }
            else
            {
                retract(ch, index, input_str);
                word_arr.push_back(Word(Token::ASSIGN, buf, line_num));
            }
        }
        else if (ch == '!')
        {
            buf += ch;
            get_char(ch, index, input_str);
            if (ch == '=')
            {
                buf += ch;
                word_arr.push_back(Word(Token::NEQ, buf, line_num));
            }
            else
            {
                retract(ch, index, input_str);
                word_arr.push_back(Word(Token::NOT, buf, line_num));
            }

        }
        else if (ch == '&')
        {
            buf += ch;
            get_char(ch, index, input_str);
            buf += ch;
            word_arr.push_back(Word(Token::AND, buf, line_num));
        }
        else if (ch == '|')
        {
            buf += ch;
            get_char(ch, index, input_str);
            buf += ch;
            word_arr.push_back(Word(Token::OR, buf, line_num));
        }
        else if (ch == ';')
        {
            buf += ch;
            word_arr.push_back(Word(Token::SEMICN, buf, line_num));
        }
        else if (ch == ',')
        {
            buf += ch;
            word_arr.push_back(Word(Token::COMMA, buf, line_num));
        }
        else if (ch == '(')
        {
            buf += ch;
            word_arr.push_back(Word(Token::LPARENT, buf, line_num));
        }
        else if (ch == ')')
        {
            buf += ch;
            word_arr.push_back(Word(Token::RPARENT, buf, line_num));
        }
        else if (ch == '[')
        {
            buf += ch;
            word_arr.push_back(Word(Token::LBRACK, buf, line_num));
        }
        else if (ch == ']')
        {
            buf += ch;
            word_arr.push_back(Word(Token::RBRACK, buf, line_num));
        }
        else if (ch == '{')
        {
            buf += ch;
            word_arr.push_back(Word(Token::LBRACE, buf, line_num));
        }
        else if (ch == '}')
        {
            buf += ch;
            word_arr.push_back(Word(Token::RBRACE, buf, line_num));
        }

        if (index == input_str.size() - 1)
        {
            break;
        }
    }
}

inline void front::word::get_char(char& ch, int& index, const string& str)
{
    if (index + 1 < str.size()) {
        ch = str[++index];
        if (ch == '\n')
        {
            line_num++;
        }
    }
}

inline bool front::word::octal_digit(char ch)
{
    return ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7';
}

inline bool front::word::hex_digit(char ch)
{
    char l_ch = tolower(ch);
    return isdigit(ch) || l_ch == 'a' || l_ch == 'b' || l_ch == 'c' || l_ch == 'd' || l_ch == 'e' || l_ch == 'f';
}

inline void front::word::retract(char& ch, int& index, const string& str)
{
    if (ch == '\n')
    {
        line_num--;
    }
    ch = str[--index];
}

inline bool front::word::is_letter(char c)
{
    return isalpha(c);
}

inline bool front::word::have_body_char(char c)
{
    return (c >= 33 && c <= 126);
}

inline bool front::word::is_illegel_char(char ch)
{
    return (ch == '`' || ch == '~' || ch == '@' || ch == '#' || ch == '$' || ch == '^');
}

int front::word::stringToInt(string s)
{
    int value;
    if (s[0] != '0')
    {
        value = stoi(s, NULL, 10);
    }
    else if (s.size() > 1 && s[1] == 'X')
    {
        value = stoi(s, NULL, 16);
    }
    else
    {
        value = stoi(s, NULL, 8);
    }
    return value;
}

Token front::word::get_token(const string& buf)
{
    if (buf == "const")
    {
        return Token::CONSTTK;
    }
    else if (buf == "int")
    {
        return Token::INTTK;
    }
    else if (buf == "void")
    {
        return Token::VOIDTK;
    }
    else if (buf == "if")
    {
        return Token::IFTK;
    }
    else if (buf == "else")
    {
        return Token::ELSETK;
    }
    else if (buf == "while")
    {
        return Token::WHILETK;
    }
    else if (buf == "break")
    {
        return Token::BREAK;
    }
    else if (buf == "continue")
    {
        return Token::CONTINUETK;
    }
    else if (buf == "return")
    {
        return Token::RETURNTK;
    }
    else
    {
        return Token::IDENFR;
    }
}

Word::Word() {
    this->line_num = -1;
    this->token_value = Token::AND;
}

Word::Word(Token in_token_value, string in_self, unsigned int in_line_num)
{
    token_value = in_token_value;
    self = in_self;
    line_num = in_line_num;
}

Token Word::get_token_value()
{
    return token_value;
}

string Word::get_token_string()
{
    return Token_strs[(int)token_value];
}

string Word::get_self() const
{
    return self;
}

bool Word::match_token(Token tk) const
{
    return token_value == tk;
}

unsigned int Word::get_line_num() const
{
    return line_num;
}