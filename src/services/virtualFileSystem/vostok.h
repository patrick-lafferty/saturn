#pragma once

#include <stdint.h>
#include <string_view>

/*
Vostok is the Virtual Object System. It allows for objects to
be mounted in the virtual file system, and accessed just like 
files via open, read, write, create, close syscalls.
*/

namespace Vostok {

    struct ArgBuffer;

    class Object {
    public:

        /*
        getFunction is called when an object is opened, and the path
        is a subdirectory of the object. For example, 
        
        open("/process/1/something")

        where /process/1 is an Object, the ProcessFileSystem needs
        to determine whether "something" is a function, a property,
        or an invalid name. 

        Returns an implementation specific index to a function,
        or -1 if no function matches the given name
        */
        virtual int getFunction(std::string_view name) = 0; 

        /*
        Reading a function returns the signature of the function, using
        ArgBuffers. Function signatures start with ArgTypes::Function,
        then one or more ArgTypes for the parameters (or void if none),
        then one ArgType for the return type, and finally ArgTypes::EndArg.

        This is written to the end of the ReadResult::buffer in reverse order,
        so the buffer looks like:

        ReadResult result;
        result.buffer[last - 1] = ArgTypes::Function
        parameters:
        result.buffer[last - 2] = some ArgTypes value thats not Function, Property, or EndArg
        ... repeat last for each parameter
        return type:
        result.buffer[last - x] = some ArgTypes value thats not Function, Property, or EndArg
        result.buffer[last - x - 1] = ArgTypes::EndArg

        IPC:
            Generates a ReadResult message
        */
        virtual void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) = 0;

        /*
        Writing a function calls the function with the supplied arguments passed via
        ArgBuffer. Each call to writeFunction generates 1 or 2 IPC::Messages. If
        the write succeeded: 
        -WriteResult success = true
        -ReadResult buffer = whatever the called function returned

        else if the write failed:
        -WriteResult success = false

        So whenever you write to a function, you must call receive() at least once
        to read the WriteResult, and then additionally a second receive() if the 
        WriteResult succeeded.

        IPC:
            Generates a WriteResult message
            [Optional] Generates a ReadResult message
        */
        virtual void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) = 0;

        /*
        describeFunction writes the signature of the given function into an ArgBuffer.
        It is called by readFunction
        */
        virtual void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) = 0;

        /*
        When the ProcessFileSystem handles an OpenRequest, it first calls getFunction
        to see if the path matches a function. If it fails it then calls getProperty
        to see if its a property.

        Returns an implementation specific index to a property,
        or -1 if no property matches the given name 
        */
        virtual int getProperty(std::string_view name) = 0;

        /*
        Writes the value of the property at the beginning of the ReadResult buffer,
        and the type of the property at the end of the ReadResult buffer.

        The type is three values:
        1) ArgTypes::Property
        2) <ArgTypes value thats not Function/Property/EndArg>
        3) ArgTypes::EndArg 

        IPC:
            Generates a ReadResult message

        */
        virtual void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) = 0;

        /*
        Sets the property to the given value, if the type of the property matches
        the type of the value in the arg buffer. 
        
        IPC:
            Generates a WriteResult message
        */
        virtual void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) = 0;
    };

    /*
    You can either open
    1) the VostokObject itself, reading it does something like returning a default
        property important to the object
    2) a function, reading returns the function's signature and writing
        calls the function
    3) a property, reading returns the value and writing sets the value
    */
    enum class DescriptorType {
        Object,
        Function,
        Property
    };

    struct FileDescriptor {
        Object* instance;

        union {
            uint32_t functionId;
            uint32_t propertyId;
        };

        DescriptorType type;

        void read(uint32_t requesterTaskId, uint32_t requestId) {
            if (type == DescriptorType::Object) {

            }
            else if (type == DescriptorType::Function) {
                instance->readFunction(requesterTaskId, requestId, functionId);
            }
            else if (type == DescriptorType::Property) {
                instance->readProperty(requesterTaskId, requestId, propertyId);
            }
        }

        void write(uint32_t requesterTaskId, uint32_t requestId, ArgBuffer& args) {
            if (type == DescriptorType::Object) {

            }
            else if (type == DescriptorType::Function) {
                instance->writeFunction(requesterTaskId, requestId, functionId, args);
            }
            else if (type == DescriptorType::Property) {
                instance->writeProperty(requesterTaskId, requestId, propertyId, args);
            }
        }
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

    /*
    An ArgBuffer stores values and their types together. Values are 
    stored at the beginning of the buffer, and their corresponding
    types are stored at the end of the buffer in reverse order.
    */
    struct ArgBuffer {

        ArgBuffer(uint8_t* buffer, uint32_t length) {
            typedBuffer = buffer;
            nextTypeIndex = length - 1;
            nextValueIndex = 0;
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
                auto size = sizeof(T);
                memcpy(typedBuffer + nextValueIndex, &arg, size);
                nextValueIndex += size;
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
                memcpy(&result, typedBuffer + nextValueIndex, size);
                nextValueIndex += size;
                nextTypeIndex--;
                return result;
            }
        }

        uint8_t* typedBuffer;
        uint32_t nextTypeIndex;
        uint32_t nextValueIndex;

        /*
        Whenever you read or write to an arg buffer, you are responsible
        for checking that both of these are false, otherwise the result
        of the read/write is undefined.
        */
        bool readFailed {false};
        bool writeFailed {false};

        bool hasErrors() const {
            return readFailed || writeFailed;
        }
    };

    template<>
    void ArgBuffer::write(char* arg, ArgTypes type);

    template<>
    void ArgBuffer::write(const char* arg, ArgTypes type);

    template<>
    void ArgBuffer::writeValueWithType(char* arg, ArgTypes type);

    template<>
    void ArgBuffer::writeValueWithType(const char* arg, ArgTypes type);

    template<>
    char* ArgBuffer::read(ArgTypes type);

    template<>
    const char* ArgBuffer::read(ArgTypes type);
}