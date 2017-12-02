#include "cpu.h"

using namespace Vostok;

namespace HardwareFileSystem {

    CPUObject::CPUObject() {

    }

    /*
    Vostok Object function support
    */
    int CPUObject::getFunction(std::string_view name) {
        return -1;
    }

    void CPUObject::readFunction(uint32_t requesterTaskId, uint32_t functionId) {
        describeFunction(requesterTaskId, functionId);
    }

    void CPUObject::writeFunction(uint32_t requesterTaskId, uint32_t functionId, ArgBuffer& args) {
    }

    void CPUObject::describeFunction(uint32_t requesterTaskId, uint32_t functionId) {
    }

    /*
    Vostok Object property support
    */
    int CPUObject::getProperty(std::string_view name) {
    }

    void CPUObject::readProperty(uint32_t requesterTaskId, uint32_t propertyId) {
    }

    void CPUObject::writeProperty(uint32_t requesterTaskId, uint32_t propertyId, ArgBuffer& args) {
    }

    /*
    CPUObject specific implementation
    */

    void detectCPU() {

    }
}