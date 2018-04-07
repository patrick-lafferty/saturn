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

#include "element.h"
#include "control.h"
#include <variant>
#include <vector>

namespace Apollo::Elements {

	enum class MetaNamespace {
		Grid		
	};

	struct MetaData {
		MetaNamespace containerNamespace;
		uint32_t metaId;

		int value;
	};

	struct ContainedElement {
		std::variant<UIElement*, Container*> element;
		Bounds bounds;
	};

    /*
    A Container is an element that arranges multiple child
    elements in some specific layout.
    */
    class Container : public UIElement {
    public:

		Container() = default;
		Container(Configuration& config) : UIElement(config) {}

		virtual void addChild(UIElement* element) = 0;
		virtual void addChild(UIElement* element, const std::vector<MetaData>& meta) = 0;
		virtual void addChild(Container* container) = 0;
		virtual void addChild(Container* container, const std::vector<MetaData>& meta) = 0;

		virtual void layoutChildren() = 0;

		virtual Bounds getChildBounds(const UIElement* child) = 0;

        virtual void requestLayoutText(UIElement* element) {
			auto parent = getParent();

			if (parent != nullptr) {
				parent->requestLayoutText(element);
			}
		}

        virtual void requestRender(UIElement* element) {
			auto parent = getParent();

			if (parent != nullptr) {
				parent->requestRender(element);
			}
		}

    private:
    };
}