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
#pragma once

#include <string_view>
#include <vector>
#include <variant>
#include <string>

std::vector<std::string_view> split(std::string_view s, char separator, bool includeSeparator = false);

namespace Saturn::Parse {

	enum class SExpType {
		Symbol,
		IntLiteral,
		FloatLiteral,
		StringLiteral,
		BoolLiteral,
		List
	};

	struct SExpression {
		SExpression(SExpType t)
			: type {t} {}

		SExpType type;
	};

	struct Symbol : SExpression {
		Symbol() : SExpression(SExpType::Symbol) {}

		std::string value;
	};

	template<class T>
	struct Literal : SExpression {
		T value;

	protected:

		Literal(SExpType t) : SExpression(t) {}
	};

	struct IntLiteral : Literal<int> {
		IntLiteral() : Literal(SExpType::IntLiteral) {}
	};

	struct FloatLiteral : Literal<float> {
		FloatLiteral() : Literal(SExpType::FloatLiteral) {}
	};

	struct StringLiteral : Literal<char*> {
		StringLiteral() : Literal(SExpType::StringLiteral) {}
	};

	struct BoolLiteral : Literal<bool> {
		BoolLiteral() : Literal(SExpType::BoolLiteral) {}
	};

	struct List : SExpression {
		List() : SExpression(SExpType::List) {}

		std::vector<SExpression*> items;
	};

	struct ParseError {
		char message[60];
		int lineNumber;
		int columnNumber;
	};

	std::variant<SExpression*, ParseError> read(std::string_view input);
}