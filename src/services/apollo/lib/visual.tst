(grid
    (margin 5)

    (items 
        (label (caption "Menu"))
        (textbox (bind text commandLine))
        (grid 
            (bind items availableCommands)
            (auto-columns 1 1)
            (column-gap 3))))
