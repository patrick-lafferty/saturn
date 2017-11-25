#pragma once

#include <stdint.h>

extern "C" void sleep(uint32_t milliseconds);

extern "C" void print(int a, int b);

namespace IPC {
    struct Message;
    enum class RecipientType;
}

enum class SystemCall {
    Exit = 1,
    Sleep,
};

//TODO: should return a bool for success/failure
//ie check if messageId = 0, means service wasn't setup yet
void send(IPC::RecipientType recipient, IPC::Message* message);
void receive(IPC::Message* buffer);