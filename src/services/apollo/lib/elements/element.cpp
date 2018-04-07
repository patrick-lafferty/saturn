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

#include "element.h"
#include "container.h"
#include <saturn/parsing.h>

using namespace Saturn::Parse;

namespace Apollo::Elements {

    Bounds UIElement::getBounds() const {
        return parent->getChildBounds(this);
    }

    void UIElement::setParent(Container* parent) {
        this->parent = parent;
    }

    bool parseElement(Saturn::Parse::SExpression* element, Configuration& config) {
        if (auto maybeConstructor = getConstructor(element)) {
            auto& constructor = maybeConstructor.value();

            if (constructor.startsWith("background")) {
                if (constructor.values->items.size() != 2) {
                    return false;
                }

                if (auto maybeColour = constructor.get<List*>(1, SExpType::List)) {
                    if (auto maybeColourConstructor = getConstructor(maybeColour.value())) {
                        auto& colourConstructor = maybeColourConstructor.value();

                        if (colourConstructor.startsWith("rgb")) {
                            if (colourConstructor.values->items.size() != 4) {
                                return false;
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
            else if (constructor.startsWith("margin")) {

            }
        }

        return true;
    }
}