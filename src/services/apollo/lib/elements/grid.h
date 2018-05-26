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

#include "container.h"
#include <optional>
#include "../databinding.h"
#include <saturn/parsing.h>
#include "../element_layout.h"

namespace Apollo::Elements {

    enum class GridMetaId {
        Row,
        Column,
        RowSpan,
        ColumnSpan
    };

    struct GridElement : ContainedElement {
        int row {0}, column {0};
        int rowSpan {0}, columnSpan {0};
    };

    enum class Unit {
        Proportional,
        Fixed
    };

    struct RowColumnDefinition {
        Unit unit;
        int desiredSpace {0};
        int actualSpace {0};
        int startingPosition {0};
    };

    struct GridConfiguration : Configuration {
        std::vector<RowColumnDefinition> rows;
        std::vector<RowColumnDefinition> columns;
        Saturn::Parse::SExpression* items {nullptr};
        int rowGap {0};
        int columnGap {0};
        Saturn::Parse::List* itemSource {nullptr};
        Saturn::Parse::List* itemTemplate {nullptr};
    };

    std::optional<GridConfiguration> parseGrid(Saturn::Parse::SExpression* grid);

    /*
    A Grid is a container that arranges multiple child
    elements in rows and columns.
    */
    class Grid : public Container {
    public:

        enum class Bindings {
            ItemSource
        };

        Grid(GridConfiguration config);

        template<class BindFunc, class CollectionBindFunc>
        static Grid* create(GridConfiguration config, BindFunc /*setupBinding*/, CollectionBindFunc setupCollectionBinding) {
            using namespace Saturn::Parse;

            auto grid = new Grid(config);

            if (config.itemSource != nullptr) {
                if (auto maybeConstructor = getConstructor(config.itemSource)) {
                    auto& constructor = maybeConstructor.value();

                    if (constructor.startsWith("bind")) {
                        if (constructor.length == 2) {
                            if (auto maybeTarget = constructor.get<Symbol*>(1, SExpType::Symbol)) {
                                setupCollectionBinding(&grid->itemSource, maybeTarget.value()->value);
                            }
                        }
                    }
                }
            }

            return grid;
        }

		virtual void addChild(UIElement* element) override;
		virtual void addChild(UIElement* element, const std::vector<MetaData>& meta) override;
		virtual void addChild(Container* container) override;
		virtual void addChild(Container* container, const std::vector<MetaData>&  meta) override;

		virtual void layoutChildren() override;

		virtual Bounds getChildBounds(const UIElement* child) override;

        virtual void layoutText(Apollo::Text::Renderer* renderer) override;
        virtual void render(Renderer* renderer, Bounds bounds, Bounds clip) override;

        template<class Item, class BindFunc>
        void instantiateItemTemplate(Item /*item*/, BindFunc binder) {
            using namespace Saturn::Parse;

            if (auto maybeConstructor = getConstructor(itemTemplate)) {
                auto& constructor = maybeConstructor.value();

                if (auto maybeChildType = getConstructorType(constructor)) {
                    auto childType = maybeChildType.value();

                    if (std::holds_alternative<KnownElements>(childType)) {
                        auto child = createElement(this, 
                            std::get<KnownElements>(childType), 
                            constructor,
                            binder,
                            [](auto, auto) {});

                        if (child) {
                            auto& element = children.back();
                            auto numberOfChildren = children.size() - 1;
                            element.row = numberOfChildren / columns.size();
                            element.column = numberOfChildren % columns.size();
                        }
                    }
                }
            }
        }

        template<class Item, class BindFunc>
        void onItemAdded(Item item, Bindings binding, BindFunc binder) {
            switch (binding) {
                case Bindings::ItemSource: {
                    instantiateItemTemplate(item, binder);
                    break;
                }
            }
        }

        void clearTemplateItems();

        virtual void onWindowReady() override;

    private:
        
        void calculateGridDimensions();
        void addChild(GridElement element);
        void applyMetaData(GridElement& element, const std::vector<MetaData>& meta);

        std::vector<GridElement> children;
        std::vector<RowColumnDefinition> rows;
        std::vector<RowColumnDefinition> columns;
        int rowGap;
        int columnGap;

        BindableCollection<Grid, Bindings> itemSource;
        Saturn::Parse::List* itemTemplate;
    };
}