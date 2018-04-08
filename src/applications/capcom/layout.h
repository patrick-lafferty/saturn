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

const char* layout = R"(

(grid

    (margins (vertical 20) (horizontal 20))

    (rows 
        (fixed-height 100)
        (fixed-height 100)
        (proportional-height 1)
    )

    (columns (proportional-width 1))

    (row-gap 20)

    (items 
        (label (caption "Capcom")
            (background (rgb 69 69 122)))

        (label (caption (bind commandLine))
            (background (rgb 0 0 20))
            (meta (grid (row 1)))
        )

        (grid
            (meta (grid (row 2)))
            (margins (vertical 10) (horizontal 50))

            (rows
                (fixed-height 100)
                (fixed-height 100)
                (fixed-height 100)
                (fixed-height 100)
            )   

            (columns
                (fixed-width 50)
                (proportional-width 1)
                (fixed-width 50)
                (proportional-width 1)
            )

            (item-source (bind currentCommands))

            (item-template
                (label (caption (bind content))
                    (background (bind background)) 
                )
            )        
        )
    )
)

)";
