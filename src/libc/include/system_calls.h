#pragma once

#include <stdint.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

extern "C" void sleep(uint32_t milliseconds);

extern "C" void print(int a, int b);

namespace IPC {
    struct Message;
    enum class RecipientType;
}

namespace Kernel {
    enum class ServiceType;
}

enum class SystemCall {
    Exit = 1,
    Sleep,
};

//TODO: should return a bool for success/failure
//ie check if messageId = 0, means service wasn't setup yet
void send(IPC::RecipientType recipient, IPC::Message* message);
void receive(IPC::Message* buffer);

void open(const char* path);
VFS::OpenResult openSynchronous(const char* path);
void create(const char* path);
void read(uint32_t fileDescriptor, uint32_t length);
VFS::ReadResult readSynchronous(uint32_t fileDescriptor, uint32_t length);

void write(uint32_t fileDescriptor, const void* data, uint32_t length);
void close(uint32_t fileDescriptor);

void waitForServiceRegistered(Kernel::ServiceType type);

uint32_t run(uintptr_t entryPoint);