#pragma once

/*
The FakeFileSystem is just a hack until a real system like Ext2
is supported. It allows you to "open" the service "binaries".
Since the services are still statically linked to the kernel,
this just returns their function address, while allowing me
to test out the virtual file system and write a startup
service (like systemd/init). 
*/
namespace FakeFileSystem {

    void service();
}