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
#include "interpreter.h"

//using namespace Saturn::Parse;

namespace Saturn::Gemini {

    CallResult interpret(Saturn::Parse::List* list, Environment& environment) {
        if (list->items.empty()) {
            return {};
        }

        if (list->items[0]->type == Saturn::Parse::SExpType::Symbol) {
            auto name = static_cast<Saturn::Parse::Symbol*>(list->items[0]);

            if (name->value.compare("lambda") == 0) {
                if (list->items.size() == 3) {
                    FunctionSignature signature {{Type::Any}, Type::Any};
                    return new Function {signature, [list, &environment](auto args) {
                        CallResult result {};

                        /*bool skip {true};
                        for (auto child : list->items) {
                            if (skip) {
                                skip = false;
                                continue;
                            }

                            result = interpret(child, environment);
                        }*/
                        auto child = environment.makeChild();
                        child.mapParameter(static_cast<Saturn::Parse::Symbol*>(list->items[1]),
                            args->items[0]);

                        result = interpret(list->items[2], child);

                        return result;
                    }};
                }
                else {
                    return {};
                }
            }
            else {

                if (auto maybeFunc = environment.lookupFunction(name)) {
                    auto& func = maybeFunc.value();
                    auto arguments = map(list, [&](auto item) {
                        auto result = interpret(item, environment);

                        if (std::holds_alternative<Object*>(result)) {
                            return std::get<Object*>(result);
                        }
                        else {
                            return (Object*)nullptr;
                        }
                    }, environment);

                    return func->call(arguments);
                }
            }
        }

        return {};
    }

    CallResult interpret(Saturn::Parse::SExpression* s, Environment& environment) {
        switch (s->type) {
            case Saturn::Parse::SExpType::List: {
                return interpret(static_cast<Saturn::Parse::List*>(s), environment);
                break;
            }
            case Saturn::Parse::SExpType::Symbol: {
                auto symbol = static_cast<Saturn::Parse::Symbol*>(s);
                if (auto object = environment.lookupObject(symbol)) {
                    return object.value();
                }
                else {
                    auto dotPosition = symbol->value.find('.');
                    if (dotPosition != std::string_view::npos) {
                        auto objectName = symbol->value.substr(0, dotPosition);
                        auto propertyName = symbol->value.substr(dotPosition + 1);

                        if (auto object = environment.lookupObject(objectName)) {
                            return object.value()->getChild(propertyName);
                        }
                        else {
                            return {};
                        }
                    }
                    else {
                        return {};
                    }
                }
            }
            case Saturn::Parse::SExpType::IntLiteral: {
                auto value = static_cast<Saturn::Parse::IntLiteral*>(s);

                return new Integer(value->value);
            }
        }

        return {};
    }
}
