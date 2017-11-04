#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#include <services/terminal/vga.h>
#include <services/terminal/terminal.h>

int printInteger(uint32_t i, Terminal& terminal, bool isNegative, int base = 10, bool upper = false) {
    char buffer[CHAR_BIT * sizeof(int) - 1];
    char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
    int digits {0};

    /*bool isNegative {i < 0};
    
    if (isNegative) {
        i *= -1;
    }*/
    
    do {
        buffer[digits] = hexDigits[i % base];

        if (upper && buffer[digits] >= 'a') {
            buffer[digits] -= 32;
        }

        digits++;
        i /= base;
    } while (i > 0);

    if (isNegative) {
        buffer[digits++] = '-';
    }

    digits--;
    int charactersWritten = digits;

    while (digits >= 0) {
        terminal.writeCharacter(buffer[digits]);
        digits--;
    }

    return charactersWritten;
}

int printInteger(int32_t i, Terminal& terminal, int base = 10, bool upper = false) {
    if (i < 0) {
        return printInteger(i * -1, terminal, true, base, upper);
    }
    else {
        return printInteger(i, terminal, false, base, upper); 
    }
}

int printf(const char* format, ...) {
    if (format == nullptr) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int charactersWritten = 0;

    auto& terminal = Terminal::getInstance();
    //auto colour = getColour(VGA::Colours::LightBlue, VGA::Colours::DarkGray);
    auto write = [&](auto c) {
        terminal.writeCharacter(c);//, colour);
        charactersWritten++;
    };

    while(*format != '\0') {

        if (*format == '%') {
            auto next = *++format;

            if (next == '%') {
                write(*format);
            }
            else {
                bool done {false};

                while (!done) {
                    switch (*format) {
                        case 'c': {
                            auto c = va_arg(args, int);
                            write(c);
                            done = true;
                            break;
                        }
                        case 's': {
                            auto s = va_arg(args, char*);

                            while(s && *s != '\0') {
                                write(*s++);
                            }

                            done = true;

                            break;
                        }
                        case 'd': {
                            auto i = va_arg(args, int);
                            charactersWritten += printInteger(i, terminal);                            

                            done = true;

                            break;
                        }
                        case 'o': {
                            auto i = va_arg(args, int);
                            charactersWritten += printInteger(i, terminal, 8);   

                            done = true;
                            break;
                        }
                        case 'x': {
                            auto i = va_arg(args, int);
                            charactersWritten += printInteger(static_cast<uint32_t>(i), terminal, false, 16);   

                            done = true;
                            break;
                        }
                        case 'X': {
                            auto i = va_arg(args, int);
                            charactersWritten += printInteger(i, terminal, false, 16, true);

                            done = true;
                            break;
                        }
                        case 'u': {
                            auto i = va_arg(args, int);
                            charactersWritten += printInteger(static_cast<uint32_t>(i), terminal, false);

                            done = true;
                            break;
                        }
        
                    }

                    format++;
                }
            }

        }
        else {
            write(*format);
            format++;
        }
    }

    va_end(args);

    return charactersWritten;
}