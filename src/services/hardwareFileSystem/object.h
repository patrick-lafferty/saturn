#pragma once

#include <services/virtualFileSystem/vostok.h>

namespace HardwareFileSystem {

    class HardwareObject : public Vostok::Object {
    public:

        virtual const char* getName() = 0;
    };
}