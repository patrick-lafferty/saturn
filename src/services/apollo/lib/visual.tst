(grid
    (margin 5)

    (rows 
        (proportional-height 1)
        (fixed-height 50)
        (proportional-height 2))

    (columns 
        (proportional-width 1)
        (proportional-width 1))

    (items 
        (label (caption "Menu"))

        (textbox
            (bind text commandLine)
            (meta (grid (row 0) (column 0))))

        (grid 
            (meta (grid (row 1)))
            (bind items availableCommands)
            (auto-columns 1 1)
            (column-gap 3))))
