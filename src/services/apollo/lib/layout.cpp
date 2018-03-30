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
#include "layout.h"
#include <saturn/parsing.h>
#include "elements/grid.h"

namespace Apollo::Elements {

    UIElement* createElement(Container* parent, KnownElements type, Constructor constructor) {
        switch (type) {
            case KnownElements::Label: {
                break;
            }
        }

        return nullptr;
    }

    std::optional<std::variant<KnownContainers, KnownElements>>
    getConstructorType(Constructor constructor) {

        if (constructor.startsWith("grid")) {
            return KnownContainers::Grid;
        }
        else if (constructor.startsWith("label")) {
            return KnownElements::Label;
        }
        else {
            return {};
        }
    }

    Container* createContainerItems(Container* parent, std::vector<SExpression*>& items) {
        for (auto item : items) {
            if (auto c = getConstructor(item)) {
                auto childConstructor = c.value();

                if (auto maybeChildType = getConstructorType(childConstructor)) {
                    auto childType = maybeChildType.value();

                    if (std::holds_variant<KnownContainers>(childType) {
                        auto child = createContainer(parent, std::get<KnownContainers>(childType), childConstructor);

                        if (child == nullptr) {
                            //TODO: leak, see below
                            return nullptr;
                        }
                    }
                    else {
                        auto child = createElement(parent, std::get<KnownElements>(childType), childConstructor);

                        if (child == nullptr) {
                            //TODO: leak, see below
                            return nullptr;
                        }
                    }
                }
            }  
            else {
                //TODO: memory leak - grid and possible children
                return nullptr;
            }
        }

        return parent;
    }

    Container* createContainer(KnownContainers type, Constructor constructor) {
        switch (type) {
            case KnownContainers::Grid: {
                if (auto config = parseGrid(constructor.values)) {
                    auto grid = new Grid(config.value());
                    return createContainerItems(grid, config.items);                    
                }

                break;
            }
        }

        return nullptr;            
    }

    Container* createContainer(Container* parent, KnownContainers type, Constructor constructor) {
        auto container = createContainer(type, constructor);

        if (constainer != nullptr) {
            parent->addChild(container);
            return parent;
        }
        else {
            return nullptr;
        }
    }

    Container* loadLayout(SExpression* root) {
        Layout layout;

        if (auto c = getConstructor(root)) {
            auto constructor = c.value();
            
            if (auto maybeType = getConstructorType(constructor)) {
                auto type = maybeType.value();

                if (std::holds_variant<KnownContainers>(type)) {
                    return createContainer(std::get<KnownContainers>(type), constructor);
                }
                else {
                    return nullptr;
                }
            }
        }

        return nullptr;
    }
}
