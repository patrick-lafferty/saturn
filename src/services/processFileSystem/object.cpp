#include <string.h>
#include "object.h"
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VFS;
using namespace Vostok;

namespace PFS {

    //functions
    int ProcessObject::getFunction(char* name) {
        if (strcmp(name, "testA") == 0) {
            return static_cast<int>(FunctionId::TestA);
        }
        else if (strcmp(name, "testB") == 0) {
            return static_cast<int>(FunctionId::TestB);
        }

        return -1;
    }

    void ProcessObject::readFunction(uint32_t requesterTaskId, uint32_t functionId) {
        describeFunction(requesterTaskId, functionId);
    }

    void replyWriteFailed(uint32_t requesterTaskId) {
        WriteResult result;
        result.success = false;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::writeFunction(uint32_t requesterTaskId, uint32_t functionId, ArgBuffer& args) {
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {

                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.readFailed) {
                    testA(requesterTaskId, x);
                }
                else {
                    replyWriteFailed(requesterTaskId);
                }

                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {

                auto b = args.read<bool>(ArgTypes::Bool);

                if (!args.readFailed) {
                    testB(requesterTaskId, b);
                }
                else {
                    replyWriteFailed(requesterTaskId);
                }

                break;
            }
            default: {
                replyWriteFailed(requesterTaskId);
            }
        }
    }

    void ProcessObject::describeFunction(uint32_t requesterTaskId, uint32_t functionId) {
        ReadResult result {};
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {
                args.writeType(ArgTypes::Uint32);
                args.writeType(ArgTypes::Void);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {
                args.writeType(ArgTypes::Bool);
                args.writeType(ArgTypes::Void);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    //properties
    int ProcessObject::getProperty(char* name) {
        return -1;
    }

    void ProcessObject::readProperty(uint32_t requesterTaskId, uint32_t propertyId) {
        describeProperty(requesterTaskId, propertyId);
    }

    void ProcessObject::writeProperty(uint32_t requesterTaskId, uint32_t propertyId, ArgBuffer& args) {
        switch(propertyId) {
            
            default: {
                replyWriteFailed(requesterTaskId);
            }
        }
    }

    void ProcessObject::describeProperty(uint32_t requesterTaskId, uint32_t propertyId) {
        ReadResult result {};
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        
        switch(propertyId) {
           
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }


    //process-specific implementation

    void acknowledgeWrite(uint32_t requesterTaskId) {
        WriteResult result;
        result.success = true;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::testA(uint32_t requesterTaskId, int x) {
        acknowledgeWrite(requesterTaskId);
        ReadResult result;
        result.success = true;
        char s[10];
        memset(s, '\0', sizeof(s));
        sprintf(s, "testA: %d", x);
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::testB(uint32_t requesterTaskId, bool b) {
        acknowledgeWrite(requesterTaskId);
        ReadResult result;
        result.success = true;
        char s[10];
        memset(s, '\0', sizeof(s));
        sprintf(s, "testB: %d", b);
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}