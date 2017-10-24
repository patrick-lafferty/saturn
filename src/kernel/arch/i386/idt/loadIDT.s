global loadIDT
extern idtPointer
loadIDT:
    lidt [idtPointer]
    ret
