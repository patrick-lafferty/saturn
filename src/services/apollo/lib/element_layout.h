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
#include "elements/label.h"

namespace Apollo::Elements {

    enum class KnownContainers {
        Grid,
        ListView
    };

    enum class KnownElements {
        Label
    };

	std::optional<std::vector<MetaData>> parseMeta(Saturn::Parse::List* config);

    std::optional<std::variant<KnownContainers, KnownElements>>
    getConstructorType(Saturn::Parse::Constructor constructor);

    template<class BindFunc, class CollectionBindFunc>
    std::optional<UIElement*> createElement(Container* parent, 
        KnownElements type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding,
        CollectionBindFunc /*setupCollectionBinding*/) {

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

    template<class BindFunc, class CollectionBindFunc>
    std::optional<UIElement*> createElement(Container* parent, 
        KnownElements type, 
        Saturn::Parse::Constructor constructor, 
        BindFunc setupBinding,
        CollectionBindFunc setupCollectionBinding,
        std::optional<Control*>& initialFocus) {

        auto element = createElement(parent, type, constructor, setupBinding, setupCollectionBinding);

        if (element && !initialFocus) {
            switch (type) {
                case KnownElements::Label: {
                    break;
                }
            }
        }

        return element;
    }
}
