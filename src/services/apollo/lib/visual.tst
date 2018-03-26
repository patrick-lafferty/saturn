(grid
    (margin 5)

    (items 
        (label (caption "Menu"))
        (textbox (bind text commandLine) (grid-row 1))

        (grid-row 1 (textbox (bind text commandLine)))

        (with 
            (grid-row 1)
            (grid-column 1)
            (textbox (bind text commandLine)))

        (textbox 
            (bind text commandLine)
            (grid-row 1)
            (grid-column 1))

        (textbox 
            (bind text commandLine)
            (meta (grid
                    (row 1)
                    (column 1))))

        (textbox
            (bind text commandLine)
            (meta (grid (row 1) (column 1))))

        (+grid+row 1)

        (grid 
            (bind items availableCommands)
            (auto-columns 1 1)
            (column-gap 3))))
