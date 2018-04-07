(grid
    (margins (vertical 20) (horizontal 20))

    (rows 
        (proportional-height 1)
        (fixed-height 50)
        (proportional-height 2))

    (columns 
        (proportional-width 1)
        (proportional-width 1))

    (items 
        (label (caption "First")
            (background (rgb 38 66 251)))
        (label (caption "Second")
            (background (rgb 24 205 4))
            (meta (grid (column 1))))

        (label (caption "Third")
            (background (rgb 69 69 69))
            (meta (grid (row 1))))
        (label (caption "$NUMBER")
            (background (rgb 37 13 37))
            (meta (grid (row 1) (column 1))))

        (label (caption "Eth")
            (background (rgb 248 121 82))
            (margins (vertical 20) (horizontal 5))
            (meta (grid (row 2))))
        (label (caption "Sixthstnd")
            (background (rgb 207 95 33))
            (meta (grid (row 2) (column 1))))

        ))