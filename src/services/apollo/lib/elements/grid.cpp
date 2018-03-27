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

namespace Apollo::Elements {

    Grid::Grid() {

        calculateGridDimensions();
    }

    void Grid::addChild(Control* control) {
        GridElement element;
        element.element = control;

        addChild(element); 
    }

    void Grid::addChild(Control* control, const std::vector<MetaData>& meta) {
        GridElement element;
        element.element = control;

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

    void Grid::addChild(GridElement element) {
        children.push_back(element);
        layoutChildren();
    }

    void Grid::layoutChildren() {

        if (children.empty()) {
            return;
        }

        auto bounds = getBounds();

        for (auto& child : children) {
            if (std::holds_alternative<Control*>(child.element)) {
                auto control = std::get<Control*>(child.element);
                child.bounds = bounds;
            }
            else {
                auto container = std::get<Container*>(child.element);
            }
        }
    }

    void allocateDefinitionSpace(std::vector<RowColumnDefinition>& definitions, int unallocatedSpace) {
        int totalProportionalUnits = 0;

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
    }

    void Grid::calculateGridDimensions() {
        auto bounds = getBounds();

        allocateDefinitionSpace(rows, bounds.height);
        allocateDefinitionSpace(columns, bounds.width);
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
            }
        }
    }
}