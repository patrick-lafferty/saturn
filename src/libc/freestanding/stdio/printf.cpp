#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <system_calls.h>
#include <services.h>
#include <services/terminal/vga.h>
#include <services/terminal/terminal.h>

template<typename T>
void printInteger(uint32_t i, T& write, bool isNegative, int base = 10, bool upper = false) {
    char buffer[CHAR_BIT * sizeof(int) - 1];
    char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
    int digits {0};

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

    while (digits >= 0) {
        write(buffer[digits]);
        digits--;
    }
}

template<typename T>
void printInteger(int32_t i, T& write, int base = 10, bool upper = false) {
    if (i < 0) {
        printInteger(i * -1, write, true, base, upper);
    }
    else {
        printInteger(i, write, false, base, upper); 
    }
}

template<typename T>
int printf_impl(const char* format, va_list args, T write, int& charactersWritten) {

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
                            unsigned char c = va_arg(args, int);
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
                            printInteger(i, write);                            

                            done = true;

                            break;
                        }
                        case 'o': {
                            auto i = va_arg(args, int);
                            printInteger(i, write, 8);   

                            done = true;
                            break;
                        }
                        case 'x': {
                            auto i = va_arg(args, int);
                            printInteger(static_cast<uint32_t>(i), write, false, 16);   

                            done = true;
                            break;
                        }
                        case 'X': {
                            auto i = va_arg(args, int);
                            printInteger(i, write, false, 16, true);

                            done = true;
                            break;
                        }
                        case 'u': {
                            auto i = va_arg(args, int);
                            printInteger(static_cast<uint32_t>(i), write, false);

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

    return charactersWritten;
}

int kprintf(const char* format, ...) {
    if (format == nullptr) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int charactersWritten = 0;

    auto& terminal = Terminal::Terminal::getInstance();
    auto write = [&](auto c) {
        terminal.writeCharacter(c);
        charactersWritten++;
    };

    printf_impl(format, args, write, charactersWritten);

    va_end(args);

    return charactersWritten;
}

int printf(const char* format, ...) {
    if (format == nullptr) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int charactersWritten = 0;
    Terminal::PrintMessage message;
    message.serviceType = Kernel::ServiceType::Terminal;
    memset(message.buffer, '\0', sizeof(message.buffer));

    auto write = [&](auto c) {
        message.buffer[charactersWritten] = c;
        charactersWritten++;
    };

    printf_impl(format, args, write, charactersWritten);

    va_end(args);
    
    message.buffer[charactersWritten] = '\0';
    message.stringLength = charactersWritten;
    send(IPC::RecipientType::ServiceName, &message);

    return charactersWritten;
}

int sprintf(char* buffer, const char* format, ...) {
    if (format == nullptr) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int charactersWritten = 0;

    auto write = [&](auto c) {
        buffer[charactersWritten] = c;
        charactersWritten++;
    };

    printf_impl(format, args, write, charactersWritten);

    va_end(args);

    return charactersWritten;
}