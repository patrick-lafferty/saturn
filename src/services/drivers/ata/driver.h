#pragma once

#include <stdint.h>

namespace ATA {

    /*
    Some registers behave differently when
    reading vs writing, indicated with comments below
    */
    enum class Register {
        Data,
        /*
        Reading Features gives "Error",
        */
        Features,
        SectorCount,
        LBALow,
        LBAMid,
        LBAHigh,
        Device,
        /*
        Reading Command gives "Status"
        */
        Command,
        /*
        Reading Control gives "Alternate" status
        */
        Control 
    };

    enum class Status {
        Error,
        SenseDataAvailable,
        AlignmentError,
        DataRequest,
        DeferredWriteError,
        StreamError,
        DeviceFault,
        DeviceReady,
        Busy
    };

    enum class Device {
        Master,
        Slave,
        None
    };

    class Driver {
    public: 

        Driver(uint8_t device, uint8_t function);
        
    private:

        Device currentDevice {Device::None}; 
    };
}