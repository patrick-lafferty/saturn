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
            memcpy(typedBuffer + nextWriteIndex, arg, strlen(arg) + 1);
        }
    }

    template<>
    void ArgBuffer::writeValueWithType(char* arg, ArgTypes type) {
        writeType(type);
        nextTypeIndex++;
        write(arg, type);
    }

    template<>
    char* ArgBuffer::read(ArgTypes type) {
        if (peekType() != type) {
            readFailed = true;
            return {};
        }
        else {
            auto result = reinterpret_cast<const char*>(typedBuffer) + nextReadIndex;
            auto length = strlen(result);
            nextReadIndex += length;
            nextTypeIndex--;
            return const_cast<char*>(result);
        }
    }

    template<>
    const char* ArgBuffer::read(ArgTypes type) {
        return const_cast<const char*>(read<char*>(type));
    }
}