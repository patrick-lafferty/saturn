#include <ctype.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

/*
isalnum = isxdigit | isalpha
isalpha = isupper | islower
isblank = \tab | \space
iscntrl = bit 1
isdigit = bit 2
isgraph = bit 3
islower = bit 4
isprint = isgraph | \space
ispunct = bit 5
isspace = bit 6
isupper = bit 7
isxdigit = bit 8
*/

uint8_t lookupTable[256];
enum class LookupBits {
    Control = 1 << 0,
    Digit = 1 << 1,
    Graph = 1 << 2,
    Lowercase = 1 << 3,
    Punctuation = 1 << 4,
    Space = 1 << 5,
    Uppercase = 1 << 6,
    HexDigit = 1 << 7
};

void loadLookupTable() {
    memset(&lookupTable, 0, sizeof(lookupTable));

    for (uint8_t i = 0; i < 0x7F; i++) {
        if (i <= 0x8) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Control);
        }
        else if (i >= 0x9 && i <= 0xD) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Control)
                | static_cast<uint8_t>(LookupBits::Space);
        }
        else if (i >= 0xE && i <= 0x1F) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Control);
        }
        else if (i == 0x20) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Space);
        }
        else if (i >= 0x21 && i <= 0x2F) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Punctuation)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x30 && i <= 0x39) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Digit)
                | static_cast<uint8_t>(LookupBits::HexDigit)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x3A && i <= 0x40) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Punctuation)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x47 && i <= 0x5A) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Uppercase)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x5B && i <= 0x60) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Punctuation)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x61 && i <= 0x66) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Lowercase)
                | static_cast<uint8_t>(LookupBits::HexDigit)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x67 && i <= 0x7A) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Lowercase)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i >= 0x7B && i <= 0x7E) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Punctuation)
                | static_cast<uint8_t>(LookupBits::Graph);
        }
        else if (i == 0x7F) {
            lookupTable[i] = static_cast<uint8_t>(LookupBits::Control);
        }
    }
}

int isalnum(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return (flags & static_cast<uint8_t>(LookupBits::HexDigit))
            || isalpha(ch);
    }
    else {
        return 0;
    }
}

int isalpha(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & 
            (static_cast<uint8_t>(LookupBits::Uppercase)
            | static_cast<uint8_t>(LookupBits::Lowercase));
    }
    else {
        return 0;
    } 
}

int islower(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Lowercase); 
    }
    else {
        return 0;
    }
}

int isupper(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Uppercase);
    }
    else {
        return 0;
    }
}

int isdigit(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Digit);
    }
    else {
        return 0;
    }
}

int isxdigit(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::HexDigit);
    }
    else {
        return 0;
    }
}

int iscntrl(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Control);
    }
    else {
        return 0;
    }
}

int isgraph(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Graph);
    }
    else {
        return 0;
    }
}

int isspace(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Space);
    }
    else {
        return 0;
    }
}

int isblank(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        return ch == '\t' || ch == ' ';
    }
    else {
        return 0;
    }
}

int isprint(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        return isgraph(ch) || ch == ' ';
    }
    else {
        return 0;
    }
}

int ispunct(int ch) {
    if (ch >= 0 && ch <= UCHAR_MAX) {
        auto flags = lookupTable[static_cast<unsigned char>(ch)];

        return flags & static_cast<uint8_t>(LookupBits::Punctuation);
    }
    else {
        return 0;
    }
}