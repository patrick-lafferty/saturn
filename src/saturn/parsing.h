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
#include <optional>

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
		SExpression() {}
		SExpression(SExpType t)
			: type {t} {}

		SExpType type;
	};

	struct Symbol : SExpression {

		Symbol() : SExpression(SExpType::Symbol) {}

		Symbol(std::string_view value) 
			: SExpression(SExpType::Symbol) {
			this->value = value;
		}

		std::string_view value;
	};

	template<class T>
	struct Literal : SExpression {
		T value;

	protected:

		Literal(SExpType t) : SExpression(t) {}
	};

	struct IntLiteral : Literal<int> {
		IntLiteral(int value = 0) 	
			: Literal(SExpType::IntLiteral) {
			this->value = value;
		}
	};

	struct FloatLiteral : Literal<float> {
		FloatLiteral(float value = 0.f) 
			: Literal(SExpType::FloatLiteral) {
			this->value = value;
		}
	};

	struct StringLiteral : Literal<std::string_view> {
		StringLiteral(std::string_view value = {}) 
			: Literal(SExpType::StringLiteral) {
			this->value = value;
		}
	};

	struct BoolLiteral : Literal<bool> {
		BoolLiteral(bool value = false) 
			: Literal(SExpType::BoolLiteral) {
			this->value = value;
		}
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

	struct Constructor {
		Symbol* name;
		List* values;
		int length;

		bool startsWith(const char* str);

		template<typename T>
		std::optional<T> get(int index, SExpType type) {
			if (values->items[index]->type == type) {
				return {static_cast<T>(values->items[index])};
			}
			else {
				return {};
			}
		}
	};

	std::variant<SExpression*, ParseError> read(std::string_view input);

	std::optional<Constructor> getConstructor(SExpression* s);
}
