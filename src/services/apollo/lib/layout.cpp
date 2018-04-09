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

using namespace Saturn::Parse;

namespace Apollo::Elements {

    bool parseGridMeta(Constructor grid, std::vector<MetaData>& meta) {

        auto count = grid.values->items.size();

        if (count < 2) {
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
                else if (constructor.startsWith("row-span")
                    && constructor.length == 2) {
                    if (auto maybeValue = constructor.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        auto value = maybeValue.value();
                        meta.push_back({MetaNamespace::Grid, static_cast<int>(GridMetaId::RowSpan), value->value});
                        failed = false;
                    }
                }
                else if (constructor.startsWith("column-span")
                    && constructor.length == 2) {
                    if (auto maybeValue = constructor.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        auto value = maybeValue.value();
                        meta.push_back({MetaNamespace::Grid, static_cast<int>(GridMetaId::ColumnSpan), value->value});
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

        if (config->items.size() < 2) {
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

    std::optional<std::variant<KnownContainers, KnownElements>>
    getConstructorType(Constructor constructor) {

        if (constructor.startsWith("grid")) {
            return KnownContainers::Grid;
        }
        else if (constructor.startsWith("list-view")) {
            return KnownContainers::ListView;
        }
        else if (constructor.startsWith("label")) {
            return KnownElements::Label;
        }
        else {
            return {};
        }
    }

    std::optional<Container*> finishContainer(Container* container, Container* parent, Saturn::Parse::List* meta) {
        if (meta != nullptr) {
            if (auto metaData = parseMeta(meta)) {
                parent->addChild(container, metaData.value());
            }
            else {
                parent->addChild(container);
            }
        }
        else {
            parent->addChild(container);
        }

        return container;
    }
}
