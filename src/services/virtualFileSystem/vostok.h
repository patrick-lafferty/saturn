#pragma once

#include <stdint.h>

namespace Vostok {

    struct ArgBuffer;

    class Object {
    public:

        virtual int getFunction(char* name) = 0; 
        virtual void readFunction(uint32_t requesterTaskId, uint32_t functionId) = 0;
        virtual void writeFunction(uint32_t requesterTaskId, uint32_t functionId, ArgBuffer& args) = 0;
        virtual void describeFunction(uint32_t requesterTaskId, uint32_t functionId) = 0;

        virtual int getProperty(char* name) = 0;
        virtual void readProperty(uint32_t requesterTaskId, uint32_t propertyId) = 0;
        virtual void writeProperty(uint32_t requesterTaskId, uint32_t propertyId, ArgBuffer& args) = 0;
    };

    enum class ArgTypes : uint8_t {
        Function,
        Property,
        Void,
        Uint32,
        Cstring,
        Bool,
        EndArg
    };

    struct ArgBuffer {

        ArgBuffer(uint8_t* buffer, uint32_t length) {
            typedBuffer = buffer;
            nextTypeIndex = length - 1;
            nextWriteIndex = 0;
            nextReadIndex = 0;
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
                writeFailed = true;
            }
            else {
                nextTypeIndex--;
                memcpy(typedBuffer + nextWriteIndex, &arg, sizeof(T));
            }
        }

        template<typename T>
        void writeValueWithType(T arg, ArgTypes type) {
            writeType(type);
            nextTypeIndex++;
            write(arg, type);
        }

        template<typename T>
        T read(ArgTypes type) {
            if (peekType() != type) {
                readFailed = true;
                return {};
            }
            else {
                T result;
                auto size = sizeof(T);
                memcpy(&result, typedBuffer + nextReadIndex, size);
                nextReadIndex += size;
                nextTypeIndex--;
                return result;
            }
        }

        uint8_t* typedBuffer;
        uint32_t nextTypeIndex;
        uint32_t nextWriteIndex;
        uint32_t nextReadIndex;
        bool readFailed {false};
        bool writeFailed {false};
    };

    template<>
    void ArgBuffer::write(char* arg, ArgTypes type);

    template<>
    void ArgBuffer::writeValueWithType(char* arg, ArgTypes type);

    template<>
    char* ArgBuffer::read(ArgTypes type);

    template<>
    const char* ArgBuffer::read(ArgTypes type);
}