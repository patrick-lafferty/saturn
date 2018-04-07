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

#include <optional>
#include <saturn/parsing.h>
#include "elements/grid.h"
#include "elements/label.h"

namespace Apollo::Elements {

    enum class KnownContainers {
        Grid
    };

    enum class KnownElements {
        Label
    };

    bool parseGridMeta(Saturn::Parse::Constructor grid, std::vector<MetaData>& meta);
	std::optional<std::vector<MetaData>> parseMeta(Saturn::Parse::List* config);

    template<class BindFunc>
    std::optional<UIElement*> createElement(Container* parent, 
        KnownElements type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding) {

        switch (type) {
            case KnownElements::Label: {
                if (auto maybeConfig = parseLabel(constructor.values)) {
                    auto config = maybeConfig.value();
                    auto label = Label::create(config, setupBinding);

                    if (config.meta != nullptr) {
                        if (auto meta = parseMeta(config.meta)) {
                            parent->addChild(label, meta.value());
                        }
                        else {
                            parent->addChild(label);
                        }
                    }
                    else {
                        parent->addChild(label);
                    }

                    return label;
                }

                break;
            }
        }

        return {};
    }

    std::optional<std::variant<KnownContainers, KnownElements>>
    getConstructorType(Saturn::Parse::Constructor constructor);

    template<class BindFunc>
    std::optional<Container*> createContainer(Container* parent, 
        KnownContainers type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding);

    template<class BindFunc>
    bool createContainerItems(Container* parent, Saturn::Parse::SExpression* itemList, BindFunc setupBinding) {
        using namespace Saturn::Parse;

        if (itemList->type != SExpType::List) {
            return false;
        }

        if (auto maybeConstructor = getConstructor(itemList)) {
            auto& items = maybeConstructor.value();

            bool first = true;
            for (auto item : items.values->items) {

                if (first) {
                    if (item->type == SExpType::Symbol) {
                        auto symbol = static_cast<Symbol*>(item);

                        if (symbol->value.compare("items") != 0) {
                            return false;
                        }
                    }

                    first = false;
                    continue;
                }

                if (auto c = getConstructor(item)) {
                    auto childConstructor = c.value();

                    if (auto maybeChildType = getConstructorType(childConstructor)) {
                        auto childType = maybeChildType.value();

                        if (std::holds_alternative<KnownContainers>(childType)) {
                            auto child = createContainer(parent, std::get<KnownContainers>(childType), childConstructor, setupBinding);

                            if (!child) {
                                return false;
                            }
                        }
                        else {
                            auto child = createElement(parent, std::get<KnownElements>(childType), childConstructor, setupBinding);

                            if (!child) {
                                return false;
                            }
                        }
                    }
                    else {
                        return false;
                    }
                }  
                else {
                    return false;
                }
            }
        }

        return true;
    }

    template<class BindFunc>
    std::optional<Container*> createContainer(KnownContainers type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding) {

        switch (type) {
            case KnownContainers::Grid: {
                if (auto maybeConfig = parseGrid(constructor.values)) {
                    auto config = maybeConfig.value();
                    auto grid = new Grid(config);
                    auto success = createContainerItems(grid, config.items, setupBinding);                    

                    if (success) {
                        return grid;
                    }
                    else {
                        delete grid;
                    }
                }

                break;
            }
        }

        return {};            
    }

    template<class BindFunc>
    std::optional<Container*> createContainer(Container* parent, 
        KnownContainers type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding) {
        
        auto maybeContainer = createContainer(type, constructor, setupBinding);

        if (maybeContainer) {
            auto container = maybeContainer.value();
            parent->addChild(container);
            return parent;
        }
        else {
            return {};
        }
    }

    template<class BindFunc>
    std::optional<Container*> loadLayout(Saturn::Parse::SExpression* root, 
        Container* window,
        BindFunc setupBinding) {

        if (auto c = getConstructor(root)) {
            auto constructor = c.value();
            
            if (auto maybeType = getConstructorType(constructor)) {
                auto type = maybeType.value();

                if (std::holds_alternative<KnownContainers>(type)) {
                    return createContainer(window, std::get<KnownContainers>(type), constructor, setupBinding);
                }
                else {
                    return {};
                }
            }
        }

        return {};
    }
}