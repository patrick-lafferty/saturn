#pragma once

#include <stdint.h>

namespace Vostok {

    class Object {
    public:

        virtual void read(uint32_t requesterTaskId, uint32_t functionId) {}
        virtual int getFunction(char* name) {return -1;}
        virtual void describe(uint32_t requesterTaskId, uint32_t functionId) {}
    };

    enum class ArgTypes : uint8_t {
        Void,
        Uint32,
        Cstring,
        EndArg
    };

    struct ArgBuffer {

        ArgBuffer(uint8_t* buffer, uint32_t length) {
            typedBuffer = buffer;
            nextTypeIndex = length - 1;
            nextWriteIndex = 0;
        }

        void writeType(ArgTypes type) {
            typedBuffer[nextTypeIndex] = static_cast<uint8_t>(type);
            nextTypeIndex--;
        }

        ArgTypes readType() {
            auto type = static_cast<ArgTypes>(typedBuffer[nextTypeIndex]);

            if (type != ArgTypes::EndArg) {
                nextTypeIndex--;
            }

            return type;
        }

        ArgTypes peekType() {
            auto type = static_cast<ArgTypes>(typedBuffer[nextTypeIndex]);
            return type;
        }

        template<typename T>
        void write(T arg, ArgTypes type) {
            if (typedBuffer[nextTypeIndex] == static_cast<uint8_t>(ArgTypes::EndArg)
                || static_cast<uint8_t>(type) != typedBuffer[nextTypeIndex]) {

            }
            else {
                nextTypeIndex--;
                memcpy(typedBuffer + nextWriteIndex, arg, sizeof(T));
            }
        }

        uint8_t* typedBuffer;
        uint32_t nextTypeIndex;
        uint32_t nextWriteIndex;
    };

    template<>
    void ArgBuffer::write(char* arg, ArgTypes type);
}