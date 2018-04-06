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
                }
                else if (c.startsWith("background")) {
                    if (c.values->items.size() != 2) {
                        return {};
                    }

                    if (auto maybeColour = c.get<List*>(1, SExpType::List)) {
                        if (auto maybeColourConstructor = getConstructor(maybeColour.value())) {
                            auto& colourConstructor = maybeColourConstructor.value();

                            if (colourConstructor.startsWith("rgb")) {
                                if (colourConstructor.values->items.size() != 4) {
                                    return {};
                                }

                                auto r = colourConstructor.get<IntLiteral*>(1, SExpType::IntLiteral);
                                auto g = colourConstructor.get<IntLiteral*>(2, SExpType::IntLiteral);
                                auto b = colourConstructor.get<IntLiteral*>(3, SExpType::IntLiteral);

                                if (r && g && b) {
                                    config.backgroundColour = 
                                        0xFF'00'00'00
                                        | ((r.value()->value & 0xFF) << 16)
                                        | ((g.value()->value & 0xFF) << 8)
                                        | ((b.value()->value & 0xFF));
                                }
                            }
                        }
                    }
                }
                else if (c.startsWith("meta")) {
                    config.meta = c.values;
                }
            }
            else {
                return {};
            }
        }

        return config;
    }

    Label::Label(LabelConfiguration config) {
        backgroundColour = config.backgroundColour;

        if (!config.caption->value.empty()) {
            auto captionLength = config.caption->value.length();
            caption = new char[captionLength + 1];
            memset(caption, '\0', captionLength + 1);
            config.caption->value.copy(caption, captionLength);
        }
    }

    void Label::layoutText(Apollo::Text::Renderer* renderer) {
        if (caption == nullptr) {
            return;
        }

        auto bounds = getBounds();
        captionLayout = renderer->layoutText(caption, bounds.width);
    } 

    void Label::render(Renderer* renderer) {
        auto bounds = getBounds();

        renderer->drawRectangle(backgroundColour, bounds.x, bounds.y, bounds.width, bounds.height);

        if (caption != nullptr) {
            renderer->drawText(captionLayout, bounds.x, bounds.y, backgroundColour);
        }
    }
    
}