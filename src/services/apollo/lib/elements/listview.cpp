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
#include "../renderer.h"
#include <services/mouse/messages.h>

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

    void ListView::addChild(UIElement* child, const std::vector<MetaData>& /*meta*/) {
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

    void ListView::addChild(Container* container, const std::vector<MetaData>&  /*meta*/) {

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
        totalHeight = 0;

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
           
            for (auto& child : children) {
                if (std::holds_alternative<UIElement*>(child.element)) {
                    auto scrollbarHeight = getScrollbarSize(bounds.height);
                    auto delta = 0.1f * scrollbarHeight;
                    child.bounds.y -= currentPosition * delta;
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

        if (totalHeight > bounds.height) {

            auto scrollbarBounds = bounds;
            scrollbarBounds.x = scrollbarBounds.x + scrollbarBounds.width - 10;
            scrollbarBounds.width = 10;

            renderer->drawRectangle(0xFF'FF'00'00, scrollbarBounds, clip);

            auto barSize = getScrollbarSize(bounds.height);
            scrollbarBounds.height = barSize;
            auto delta = 0.1f * barSize;
            scrollbarBounds.y += currentPosition * delta;

            renderer->drawRectangle(0xFF'00'FF'00, scrollbarBounds, clip);
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

    int ListView::getScrollbarSize(int height) {

        int barSize = ((float)height / totalHeight) * height;
        return barSize;
    }

    void ListView::clearTemplateItems() {
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

    void ListView::onWindowReady() {

        receivedWindowReady = true;

        for (auto& child : children) {
            UIElement* element {nullptr};

            if (std::holds_alternative<UIElement*>(child.element)) {
                element = std::get<UIElement*>(child.element);
            }
            else {
                element = std::get<Container*>(child.element);
            }

            element->onWindowReady();
        }
    }

    void ListView::handleMouseScroll(const Mouse::Scroll& message) {
        
        if (message.direction == Mouse::ScrollDirection::Vertical) {
            auto bounds = getBounds();
            auto scrollbarHeight = getScrollbarSize(bounds.height);
            auto delta = scrollbarHeight * 0.1f;
            auto maxTicks = ((float)bounds.height - scrollbarHeight) / delta;

            if (message.magnitude == Mouse::ScrollMagnitude::UpBy1) {
                currentPosition = std::min((int)maxTicks, currentPosition + 1);
            }
            else {
                currentPosition = std::max(0, currentPosition - 1);
            }
        }

        layoutChildren();
        requestRender(this);
    }
}