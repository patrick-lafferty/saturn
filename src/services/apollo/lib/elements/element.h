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
#include <stdint.h>

namespace Saturn::Parse {
	struct List;
    struct SExpression;
}

namespace Apollo {
    class Renderer;

    namespace Text {
        class Renderer;
    }
}

namespace Apollo::Elements {

	struct Bounds {
		int x, y;
		int width, height;
	};

    struct Margins {
        int vertical {0};
        int horizontal {0};
    };

	struct Configuration {
		Saturn::Parse::List* meta {nullptr};
        uint32_t backgroundColour {0};
        uint32_t fontColour {0x00'64'95'EDu};
        Margins margins;
        Margins padding;
	};

	class Container;

    /*
    Base class for all elements
    */
	class UIElement {
	public:

        UIElement() = default;
        UIElement(Configuration& config);
        virtual ~UIElement() {}

        int getDesiredWidth();
        int getDesiredHeight();

        Container* getParent();
        void setParent(Container* parent);

        Bounds getBounds() const;

        virtual void layoutText(Apollo::Text::Renderer* renderer) = 0; 
        virtual void render(Renderer* renderer) = 0;

        void requestLayoutText();
        void requestRender();

    private:

        int desiredWidth, desiredHeight;
        Container* parent {nullptr};

    protected:

        uint32_t backgroundColour;
        uint32_t fontColour;
        Margins margins;
        Margins padding;
    };

    /*
    Base parsing function that dervied elements use as a 
    fallback to parse a single constructor at a time
    */
    bool parseElement(Saturn::Parse::SExpression* element, Configuration& config);
}
