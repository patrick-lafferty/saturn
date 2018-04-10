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

#include "grid.h"
#include <algorithm>
#include "../renderer.h"

using namespace Saturn::Parse;

namespace Apollo::Elements {

    bool parseRowColumn(List* config, std::vector<RowColumnDefinition>& definitions,
        const char* proportional, const char* fixed) {

        if (config->items.size() == 1) {
            return false;
        }

        auto count = static_cast<int>(config->items.size());
        for (int i = 1; i < count; i++) {
            bool failed = true;
            
            if (auto maybeConstructor = getConstructor(config->items[i])) {
                auto& constructor = maybeConstructor.value();

                if (constructor.length != 2) {
                    return false;
                }

                if (auto value = constructor.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                    if (constructor.startsWith(proportional)) {
                        definitions.push_back({Unit::Proportional, value.value()->value});
                        failed = false;
                    }
                    else if (constructor.startsWith(fixed)) {
                        definitions.push_back({Unit::Fixed, value.value()->value});
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

    std::optional<GridConfiguration> parseGrid(SExpression* grid) {
        GridConfiguration config;

        if (grid->type != SExpType::List) {
            return {};
        }

        bool first = true;
        for (auto s : static_cast<List*>(grid)->items) {
            if (first) {
                first = false;

                if (s->type != SExpType::Symbol
                    || static_cast<Symbol*>(s)->value.compare("grid") != 0) {
                    return {};
                }

                continue;
            }

            if (auto constructor = getConstructor(s)) {
                auto& c = constructor.value();

                if (c.startsWith("rows")) {
                    if (!parseRowColumn(c.values, config.rows, "proportional-height", "fixed-height")) {
                        return {};
                    }
                }
                else if (c.startsWith("columns")) {
                    if (!parseRowColumn(c.values, config.columns, "proportional-width", "fixed-width")) {
                        return {};
                    }
                }
                else if (c.startsWith("items")) {
                    config.items = c.values;
                }
                else if (c.startsWith("item-source")) {
                    if (auto value = c.get<List*>(1, SExpType::List)) {
                        config.itemSource = value.value();
                    }
                }
                else if (c.startsWith("item-template")) {
                    if (auto value = c.get<List*>(1, SExpType::List)) {
                        config.itemTemplate = value.value();
                    }
                }
                else if (c.startsWith("meta")) {
                    config.meta = c.values;
                }
                else if (c.startsWith("row-gap")) {
                    if (c.length != 2) {
                        return {};
                    }

                    if (auto value = c.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        config.rowGap = value.value()->value;
                    }
                    else {
                        return {};
                    }
                }
                else if (c.startsWith("column-gap")) {
                    if (c.length != 2) {
                        return {};
                    }

                    if (auto value = c.get<IntLiteral*>(1, SExpType::IntLiteral)) {
                        config.columnGap = value.value()->value;
                    }
                    else {
                        return {};
                    }
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

    Grid::Grid(GridConfiguration config)
        : Container(config), itemSource{this, Bindings::ItemSource} {

        rows = std::move(config.rows);
        columns = std::move(config.columns);
        rowGap = config.rowGap;
        columnGap = config.columnGap;
        itemTemplate = config.itemTemplate;
    }

    void Grid::addChild(UIElement* child) {
        GridElement element;
        element.element = child;

        addChild(element); 
    }

    void Grid::addChild(UIElement* child, const std::vector<MetaData>& meta) {
        GridElement element;
        element.element = child;

        applyMetaData(element, meta);
        addChild(element);
    }

    void Grid::addChild(Container* container) {
        GridElement element;
        element.element = container;

        addChild(element);
    }

    void Grid::addChild(Container* container, const std::vector<MetaData>& meta) {
        GridElement element;
        element.element = container;

        applyMetaData(element, meta);
        addChild(element);
    }

    Bounds getCellBounds(RowColumnDefinition& row, RowColumnDefinition& column) {
        return {column.startingPosition, row.startingPosition,
                column.actualSpace, row.actualSpace};
    }

    Bounds getSpannedCellBounds(std::vector<RowColumnDefinition>& rows, 
        std::vector<RowColumnDefinition>& columns,
        GridElement& element) {
        
        Bounds bounds {columns[element.column].startingPosition, rows[element.row].startingPosition,
                columns[element.column].actualSpace, rows[element.row].actualSpace};

        if (element.rowSpan > 0) {
            auto& row = rows[element.row + element.rowSpan - 1];
            bounds.height = row.startingPosition + row.actualSpace - bounds.y;
        }

        if (element.columnSpan > 0) {
            auto& column = columns[element.column + element.columnSpan - 1];
            bounds.width = column.startingPosition + column.actualSpace - bounds.x;
        }

        return bounds;
    }

    void Grid::addChild(GridElement element) {
        element.bounds = getCellBounds(rows[element.row], columns[element.column]);
        children.push_back(element);

        if (std::holds_alternative<UIElement*>(element.element)) {
            auto child = std::get<UIElement*>(element.element);
            child->setParent(this);
        }
        else if (std::holds_alternative<Container*>(element.element)) {
            auto child = std::get<Container*>(element.element);
            child->setParent(this);
        }
    }

    void Grid::layoutChildren() {

        if (children.empty()) {
            return;
        }

        calculateGridDimensions();

        for (auto& child : children) {
            
            if (child.rowSpan > 0 || child.columnSpan > 0) {
                child.bounds = getSpannedCellBounds(rows, columns, child);
            }
            else {
                child.bounds = getCellBounds(rows[child.row], columns[child.column]);
            }

            if (std::holds_alternative<UIElement*>(child.element)) {
                auto control = std::get<UIElement*>(child.element);
            }
            else {
                auto container = std::get<Container*>(child.element);
                container->layoutChildren();
            }
        }
    }

    void allocateDefinitionSpace(std::vector<RowColumnDefinition>& definitions, int unallocatedSpace, int currentPosition, int gap) {
        int totalProportionalUnits = 0;

        if (gap > 0) {
            unallocatedSpace -= gap * (definitions.size() - 1);
        }

        for (auto& definition : definitions) {
            if (definition.unit == Unit::Fixed) {
                if (unallocatedSpace > 0) {
                    auto space = std::min(unallocatedSpace, definition.desiredSpace);
                    definition.actualSpace = space;
                    unallocatedSpace -= space;
                }
                else {
                    break;
                }
            }
            else {
                totalProportionalUnits += definition.desiredSpace;
            }
        }

        if (totalProportionalUnits > 0 
            && unallocatedSpace > 0) {

            auto proportionalSpace = unallocatedSpace / totalProportionalUnits;
            
            for (auto& definition : definitions) {
                if (definition.unit == Unit::Proportional) {
                    definition.actualSpace = proportionalSpace * definition.desiredSpace;
                }
            }
        }

        for (auto& definition : definitions) {
            definition.startingPosition = currentPosition;
            currentPosition += definition.actualSpace + gap;
        }
    }

    void Grid::calculateGridDimensions() {
        auto bounds = getBounds();

        allocateDefinitionSpace(rows, bounds.height, bounds.y, rowGap);
        allocateDefinitionSpace(columns, bounds.width, bounds.x, columnGap);
    }

    void Grid::applyMetaData(GridElement& element, const std::vector<MetaData>& meta) {

        for (auto& data : meta) {
            if (data.containerNamespace != MetaNamespace::Grid) {
                continue;
            }

            switch (static_cast<GridMetaId>(data.metaId)) {
                case GridMetaId::Row: {
                    element.row = std::max(0, 
                        std::min(data.value, static_cast<int>(rows.size()) - 1));
                    break;
                }
                case GridMetaId::Column: {
                    element.column = std::max(0, 
                        std::min(data.value, static_cast<int>(columns.size()) - 1));
                    break;
                }
                case GridMetaId::RowSpan: {
                    element.rowSpan = data.value;
                    break;
                }
                case GridMetaId::ColumnSpan: {
                    element.columnSpan = data.value;
                    break;
                }
            }
        }

        if (element.rowSpan > 0) {
            auto maxRow = element.row + element.rowSpan;

            if (maxRow > rows.size()) {
                element.rowSpan = rows.size() - element.row;
            }
        }

        if (element.columnSpan > 0) {
            auto maxColumn = element.column + element.columnSpan;

            if (maxColumn > columns.size()) {
                element.columnSpan = columns.size() - element.column;
            }
        }
    }

    Bounds Grid::getChildBounds(const UIElement* child) {
        for (auto& element : children) {
            if (std::holds_alternative<UIElement*>(element.element)) {
                auto base = std::get<UIElement*>(element.element);

                if (base == child) {
                    return element.bounds;
                }
            }
            else if (std::get<Container*>(element.element) == child) {
                return element.bounds;
            }
        }

        return {};
    }

    void Grid::layoutText(Apollo::Text::Renderer* renderer) {
        for (auto& element : children) {
            if (std::holds_alternative<UIElement*>(element.element)) {
                auto child = std::get<UIElement*>(element.element); 
                child->layoutText(renderer);
            }
            else if (std::holds_alternative<Container*>(element.element)) {
                auto child = std::get<Container*>(element.element); 
                child->layoutText(renderer);
            }
        }
    }

    void Grid::render(Renderer* renderer, Bounds bounds, Bounds clip) {
        for (auto& element : children) {
            if (std::holds_alternative<UIElement*>(element.element)) {
                auto child = std::get<UIElement*>(element.element); 
                child->render(renderer, element.bounds, bounds);
            }
            else if (std::holds_alternative<Container*>(element.element)) {
                auto child = std::get<Container*>(element.element); 
                child->render(renderer, element.bounds, bounds);
            }
        }

        if (children.empty()) {
            auto bounds = getBounds();
            //renderer->drawRectangle(getBackgroundColour(), bounds.x, bounds.y, bounds.width, bounds.height);
        }
    }

    void Grid::clearTemplateItems() {
        for (auto& child : children) {
            if (std::holds_alternative<UIElement*>(child.element)) {
                auto element = std::get<UIElement*>(child.element);
                delete element;
            }
            else {
                auto container = std::get<Container*>(child.element);
                delete container;
            }
        }

        children.clear();

        requestRender(this);
    }
}