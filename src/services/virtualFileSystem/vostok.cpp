#include "vostok.h"
#include <string.h>

namespace Vostok {
    template<>
    void ArgBuffer::write(char* arg, ArgTypes type) {
        if (typedBuffer[nextTypeIndex] == static_cast<uint8_t>(ArgTypes::EndArg)
            || static_cast<uint8_t>(type) != typedBuffer[nextTypeIndex]) {
            writeFailed = true;
        }
        else {
            nextTypeIndex--;
            auto size = strlen(arg) + 1;
            memcpy(typedBuffer + nextValueIndex, arg, size);
            nextValueIndex += size;
        }
    }

    template<>
    void ArgBuffer::write(const char* arg, ArgTypes type) {
        write(const_cast<char*>(arg), type);
    }

    template<>
    void ArgBuffer::writeValueWithType(char* arg, ArgTypes type) {
        writeType(type);
        nextTypeIndex++;
        write(arg, type);
    }

    template<>
    void ArgBuffer::writeValueWithType(const char* arg, ArgTypes type) {
        writeValueWithType(const_cast<char*>(arg), type);
    }

    template<>
    char* ArgBuffer::read(ArgTypes type) {
        if (peekType() != type) {
            readFailed = true;
            return {};
        }
        else {
            auto result = reinterpret_cast<const char*>(typedBuffer) + nextValueIndex;
            auto length = strlen(result) + 1;
            nextValueIndex += length;
            nextTypeIndex--;
            return const_cast<char*>(result);
        }
    }

    template<>
    const char* ArgBuffer::read(ArgTypes type) {
        return const_cast<const char*>(read<char*>(type));
    }
}