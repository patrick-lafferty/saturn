#pragma once

#include <services/virtualFileSystem/vostok.h>
#include <system_calls.h>

namespace HardwareFileSystem {

    class HardwareObject : public Vostok::Object {
    public:

        virtual const char* getName() = 0;
    };

    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success);

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