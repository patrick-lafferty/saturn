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

#include "label.h"
#include <algorithm>
#include <saturn/parsing.h>
#include "../renderer.h"

using namespace Saturn::Parse;

namespace Apollo::Elements {

    std::optional<LabelConfiguration> parseLabel(SExpression* label) {
        LabelConfiguration config;

        if (label->type != SExpType::List) {
            return {};
        }

        bool first = true;
        for (auto s : static_cast<List*>(label)->items) {
            if (first) {
                first = false;

                if (s->type != SExpType::Symbol
                    || static_cast<Symbol*>(s)->value.compare("label") != 0) {
                    return {};
                }

                continue;
            }

            if (auto constructor = getConstructor(s)) {
                auto& c = constructor.value();

                if (c.startsWith("caption")) {
                    if (c.values->items.size() != 2) {
                        return {};
                    }

                    if (auto value = c.get<StringLiteral*>(1, SExpType::StringLiteral)) {
                        config.caption = value.value();
                    }
                    else if (auto value = c.get<List*>(1, SExpType::List)) {
                        config.caption = value.value();
                    }
                }
                else if (c.startsWith("meta")) {
                    config.meta = c.values;
                }
                else if (!parseElement(s, config)) {
                    return {};
                }
            }
            else {
                return {};
            }
        }

        return config;
    }

    Label::Label(LabelConfiguration config)
        : UIElement(config) {

        if (std::holds_alternative<StringLiteral*>(config.caption)) {
            auto value = std::get<StringLiteral*>(config.caption);

            if (!value->value.empty()) {
                auto captionLength = value->value.length();
                auto text = new char[captionLength + 1];
                memset(text, '\0', captionLength + 1);
                value->value.copy(text, captionLength);
                caption = text;
            }
        }
    }

    void Label::layoutText(Apollo::Text::Renderer* renderer) {
        if (std::holds_alternative<char*>(caption)
            && std::get<char*>(caption) == nullptr) {
            return;
        }

        char* captionText {nullptr};

        if (std::holds_alternative<char*>(caption)) {
            captionText = std::get<char*>(caption);
        }
        else if (std::holds_alternative<Bindable<Label, Bindings, char*>>(caption)) {
            auto& binding = std::get<Bindable<Label, Bindings, char*>>(caption);
            captionText = binding.getValue();
        }

        if (captionText == nullptr) {
            return;
        }

        auto bounds = getBounds();
        captionLayout = renderer->layoutText(captionText, bounds.width, fontColour);
    } 

    void Label::render(Renderer* renderer) {
        auto bounds = getBounds();

        auto backgroundColour = getBackgroundColour();

        renderer->drawRectangle(backgroundColour, bounds.x, bounds.y, bounds.width, bounds.height);

        char* captionText {nullptr};

        if (std::holds_alternative<char*>(caption)) {
            captionText = std::get<char*>(caption);
        }
        else if (std::holds_alternative<Bindable<Label, Bindings, char*>>(caption)) {
            auto& binding = std::get<Bindable<Label, Bindings, char*>>(caption);
            captionText = binding.getValue();
        }

        if (captionText == nullptr) {
            return;
        }

        bounds.x = adjustForAlignment(bounds.x, horizontalAlignment, bounds.width, captionLayout.bounds.width);        
        bounds.y = adjustForAlignment(bounds.y, verticalAlignment, bounds.height, captionLayout.bounds.height);

        renderer->drawText(captionLayout, bounds.x + padding.horizontal, bounds.y + padding.vertical, backgroundColour);
    }

    void Label::onChange(Bindings binding) {
        switch (binding) {
            case Bindings::Caption: {
                requestLayoutText();
                requestRender();
                break;
            }
        }
    }
}