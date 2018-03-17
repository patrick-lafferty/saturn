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

            if (c == '(' || c == '[' || c == '{') {
                brackets.push(c);
                expressions.push(new List);
            }
            else if (c == ')' || c == ']' || c == '}') {
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
                    
                }
            }
        }

        return new List{topLevelExpressions};
    }
}