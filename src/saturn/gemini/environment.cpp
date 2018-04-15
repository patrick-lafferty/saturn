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

#include "environment.h"

//using namespace Saturn::Parse;

namespace Saturn::Gemini {

    Object::Object(Saturn::Parse::List* list) {
        type = GeminiType::Object;

        bool skip {true};
        for (auto item : list->items) {
            if (skip) {
                skip = false;
                continue;
            }

            if (item->type == Saturn::Parse::SExpType::List) {
                if (auto maybeConstructor = Saturn::Parse::getConstructor(item)) {
                    auto& constructor = maybeConstructor.value();
                    if (auto maybeValue = constructor.get<Saturn::Parse::IntLiteral*>(1, Saturn::Parse::SExpType::IntLiteral)) {
                        addChild(constructor.name->value, 
                            new Integer(maybeValue.value()->value));
                    }       
                }
            }
        }
    }

    Function::Function(FunctionSignature signature, std::function<CallResult(List*)> func)
        : signature{signature}, f {func} {
        type = GeminiType::Function;
    }

    bool checkType(Type input, Saturn::Parse::SExpression* arg) {

        switch (input) {
            case Type::String: {

                switch (arg->type) {
                    case Saturn::Parse::SExpType::StringLiteral: {
                        return true;
                    }
                    case Saturn::Parse::SExpType::Symbol: {
                        auto symbol = static_cast<Saturn::Parse::Symbol*>(arg)->value;

                        if (symbol.empty()) {
                            return false;
                        }
                        else if (symbol[0] == '/') {
                            return true;
                        }
                        else {
                            return false;
                        }
                    }
                    default: return false;
                }

                break;
            }
            case Type::Bool: {
                switch (arg->type) {
                    case Saturn::Parse::SExpType::BoolLiteral: {
                        return true;
                    }
                    default: return false;
                }

                break;
            }
            case Type::Object: {
                switch (arg->type) {
                    case Saturn::Parse::SExpType::List: {
                        return true;
                    }
                    default: return false;
                }

                break;
            }
            case Type::Any: {
                return true;
            }
        }

        return false;
    }

    std::variant<ArgumentMismatch, Object*> Function::call(List* arguments) {
        if (f) {

            /*int argCount = arguments->items.size() - 1;
            
            if (argCount != signature.input.size()) {
                return ArgumentMismatch{};
            }*/

            /*if (argCount > 0) {
                for (int i = 0; i < argCount; i++) {
                    auto arg = arguments->items[i + 1];

                    if (!checkType(signature.input[i], arg)) {
                        return ArgumentMismatch{};
                    }
                }
            }*/

            return f(arguments);
        }

        return {};
    }

    std::optional<Function*> Environment::lookupFunction(Saturn::Parse::Symbol* symbol) {
        auto hash = std::hash<std::string_view>{}(symbol->value);
        auto it = functions.find(hash);

        if (it != end(functions)) {
            return it->second;
        }
        else {
            if (parent != nullptr) {
                return parent->lookupFunction(symbol);
            }
            else {
                return {};
            }
        }
    }

    std::optional<Object*> Environment::lookupObject(Saturn::Parse::Symbol* symbol) {
        return lookupObject(symbol->value);
    }

    std::optional<Object*> Environment::lookupObject(std::string_view name) {
        auto hash = std::hash<std::string_view>{}(name);
        auto it = objects.find(hash);

        if (it != end(objects)) {
            return it->second;
        }
        else {
            if (parent != nullptr) {
                return parent->lookupObject(name);
            }
            else {
                return {};
            }
        }
    }

    void Environment::addFunction(std::string_view name, Function* function) {
        auto hash = std::hash<std::string_view>{}(name);
        functions[hash] = function;
    }

    void Environment::addObject(std::string_view name, Object* object) {
        auto hash = std::hash<std::string_view>{}(name);
        objects[hash] = object;
    }
}
