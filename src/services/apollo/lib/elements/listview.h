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
#include <saturn/parsing.h>

namespace Apollo::Elements {

    struct ListViewConfiguration : Configuration {
        Saturn::Parse::SExpression* items {nullptr};
    };

    std::optional<ListViewConfiguration> parseListView(Saturn::Parse::SExpression* list);

    class ListView : public Container {
    public:

        enum class Bindings {

        };

        ListView(ListViewConfiguration config);

        template<class BindFunc, class CollectionBindFunc>
        static ListView* create(ListViewConfiguration config, BindFunc setupBinding, CollectionBindFunc setupCollectionBinding) {
            using namespace Saturn::Parse;

            auto list = new ListView(config);

            return list;
        }

        virtual void addChild(UIElement* element) override;
		virtual void addChild(UIElement* element, const std::vector<MetaData>& meta) override;
		virtual void addChild(Container* container) override;
		virtual void addChild(Container* container, const std::vector<MetaData>&  meta) override;

		virtual void layoutChildren() override;

		virtual Bounds getChildBounds(const UIElement* child) override;

        virtual void layoutText(Apollo::Text::Renderer* renderer) override;
        virtual void render(Renderer* renderer) override;

    private:

        void addChild(ContainedElement element);

        std::vector<ContainedElement> children;
    };
}