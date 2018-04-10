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

#include "element.h"
#include <optional>
#include <variant>
#include "../text.h"
#include "../databinding.h"
#include <saturn/parsing.h>

namespace Apollo::Elements {

    struct LabelConfiguration : Configuration {
        std::variant<Saturn::Parse::StringLiteral*, Saturn::Parse::List*> caption;
    };

    std::optional<LabelConfiguration> parseLabel(Saturn::Parse::SExpression* label);

    /*
    A Label is a UI Element that displays a block of text. The text can be changed 
    at runtime with code, but isn't user interactive like a textbox is.
    */
    class Label : public UIElement {
    public:

        enum class Bindings {
            Caption
        };

        Label(LabelConfiguration config);

        template<class BindFunc>
        static Label* create(LabelConfiguration config, BindFunc setupBinding) {
            using namespace Saturn::Parse;

            auto label = new Label(config);
            label->setupBindings(config, setupBinding);

            if (std::holds_alternative<List*>(config.caption)) {
                auto binding = std::get<List*>(config.caption);

                if (auto maybeConstructor = getConstructor(binding)) {
                    auto& constructor = maybeConstructor.value();

                    if (constructor.startsWith("bind")) {
                        if (constructor.length == 2) {
                            if (auto maybeTarget = constructor.get<Symbol*>(1, SExpType::Symbol)) {
                                Bindable<Label, Bindings, char*> caption{label, Bindings::Caption}; 
                                label->caption = caption;

                                auto& cap = std::get<Bindable<Label, Bindings, char*>>(label->caption);
                                setupBinding(&cap, maybeTarget.value()->value); 
                            }
                        }
                    }
                }
            }

            return label;
        }

        virtual void layoutText(Apollo::Text::Renderer* renderer) override;
        virtual void render(Renderer* renderer, Bounds bounds, Bounds clip) override;

        void onChange(Bindings binding);

    private:

        Apollo::Text::TextLayout captionLayout;        
        std::variant<char*, Bindable<Label, Bindings, char*>> caption;
    };
}