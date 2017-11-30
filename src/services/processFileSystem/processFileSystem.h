#pragma once

#include <ipc.h>

/*
The Process Filesystem is meant to offer similar functionality to Plan9's /proc.
It mounts a collection of ProcessObjects located at /process, with each
object being a "file" in that directory with its pid as its filename.

One ProcessObject is created per kernel task started, and stores read-only
information about the process.
*/

namespace PFS {

    void service();
}