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
#include <string>
#include <variant>

namespace Saturn::Gemini {

    enum class Type {
        String,
        Bool,
        Object,
        Any,
        Void
    };

    struct FunctionSignature {
        std::vector<Type> input;
        Type output;
    };

    struct ArgumentMismatch {

    };

    enum class GeminiType {
        Object,
        Function,
        List,
        Integer,
        Float,
        Boolean,
        String
    };

    class Object {
    public:

        Object() {
            type = GeminiType::Object;
        }

        Object(Saturn::Parse::List* list);

        virtual ~Object() {}

        GeminiType type;

        std::map<std::size_t, class Object*> children;
        void addChild(std::string_view name, Object* value) {
            auto hash = std::hash<std::string_view>{}(name);
            children[hash] = value;
        }

        Object* getChild(std::string_view name) {
            auto hash = std::hash<std::string_view>{}(name);
            auto it = children.find(hash);

            if (it != end(children)) {
                return it->second;
            }
            else {
                return nullptr;
            }
        }
    };

    class Integer : public Object {
    public:

        Integer(int value) {
            type = GeminiType::Integer; 
            this->value = value;
        }

        int value {0};
    };

    class Float : public Object {
    public:
        
        Float(float value) {
            type = GeminiType::Float;
            this->value = value;
        }

        float value {0.f};
    };

    class Boolean : public Object {
    public:

        Boolean(bool value) {
            type = GeminiType::Boolean;
            this->value = value;
        }

        bool value {false};
    };

    class String : public Object {
    public:

        String(std::string_view value) {
            type = GeminiType::String;
            this->value = value;
        }

        std::string value;
    };

    struct Result {
        std::variant<char*, Object*> value;
    };

    typedef std::variant<ArgumentMismatch, Object*> CallResult;

    class List : public Object {
    public:

        List() {
            type = GeminiType::List;
        }

        std::vector<Object*> items;
    };

    class Function : public Object {
    public:

        Function() {
            type = GeminiType::Function;
        }

        Function(FunctionSignature signature, std::function<CallResult(List*)> func);
        CallResult call(List* arguments);

    private:

        FunctionSignature signature;
        std::function<CallResult(List*)> f;
    };

    class Environment {
    public:

        std::optional<Function*> lookupFunction(Saturn::Parse::Symbol* symbol);
        std::optional<Object*> lookupObject(Saturn::Parse::Symbol* symbol);
        std::optional<Object*> lookupObject(std::string_view name);

        void addFunction(std::string_view name, Function* function);
        void addObject(std::string_view name, Object* object);

        void mapParameter(Saturn::Parse::Symbol* parameter, Object* argument) {
            addObject(parameter->value, argument);
        }

        Environment makeChild() {
            Environment child;
            child.parent = this;

            return child;
        }

    private:

        Environment* parent {nullptr};
        std::map<std::size_t, Function*> functions;
        std::map<std::size_t, Object*> objects;
    };

    template<class Func>
    List* map(Saturn::Parse::List* list, Func func, Environment& /*environment*/, bool skipHead = true) {
        auto result = new List();


        for (auto item : list->items) {
            if (skipHead) {
                skipHead = false;
                continue;
            }

            result->items.push_back(func(item));
        }

        return result;
    }

}