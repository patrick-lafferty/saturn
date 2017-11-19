#pragma once

#include <stdint.h>

extern "C" void sleep(uint32_t milliseconds);

extern "C" void print(int a, int b);

namespace IPC {
    struct Message;
    enum class RecipientType;
}

void send(IPC::RecipientType recipient, IPC::Message* message);
void receive(IPC::Message* buffer);