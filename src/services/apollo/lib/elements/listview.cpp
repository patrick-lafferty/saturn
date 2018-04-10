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

#include "listview.h"

using namespace Saturn::Parse;

namespace Apollo::Elements {

    std::optional<ListViewConfiguration> parseListView(SExpression* list) {
        ListViewConfiguration config;

        if (list->type != SExpType::List) {
            return {};
        }

        bool first = true;
        for (auto s : static_cast<List*>(list)->items) {
            if (first) {
                first = false;

                if (s->type != SExpType::Symbol
                    || static_cast<Symbol*>(s)->value.compare("list-view") != 0) {
                    return {};
                }

                continue;
            }

            if (auto constructor = getConstructor(s)) {
                auto& c = constructor.value();

                if (c.startsWith("items")) {
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

    ListView::ListView(ListViewConfiguration config)
        : Container(config), itemSource{this, Bindings::ItemSource} {

        itemTemplate = config.itemTemplate;
    }

    void ListView::addChild(UIElement* child) {
        ContainedElement element;
        element.element = child;

        addChild(element);
    }

    void ListView::addChild(UIElement* child, const std::vector<MetaData>& meta) {
        ContainedElement element;
        element.element = child;

        //TODO: support listview metadata

        addChild(element);
    }

    void ListView::addChild(Container* container) {
        ContainedElement element;
        element.element = container;

        addChild(element);
    }

    void ListView::addChild(Container* container, const std::vector<MetaData>&  meta) {

        ContainedElement element;
        element.element = container;

        //TODO: support listview metadata

        addChild(element);
    }

    void ListView::layoutChildren() {
        if (children.empty()) {
            return;
        }

        auto bounds = getBounds();
        auto totalHeight = 0;
        auto startY = bounds.y;

        for (auto& child : children) {
            child.bounds = bounds;

            if (std::holds_alternative<UIElement*>(child.element)) {
                auto element = std::get<UIElement*>(child.element);
                auto height = element->getDesiredHeight();
                bounds.y += height;
                child.bounds.height = height;
                totalHeight += height;

            }
        }

        if (totalHeight > bounds.height) {
            auto start = startY + bounds.height;

            for (auto it = rbegin(children); it != rend(children); ++it) {
                auto& child = *it;
                if (std::holds_alternative<UIElement*>(child.element)) {
                    auto element = std::get<UIElement*>(child.element);
                    start -= child.bounds.height;
                    child.bounds.y = start;
                }
            }
        }

    }

    Bounds ListView::getChildBounds(const UIElement* child) {
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

    void ListView::layoutText(Apollo::Text::Renderer* renderer) {
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

        layoutChildren();
    }

    void ListView::render(Renderer* renderer, Bounds bounds, Bounds clip) {
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
    }

    void ListView::addChild(ContainedElement element) {
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
}