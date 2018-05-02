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
#include "container.h"
#include "messages.h"
#include "lib/window.h"
#include <services.h>
#include <system_calls.h>
#include <algorithm>

using namespace Kernel;

namespace Apollo {

    void Container::addChild(Tile tile, Size size, bool focusable) {
        tile.parent = this;
        children.push_back({size, focusable, tile});
        activeTaskId = tile.handle.taskId;
        layoutChildren();
    }

    void Container::addChild(Container* container, Size size) {
        children.push_back({size, false, container});
        container->parent = this;
        layoutChildren();
    }

    void placeChild(Split split, ContainerChild& child, Bounds& childBounds, int gapSize) {
        
        if (split == Split::Vertical) {
            childBounds.width = child.size.actualSpace;
        }
        else {
            childBounds.height = child.size.actualSpace;
        }

        if (std::holds_alternative<Tile>(child.child)) {
            auto& tile = std::get<Tile>(child.child);
            tile.bounds = childBounds;
            Resize resize;
            resize.width = childBounds.width;
            resize.height = childBounds.height;
            resize.recipientId = tile.handle.taskId;
            send(IPC::RecipientType::TaskId, &resize);
        }
        else {
            auto container = std::get<Container*>(child.child);
            container->bounds = childBounds;
            container->layoutChildren();
        }

        switch (split) {
            case Split::Horizontal: {
                childBounds.y += childBounds.height;
                childBounds.y += gapSize;
                break;
            }
            case Split::Vertical: {
                childBounds.x += childBounds.width;
                childBounds.x += gapSize;
                break;
            }
        }
    }

    void Container::layoutChildren() {
        if (children.empty()) {
            return;
        }

        int unallocatedSpace {0};
        Bounds childBounds {bounds.x, bounds.y};

        if (split == Split::Vertical) {
            unallocatedSpace = bounds.width;
            childBounds.height = bounds.height;
        }
        else {
            unallocatedSpace = bounds.height;
            childBounds.width = bounds.width;
        }

        const auto gapSize = 5;
        unallocatedSpace -= gapSize * (children.size() - 1);

        int totalProportionalUnits = 0;

        for (auto& child : children) {
            if (child.size.unit == Unit::Fixed) {
                if (unallocatedSpace > 0) {
                    auto space = std::min(unallocatedSpace, child.size.desiredSpace);
                    child.size.actualSpace = space;
                    unallocatedSpace -= space;
                    placeChild(split, child, childBounds, gapSize);
                }
                else {
                    break;
                }
            }
            else {
                totalProportionalUnits += child.size.desiredSpace;
            }
        }

        if (totalProportionalUnits > 0 
            && unallocatedSpace > 0) {

            auto proportionalSpace = unallocatedSpace / totalProportionalUnits;
            
            for (auto& child : children) {
                if (child.size.unit == Unit::Proportional) {
                    child.size.actualSpace = proportionalSpace * child.size.desiredSpace;
                    placeChild(split, child, childBounds, gapSize);
                }
            }
        }
    }

    uint32_t Container::getChildrenCount() {
        return children.size();
    }

    std::optional<Tile*> Container::findTile(uint32_t taskId) {
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child);

                if (tile.handle.taskId == taskId) {
                    return &tile;
                }
            }
            else {
                auto container = std::get<Container*>(child.child);

                if (auto tile = container->findTile(taskId)) {
                    return tile;
                }
            }
        }

        return {};
    }

    void Container::composite(uint32_t volatile* frameBuffer, uint32_t displayWidth) {
        for (auto& child : children) {
           if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 
                renderTile(frameBuffer, tile, displayWidth, {0, 0, tile.bounds.width, tile.bounds.height});
           }
           else {
                auto container = std::get<Container*>(child.child); 
                container->composite(frameBuffer, displayWidth);
           }
        }
    }

    void Container::dispatchRenderMessages() {
       for (auto& child : children) {
           if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 

                if (tile.canRender) {
                    tile.canRender = false;
                    Render render;
                    render.recipientId = tile.handle.taskId;
                    send(IPC::RecipientType::TaskId, &render);
                }
           }
           else {
                auto container = std::get<Container*>(child.child); 
                container->dispatchRenderMessages();
           }
        } 
    }

    bool Container::focusPreviousTile() {
        auto previous {activeTaskId};

        bool isFirst {true};
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 

                if (!child.focusable) {
                    continue;
                }

                if (tile.handle.taskId == activeTaskId) {
                    if (isFirst) {
                        return false;
                    }
                    else {
                        break;
                    }
                }
                else {
                    previous = tile.handle.taskId;
                }

                if (isFirst) {
                    isFirst = false;
                }
            }
        }

        activeTaskId = previous;
        return true;
    }

    bool Container::focusNextTile() {
        auto next {activeTaskId};

        for (auto it = rbegin(children); it != rend(children); ++it) {
            if (std::holds_alternative<Tile>(it->child)) {
                auto& tile = std::get<Tile>(it->child); 

                if (!it->focusable) {
                    continue;
                }

                if (tile.handle.taskId == activeTaskId) {
                    break;
                }
                else {
                    next = tile.handle.taskId;
                }
            }
        }

        activeTaskId = next;
    }
     
}
    