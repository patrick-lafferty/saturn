#include "features.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUFeaturesObject::CPUFeaturesObject() {
        
    }

    void CPUFeaturesObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("instruction_sets", ArgTypes::Cstring);
        args.writeValueWithType("instructions", ArgTypes::Cstring);
        args.writeValueWithType("support", ArgTypes::Cstring);
        args.writeValueWithType("extensions", ArgTypes::Cstring);
        args.writeValueWithType("technology", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int CPUFeaturesObject::getFunction(std::string_view name) {
        return -1;
    }

    void CPUFeaturesObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void CPUFeaturesObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
    }

    void CPUFeaturesObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
    }

    /*
    Vostok Object property support
    */
    int CPUFeaturesObject::getProperty(std::string_view name) {
        return -1;
    }

    void CPUFeaturesObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = false;
        result.requestId = requestId;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUFeaturesObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUFeaturesObject::getNestedObject(std::string_view name) {
        if (name.compare("instruction_sets") == 0) {
            return &instructionSets;
        }
        else if (name.compare("instructions") == 0) {
            return &instructions;
        }
        else if (name.compare("support") == 0) {
            return &support;
        }

        return nullptr;
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUFeaturesObject::getName() {
        return "features";
    }

    void detectFeatures(uint32_t ecx, uint32_t edx) {

        detectInstructionSets(ecx, edx);
        detectInstructions(ecx, edx);
        detectSupport(ecx, edx);
    }
}