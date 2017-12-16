#pragma once

#include <services/virtualFileSystem/vostok.h>
#include <system_calls.h>

namespace HardwareFileSystem {

    class HardwareObject : public Vostok::Object {
    public:

        virtual int getFunction(std::string_view) override;
        virtual void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        virtual void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        virtual void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

        virtual Object* getNestedObject(std::string_view name) override;

        virtual const char* getName() = 0;
    };

    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success, bool expectReadResult = false);

    template<typename Arg>
    void writeTransaction(const char* path, Arg value, Vostok::ArgTypes argType) {
        auto openResult = openSynchronous(path);

        if (openResult.success) {
            auto readResult = readSynchronous(openResult.fileDescriptor, 0);

            if (readResult.success) {
                Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};

                auto type = args.readType();

                if (type == Vostok::ArgTypes::Property) {
                    args.write(value, argType);
                }

                write(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
                receiveAndIgnore();
            }

            close(openResult.fileDescriptor);
            receiveAndIgnore();
        }
    }
}