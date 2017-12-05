#include "driver.h"
#include <services.h>
#include <system_calls.h>

using namespace Kernel;

namespace BGA {

    void registerMessages() {

    }

    uint8_t glyphs[][8] = { 
        //a
        {
            0b00011000,
            0b00100100,
            0b01000010,
            0b11111111,
            0b10000001,
            0b10000001,
            0b10000001,
            0b00000000,
        },
        //B
        {
            0b11111000,
            0b10000110,
            0b10000010,
            0b11111000,
            0b10000100,
            0b10000010,
            0b10000110,
            0b11111000,
        }, 
        //C
        {
            0b11111111,
            0b10000000,
            0b10000000,
            0b10000000,
            0b10000000,
            0b10000000,
            0b10000000,
            0b11111111,
        }, 
        //D
        {
            0b11111000,
            0b10000100,
            0b10000100,
            0b10000100,
            0b10000100,
            0b10000100,
            0b10000100,
            0b11111000,
        }, 
        //E
        {
            0b11111100,
            0b10000000,
            0b10000000,
            0b11111100,
            0b10000000,
            0b10000000,
            0b10000000,
            0b11111100,
        }, 
        //F
        {
            0b11111100,
            0b10000000,
            0b10000000,
            0b11111100,
            0b10000000,
            0b10000000,
            0b10000000,
            0b10000000,
        }, 
    };

    void drawA(uint32_t volatile* lfb, uint8_t glyph) {
        /*
        8pixels by 8 pixels

        1 row is 8 pixels is 32 bytes
        */
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (glyphs[glyph][y] & (1 << (7 - x))) {
                    lfb[x + y * 800] = 0x00'00'00'FF;
                }
            }
        }
    }

    void messageLoop(uint32_t address) {
        auto linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(address);

        auto colour = 0x00'FF'00'00u;
        auto bufferSize = 800 * 600 * 4;

        //memset(linearFrameBuffer, colour, bufferSize);
        /*for (int y = 0; y < 600; y++) {
            for (int x = 0; x < 800; x++) {
                linearFrameBuffer[x + y * 800] = colour;
            }

            if (y == 100) colour = 0x00'2F'00'00; 
            if (y == 200) colour = 0x00'00'fF'00;
            if (y == 300) colour = 0x00'00'2F'00;
            if (y == 400) colour = 0x00'00'00'FF;
            if (y == 500) colour = 0x00'00'00'2F;
        }*/
        auto asPerWidth = 800 / 8;
        auto ysPerWidth = 600/8;

        for (int y = 0; y < 600; y+= 16) {
            for (int x = 0; x < 800; x+= 16) {
                drawA(linearFrameBuffer + x + y * 800, x > 400 ? 5 : 4);
            }
        }

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);
        }
    }

    uint32_t registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::BGA;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //can't print to screen, how to notify?
        }
        else if (buffer.messageId == VGAServiceMeta::MessageId) {
            auto msg = IPC::extractMessage<VGAServiceMeta>(buffer);

            registerMessages();

            NotifyServiceReady ready;
            send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

            return msg.vgaAddress;
        }

        return 0;
    }

    enum class BGAIndex {
        Id,
        XRes,
        YRes,
        BPP,
        Enable,
        Bank,
        VirtWidth,
        VirtHeight,
        XOffset,
        YOffset
    };

    void writeRegister(BGAIndex bgaIndex, uint16_t value) {
        uint16_t ioPort {0x01CE};
        uint16_t dataPort {0x01CF};
       
        auto index = static_cast<uint16_t>(bgaIndex);
        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (index), "d" (ioPort));

        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (value), "d" (dataPort));
    }

    void setupAdaptor() {
        writeRegister(BGAIndex::Enable, 0);

        writeRegister(BGAIndex::XRes, 800);
        writeRegister(BGAIndex::YRes, 600);
        writeRegister(BGAIndex::BPP, 32);

        writeRegister(BGAIndex::Enable, 1 | 0x40);
    }

    void service() {
        auto bgaAddress = registerService();
        setupAdaptor();
        messageLoop(bgaAddress);
    }
}