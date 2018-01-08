#include "driver.h"
#include <services.h>
#include <system_calls.h>
#include <services/startup/startup.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

using namespace Kernel;

namespace BGA {

    void registerMessages() {

    }

    struct FreeTypeConfig {
        FT_Library library;
        FT_Face face;
    };

    void messageLoop(uint32_t address, FreeTypeConfig config) {
        /*auto linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(address);

        auto colour = 0x00'FF'00'00u;
        auto bufferSize = 800 * 600 * 4;

    
auto theta = 0.f;
        while (true) {
        char* name = "pat";
        auto index = 0;
        auto it = name;
        auto lastPitch = 0;
        FT_Matrix  matrix;
        while (*it != '\0') {
            auto glyphIndex = FT_Get_Char_Index(config.face, *it); 
            auto error = FT_Load_Glyph(
                config.face,
                glyphIndex,
                0
            );
            theta++;
            auto rad = theta * 3.14159 / 180;
float sinangle;
float cosangle;
asm("fsin" : "=t" (sinangle) : "0" (rad));
asm("fcos" : "=t" (cosangle) : "0" (rad));

            matrix.xx = cosangle * 3 * 0x10000L ;
matrix.xy = -sinangle * 0.12 * index * 0x10000L;
matrix.yx = sinangle + 0;
matrix.yy = cosangle * 3 * 0x10000L;


FT_Glyph glyph;
FT_Get_Glyph( config.face->glyph, &glyph );
FT_Glyph_Transform( glyph, &matrix, 0 );
FT_Vector  origin;


origin.x = 32; 
origin.y = 0;
            error = FT_Render_Glyph(config.face->glyph, FT_RENDER_MODE_NORMAL);
            FT_Glyph_To_Bitmap(
          &glyph,
          FT_RENDER_MODE_NORMAL,
          &origin,
          1 );
            auto slot = config.face->glyph;
FT_BitmapGlyph  bit = (FT_BitmapGlyph)glyph;
            for (int y = 0; y < bit->bitmap.rows; y++) {
                for (int x = 0; x < bit->bitmap.pitch; x++) {
                    linearFrameBuffer[x + lastPitch + y * 800] = *bit->bitmap.buffer++;
                }
            }

            lastPitch += slot->bitmap.pitch + 100;

            index++;
            it++;
        }
        }
*/
        Startup::runProgram("/bin/windows.service");

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

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry) {

            if (buffer.messageId == static_cast<uint32_t>(MessageId::RegisterServiceDenied)) {
                //can't print to screen, how to notify?
            }

            else if (buffer.messageId == static_cast<uint32_t>(MessageId::VGAServiceMeta)) {
                auto msg = IPC::extractMessage<VGAServiceMeta>(buffer);

                registerMessages();

                NotifyServiceReady ready;
                send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

                return msg.vgaAddress;
            }
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

    uint16_t readRegister(BGAIndex bgaIndex) {
        uint16_t ioPort {0x01CE};
        uint16_t dataPort {0x01CF};
       
        auto index = static_cast<uint16_t>(bgaIndex);
        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (index), "d" (ioPort));

        uint16_t result {0};

        asm volatile("inw %1, %0"
            : "=a" (result)
            : "Nd" (dataPort));

        return result;
    }

    void discover() {
        auto id = readRegister(BGAIndex::Id);

        /*
        QEMU reports 0xB0C0
        VirtualBox reports 0xB0C4
        */
        printf("[BGA] Id: %d\n", id);
    }

    FreeTypeConfig setupAdaptor() {
//if (false) {
        //discover();
        writeRegister(BGAIndex::Enable, 0);

        writeRegister(BGAIndex::XRes, 800);
        writeRegister(BGAIndex::YRes, 600);
        writeRegister(BGAIndex::BPP, 32);

        writeRegister(BGAIndex::Enable, 1 | 0x40);

//}
        FreeTypeConfig config;

        auto error = FT_Init_FreeType(&config.library);

        if (error != 0) {
            printf("[BGA] Error initializing FreeType: %d\n", error);
            return {nullptr, nullptr};
        }

        error = FT_New_Face(config.library,
            "/system/fonts/OxygenMono-Regular.ttf",
            0,
            &config.face);

        if (error != 0) {
            printf("[BGA] Error creating font face: %d\n", error);
            return {nullptr, nullptr};
        }

        error = FT_Set_Char_Size(
            config.face,
            0,
            16 * 64,
            300,
            300
        );

        if (error != 0) {
            printf("[BGA] Error setting character size: %d\n", error);
            return {nullptr, nullptr};
        }

        return config;
    }

    void service() {
        auto bgaAddress = registerService();
        sleep(200);
        auto freetypeConfig = setupAdaptor();

        if (freetypeConfig.library == nullptr) {
            printf("[BGA] Error while setting up freetype, shutting down BGA driver\n");
            return;
        }
        else {
            printf("[BGA] setup freetype\n");
        }

        messageLoop(bgaAddress, freetypeConfig);
    }
}