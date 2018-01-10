/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <stdint.h>
#include <services/virtualFileSystem/messages.h>

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
    Send,
    Receive,
    FilteredReceive
};

//TODO: should return a bool for success/failure
//ie check if messageId = 0, means service wasn't setup yet
void send(IPC::RecipientType recipient, IPC::Message* message);
void receive(IPC::Message* buffer);
void filteredReceive(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId);
void receiveAndIgnore();

void open(const char* path);
VirtualFileSystem::OpenResult openSynchronous(const char* path);
void create(const char* path);
void read(uint32_t fileDescriptor, uint32_t length);
VirtualFileSystem::ReadResult readSynchronous(uint32_t fileDescriptor, uint32_t length);
VirtualFileSystem::Read512Result read512Synchronous(uint32_t fileDescriptor, uint32_t length);

void write(uint32_t fileDescriptor, const void* data, uint32_t length);
VirtualFileSystem::WriteResult writeSynchronous(uint32_t fileDescriptor, const void* data, uint32_t length);
void close(uint32_t fileDescriptor);

void seek(uint32_t fileDescriptor, uint32_t offset, uint32_t origin);
VirtualFileSystem::SeekResult seekSynchronous(uint32_t fileDescriptor, uint32_t offset, uint32_t origin);

void waitForServiceRegistered(Kernel::ServiceType type);

uint32_t run(uintptr_t entryPoint);
uint32_t run(char* path);

void* map(uint32_t address, uint32_t size, uint32_t flags);