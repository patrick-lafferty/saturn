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

	/*
	Each derived container type may have its own set of meta properties.
	Those that do get their own namespace defined in here 
	*/
	enum class MetaNamespace {
		Grid		
	};

	/*
	MetaData is data that is attached to some UI element that its parent
	container uses to change how the element is arranged

	For example:

	Grids have rows and columns. Instead of hardcoding a rowNumber variable
	in the base UIElement, the row number can be supplied from elsewhere.
	*/
	struct MetaData {
		/*
		ContainerNamespace specifies what container this data belongs to
		*/
		MetaNamespace containerNamespace;

		/*
		Each container can have their own unique set of properties that
		they expose as attachable metadata, defined in their own enum class.
		*/
		uint32_t metaId;

		/*
		The value of the attached property, currently only int types
		are supported
		*/
		int value;
	};

	/*
	Base class for container children. Allows for derived containers
	to store container-specific data with each child, such as rows
	and columns for Grid children.
	*/
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

		/*
		Adds a single element to the container with default configuration
		*/
		virtual void addChild(UIElement* element) = 0;

		/*
		Adds a single element to the container with some container-specific
		configuration
		*/
		virtual void addChild(UIElement* element, const std::vector<MetaData>& meta) = 0;

		/*
		Adds a single child container with default configuration
		*/
		virtual void addChild(Container* container) = 0;

		/*
		Adds a single child container with some container-specific
		configuration
		*/
		virtual void addChild(Container* container, const std::vector<MetaData>& meta) = 0;

		/*
		Arranges the children in some fashion by calculating each
		child element's bounds
		*/
		virtual void layoutChildren() = 0;

		/*
		Returns the bounds of the given child. Bounds are calculated
		in a layoutChildren call and stored in the ContainedElement
		*/
		virtual Bounds getChildBounds(const UIElement* child) = 0;

		/*
		Forwards text layout requests up the ui hierarchy until it
		reaches the Window, which is capable of performing the request.
		*/
        virtual void requestLayoutText(UIElement* element) {
			auto parent = getParent();

			if (parent != nullptr) {
				parent->requestLayoutText(element);
			}
		}

		/*
		Forwards render requests up the ui hierarchy until it
		reaches the Window, which is capable of performing the request.
		*/
        virtual void requestRender(UIElement* element) {
			auto parent = getParent();

			if (parent != nullptr) {
				parent->requestRender(element);
			}
		}

    private:
    };
}
