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
        (label (caption "Padded Label")
            (padding (vertical 50) (horizontal 100))
            (background (rgb 38 66 251)))
        (label (caption "Second")
            (background (rgb 24 205 4))
            (meta (grid (column 1))))

        (label (caption (bind "caption"))
            (background (rgb 69 69 69))
            (meta (grid (row 1))))
        (label (caption "<- that one was typed")
            (background (rgb 37 13 37))
            (meta (grid (row 1) (column 1))))

        (label (caption "Margined Label")
            (background (rgb 248 121 82))
            (margins (vertical 50) (horizontal 90))
            (meta (grid (row 2))))

        (grid

            (rows 
                (proportional-height 2)
                (proportional-height 1))

            (columns 
                (proportional-width 1)
                (proportional-width 1)
                (proportional-width 1))

            (items 
                (label (caption "a")
                    (background (rgb 33 146 195)))
                (label (caption "b")
                    (background (rgb 62 16 140))
                    (meta (grid (column 1))))
                (label (caption "3")
                    (background (rgb 241 220 69))
                    (meta (grid (column 2))))

                (label (caption "spans 3 columns")
                    (background (rgb 12 87 117))
                    (meta (grid (row 1) (column-span 3)))))

            (meta (grid (row 2) (column 1))))))