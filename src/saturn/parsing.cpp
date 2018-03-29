/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "parsing.h"
#include <stack>
#include <ctype.h>
#include <stdlib.h>

using namespace std;

vector<string_view> split(string_view s, char separator, bool includeSeparator) {
    vector<string_view> substrings;

    while (!s.empty()) {
        if (*s.data() == separator) {

            if (includeSeparator) {
                substrings.push_back(s.substr(0, 1));
            }

            s.remove_prefix(1);
        }

        auto nextSplit = s.find(separator);

        if (nextSplit == string_view::npos) {
            break;
        }

        substrings.push_back(s.substr(0, nextSplit));
        s.remove_prefix(nextSplit);
    }

    if (!s.empty()) {
        substrings.push_back(s);
    }

    return substrings;
}

namespace Saturn::Parse {

    bool bracketMatches(char opener, char closer) {
        if (opener == '(') {
            return closer == ')';
        }
        else if (opener == '[') {
            return closer == ']';
        }
        else if (opener == '{') {
            return closer == '}';
        }
        else {
            return false;
        }
    }

    enum class LiteralType {
        Int,
        Float,
        String,
        None
    };

    std::variant<SExpression*, ParseError> read(std::string_view input) {
        auto length = input.length();
        std::stack<char> brackets;
        std::stack<SExpression*> expressions;
        int currentLine {0};
        int currentColumn {0};
        List topLevelExpressions;

        for (unsigned int i = 0u; i < length; i++) {
            auto c = input[i];
            currentColumn++;

            switch (c) {
                case '(':
                case '[':
                case '{': {

                    brackets.push(c);
                    expressions.push(new List);
                    break;
                }
                case ')':
                case ']':
                case '}': {

                    if (brackets.empty() || expressions.empty()) {
                        return ParseError {"Unexpected closing bracket", currentLine, currentColumn};
                    }

                    if (!bracketMatches(brackets.top(), c)) {
                        return ParseError {"Mismatched bracket", currentLine, currentColumn};
                    }

                    auto completedExpression = expressions.top();
                    expressions.pop();

                    if (expressions.empty()) {
                        topLevelExpressions.items.push_back(completedExpression);
                    }
                    else {
                        if (expressions.top()->type == SExpType::List) {
                            auto list = static_cast<List*>(expressions.top());
                            list->items.push_back(completedExpression);
                        }
                        else {
                            return ParseError {"Expected list", currentLine, currentColumn};
                        }
                    }

                    break;
                }
                case '\n': {
                    currentLine++;
                    currentColumn = 0;
                    break;
                }
                case ' ':
                case '\t': {
                    continue;
                }
                default: {

                    if (expressions.top()->type != SExpType::List) {
                        return ParseError {"Expected list", currentLine, currentColumn};
                    }

                    auto list = static_cast<List*>(expressions.top());

                    LiteralType type {LiteralType::None};

                    if (isdigit(c)) {
                        type = LiteralType::Int;
                    }
                    else if (c == '"') {
                        type = LiteralType::String;
                    }
                    else {
                        auto substring = input.substr(i);

                        if (substring.compare(0, 4, "true") == 0
                            && substring.find_first_of(" )]}") == 4) {
                            list->items.push_back(new BoolLiteral {true});
                            i += 4;
                            continue;
                        }
                        else if (substring.compare(0, 5, "false") == 0
                            && substring.find_first_of(" )]}") == 5) {
                            list->items.push_back(new BoolLiteral {false});
                            i += 5;
                            continue;
                        }
                    }

                    bool done {false};
                    auto j = i;

                    for (; j < length; j++) {

                        c = input[j];

                        switch (type) {
                            case LiteralType::Int: {
                                
                                if (c == '.') {
                                    type = LiteralType::Float;
                                }
                                else if (!isdigit(c)) {
                                    done = true;
                                    break;
                                }

                                break;
                            }
                            case LiteralType::Float: {

                                if (c == '.') {
                                    return ParseError {"Unexpected second '.' in float literal", currentLine, currentColumn};
                                }
                                else if (!isdigit(c)) {
                                    done = true;
                                    break;
                                }

                                break;
                            }
                            case LiteralType::String: {
                                if (c == '"' && j > i) {
                                    j++;
                                    done = true;
                                }

                                break;
                            }
                            case LiteralType::None: {
                                if (c == '(' || c == ')' || c == '\n' || c == ' ' || c == '\t') {
                                    done = true;
                                }

                                break;
                            }
                        }

                        if (done) {
                            break;
                        }
                    }

                    auto value = input.substr(i, j - i);

                    switch (type) {
                        case LiteralType::Int: {
                            list->items.push_back(new IntLiteral {strtol(value.data(), nullptr, 10)});
                            break;
                        }
                        case LiteralType::Float: {
                            return ParseError {"Float parsing not implemented", currentLine, currentColumn};
                            break;
                        }
                        case LiteralType::String: {
                            list->items.push_back(new StringLiteral {value});
                            break;
                        }
                        case LiteralType::None: {
                            auto symbol = new Symbol {value};
                            list->items.push_back(symbol);

                            for (auto s : list->items) {
                                auto pause = 0;
                            }
                            break;
                        }
                    }

                    i += (j - i) - 1;

                    break;
                }
            }
        }

        return new List{topLevelExpressions};
    }

    std::optional<Constructor> getConstructor(SExpression* s) {
        if (s->type != SExpType::List) {
            return {};
        }

        auto values = static_cast<List*>(s);

        if (values->items.empty()) {
            return {};
        }

        if (values->items[0]->type != SExpType::Symbol) {
            return {};
        }

        return Constructor {static_cast<Symbol*>(values->items[0]), values};
    }

    bool Constructor::startsWith(const char* str) {
        return name->value.compare(str) == 0;
    }
}