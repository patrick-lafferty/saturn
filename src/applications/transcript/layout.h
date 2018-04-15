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

namespace TranscriptApp {

const char* layout = R"(

(grid

    (margins (vertical 20) (horizontal 20))

    (rows
        (fixed-height 30)
        (proportional-height 1)
    ) 

    (columns 
        (fixed-width 150)
        (proportional-width 1))

    (row-gap 10)

    (items
        (label (caption "Filter events:")
            (alignment (horizontal center) (vertical center))
            (font-colour (rgb 0 0 20))
            (background (rgb 100 149 237))
        )

        (label (caption (bind commandLine))
            (alignment (vertical center))
            (padding (horizontal 10))
            (background (rgb 0 0 20))
            (meta (grid (column 1)))
        )

        (list-view
            (meta (grid (row 1) (column-span 2)))

            (item-source (bind events))

            (item-template
                (label (caption (bind content))
                    (font-colour (rgb 100 149 237))
                    (background (rgb 0 0 20))
                )
            )
        )
    )
)

)";

}
