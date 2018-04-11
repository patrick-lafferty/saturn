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

#include "../parsing.h"
#include <map>
#include <functional>
#include <string_view>
#include <variant>

namespace Saturn::Gemini {

    enum class Type {
        String,
        Void
    };

    struct FunctionSignature {
        std::vector<Type> input;
        Type output;
    };

    struct ArgumentMismatch {

    };

    struct Result {
        std::variant<char*> value;
    };

    typedef std::variant<ArgumentMismatch, Result> CallResult;

    class Function {
    public:

        Function() = default;
        Function(FunctionSignature signature, std::function<CallResult(Saturn::Parse::List*)> func);
        CallResult call(Saturn::Parse::List* arguments);

    private:

        FunctionSignature signature;
        std::function<CallResult(Saturn::Parse::List*)> f;
    };

    class Environment {
    public:

        std::optional<Function> lookupFunction(Saturn::Parse::Symbol* symbol);

        void addFunction(std::string_view name, Function function);

    private:

        Environment* parent {nullptr};
        std::map<std::size_t, Function> functions;
    };
}