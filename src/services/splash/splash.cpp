#include "splash.h"
#include <services.h>
#include <services/terminal/terminal.h>
#include <system_calls.h>

using namespace Kernel;

namespace Splash {

    uint32_t SplashScreen::MessageId;

    void registerMessages() {
        IPC::registerMessage<SplashScreen>();
    }

    void service() {

        waitForServiceRegistered(ServiceType::Terminal);

        Terminal::PrintMessage message {};
        message.serviceType = ServiceType::Terminal;
        auto s = "\e[1;40H\e[48;5;4m\e[38;5;9mSaturn OS\e[48;5;0m\n";
        memcpy(message.buffer, s, 48);
        send(IPC::RecipientType::ServiceName, &message);
    }
}