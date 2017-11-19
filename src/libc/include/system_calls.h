#pragma once

#include <stdint.h>

extern "C" void sleep(uint32_t milliseconds);

extern "C" void print(int a, int b);

namespace IPC {
    struct Message;
}

void send(uint32_t taskId, IPC::Message* message);
void receive(IPC::Message* buffer);