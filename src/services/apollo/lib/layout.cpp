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
#include "elements/label.h"

using namespace Saturn::Parse;

namespace Apollo::Elements {

    bool parseGridMeta(Constructor grid, std::vector<MetaData>& meta) {
        auto count = grid.values->items.size();

        if (count <= 2) {
            return false;
        }

        for (auto i = 1u; i < count; i++) {
            bool failed = true;

            if (auto maybeConstructor = getConstructor(grid.values->items[i])) {
                auto& constructor = maybeConstructor.value();

                if (constructor.startsWith("row") 
                    && constructor.length == 2) {
                    if (auto maybeValue = constructor.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        auto value = maybeValue.value();
                        meta.push_back({MetaNamespace::Grid, static_cast<int>(GridMetaId::Row), value->value});
                        failed = false;
                    }
                }
                else if (constructor.startsWith("column")
                    && constructor.length == 2) {
                    if (auto maybeValue = constructor.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        auto value = maybeValue.value();
                        meta.push_back({MetaNamespace::Grid, static_cast<int>(GridMetaId::Column), value->value});
                        failed = false;
                    }
                }
            }

            if (failed) {
                return false;
            }
        }

        return true;
    }

	std::optional<std::vector<MetaData>> parseMeta(List* config) {

        if (config->items.size() <= 2) {
            return {};
        }

        std::vector<MetaData> meta;

        for (auto i = 1u; i < config->items.size(); i++) {
            if (auto maybeConstructor = getConstructor(config->items[i])) {
                auto& constructor = maybeConstructor.value();

                if (constructor.startsWith("grid")) {
                    auto success = parseGridMeta(constructor, meta);

                    if (!success) {
                        return {};
                    }
                }
            }
        }

        return meta;
    }

    std::optional<UIElement*> createElement(Container* parent, KnownElements type, Constructor constructor) {
        switch (type) {
            case KnownElements::Label: {
                if (auto maybeConfig = parseLabel(constructor.values)) {
                    auto config = maybeConfig.value();
                    auto label = new Label(config);

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

    std::optional<Container*> createContainer(Container* parent, KnownContainers type, Constructor constructor);

    bool createContainerItems(Container* parent, SExpression* itemList) {

        if (itemList->type != SExpType::List) {
            return false;
        }

        std::vector<SExpression*>& items = static_cast<List*>(itemList)->items;

        for (auto item : items) {
            if (auto c = getConstructor(item)) {
                auto childConstructor = c.value();

                if (auto maybeChildType = getConstructorType(childConstructor)) {
                    auto childType = maybeChildType.value();

                    if (std::holds_alternative<KnownContainers>(childType)) {
                        auto child = createContainer(parent, std::get<KnownContainers>(childType), childConstructor);

                        if (!child) {
                            return false;
                        }
                    }
                    else {
                        auto child = createElement(parent, std::get<KnownElements>(childType), childConstructor);

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

        return true;
    }

    std::optional<Container*> createContainer(KnownContainers type, Constructor constructor) {
        switch (type) {
            case KnownContainers::Grid: {
                if (auto maybeConfig = parseGrid(constructor.values)) {
                    auto config = maybeConfig.value();
                    auto grid = new Grid(config);
                    auto success = createContainerItems(grid, config.items);                    

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

    std::optional<Container*> createContainer(Container* parent, KnownContainers type, Constructor constructor) {
        auto maybeContainer = createContainer(type, constructor);

        if (maybeContainer) {
            auto container = maybeContainer.value();
            parent->addChild(container);
            return parent;
        }
        else {
            return {};
        }
    }

    std::optional<Container*> loadLayout(SExpression* root, Container* window) {

        if (auto c = getConstructor(root)) {
            auto constructor = c.value();
            
            if (auto maybeType = getConstructorType(constructor)) {
                auto type = maybeType.value();

                if (std::holds_alternative<KnownContainers>(type)) {
                    return createContainer(window, std::get<KnownContainers>(type), constructor);
                }
                else {
                    return {};
                }
            }
        }

        return {};
    }
}
