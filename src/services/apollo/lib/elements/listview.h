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

    struct ListViewConfiguration : Configuration {
        Saturn::Parse::SExpression* items {nullptr};
        Saturn::Parse::List* itemSource {nullptr};
        Saturn::Parse::List* itemTemplate {nullptr};
    };

    /*
    Tries to create a ListViewConfiguration from the given s-expression,
    and then a ListView can be created using that config
    */
    std::optional<ListViewConfiguration> parseListView(Saturn::Parse::SExpression* list);

    /*
    A ListView is a container that arranges its children linearly
    in either a row or a column. 
    */
    class ListView : public Container {
    public:

        enum class Bindings {
            ItemSource
        };

        ListView(ListViewConfiguration config);

        /*
        A helper function to create ListViews that use databinding.
        The *BindFuncs use templates, constructors for non-template classes
        can't be templated, hence the need for a static function
        */
        template<class BindFunc, class CollectionBindFunc>
        static ListView* create(ListViewConfiguration config, BindFunc setupBinding, CollectionBindFunc setupCollectionBinding) {
            using namespace Saturn::Parse;

            auto list = new ListView(config);

            if (config.itemSource != nullptr) {
                if (auto maybeConstructor = getConstructor(config.itemSource)) {
                    auto& constructor = maybeConstructor.value();

                    if (constructor.startsWith("bind")) {
                        if (constructor.length == 2) {
                            if (auto maybeTarget = constructor.get<Symbol*>(1, SExpType::Symbol)) {
                                setupCollectionBinding(&list->itemSource, maybeTarget.value()->value);
                            }
                        }
                    }
                }
            }

            return list;
        }

        virtual void addChild(UIElement* element) override;
		virtual void addChild(UIElement* element, const std::vector<MetaData>& meta) override;
		virtual void addChild(Container* container) override;
		virtual void addChild(Container* container, const std::vector<MetaData>&  meta) override;

		virtual void layoutChildren() override;

		virtual Bounds getChildBounds(const UIElement* child) override;

        virtual void layoutText(Apollo::Text::Renderer* renderer) override;
        virtual void render(Renderer* renderer, Bounds bounds, Bounds clip) override;

        /*
        Creates a child element for some user-defined object by using the
        item-template supplied in the layout file
        */
        template<class Item, class BindFunc>
        void instantiateItemTemplate(Item item, BindFunc binder) {
            using namespace Saturn::Parse;

            if (auto maybeConstructor = getConstructor(itemTemplate)) {
                auto& constructor = maybeConstructor.value();

                if (auto maybeChildType = getConstructorType(constructor)) {
                    auto childType = maybeChildType.value();

                    if (std::holds_alternative<KnownElements>(childType)) {
                        createElement(this, 
                            std::get<KnownElements>(childType), 
                            constructor,
                            binder,
                            [](auto, auto) {});
                    }
                }
            }
        }

        /*
        Handles items being added to the ListView from an ObservableCollection.
        See ObservableCollection::add, apollo\lib\databinding.h
        */
        template<class Item, class BindFunc>
        void onItemAdded(Item item, Bindings binding, BindFunc binder) {
            switch (binding) {
                case Bindings::ItemSource: {
                    instantiateItemTemplate(item, binder);
                    break;
                }
            }
        }

        /*
        Deletes all of this container's children.

        TODO: this should probably just be clear(), and be in Container
        */
        void clearTemplateItems();

    private:

        void addChild(ContainedElement element);

        std::vector<ContainedElement> children;
        BindableCollection<ListView, Bindings> itemSource;
        Saturn::Parse::List* itemTemplate;
        int currentPosition {0};
    };
}