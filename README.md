| ![Latest Screenshot](https://github.com/patrick-lafferty/saturn/blob/master/screenshots/Dsky.PNG) |
| :-: |
| *Dsky application demonstrating Gemini interpreter* |
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

# Saturn

Saturn is a new operating system I started in late 2017. It currently features:

* 32-bit Microkernel, with basically only the scheduler, physical/virtual memory managers, interrupt handler and service registry in the kernel itself, and everything else delegated to user services + a few kernel services
* Paging/Virtual Memory
* Multitasking with 
  * kernel and user tasks
  * separate process memory spaces
  * rudimentary ELF loading support
* IPC
  * asynchronous message passing where messages are identified by two
  uint32s: a global message namespace id, and a message id unique to that namespace
  * shared memory
* a Virtual File System along with 
  * partial Ext2 support (readonly, file sizes limited to about 67M [only singly and doubly indirect blocks implemented, missing triply indirect]) 
  * a process filesystem similar to procfs, and 
  * a hardware filesystem similar to sysfs
* Basic PCI support: enumerating devices and loading appropriate drivers
* Drivers:
  * PS2
  * Keyboard
  * ATA PIO (readonly)
  * Bochs VBE
* Minimal C Standard Library implementation
* GUI Framework and Window Manager (Apollo)

## What's different about Saturn?

The Virtual Object System, Vostok. Inspired by Plan9's "everything is a file*system*", I thought it might be interesting to see what "everything is an *object*" would look like. Objects have properties that you can read and write just like files via the virtual file system, but they also have functions you can call. Instead of having a single control file where you write a command to run it, you can expose multiple "files", with reflection. Objects can also be nested inside other objects.

Reading a function gives you the function's signature (argument types + return type). Writing to a function executes the function. If it has a non-void return type, you will receive a message with the result.

Example: 

The Hardware Filesystem exposes a PCI Object at /system/hardware/pci, which contains a Host Object, which contains Devices which contain Functions that the Discovery system enumerated at startup. To find a device with a certain PCI classId and subclass Id, you can call the find function at /system/hardware/pci/find with the two ids to get the first matching device.  That's actually what the mass storage filesystem does (see src/services/massStorageFileSystem/system.cpp, function callFind).

This is all handled via the virtual file system in a language agnostic way. Essentially all you need is fopen, fread and fwrite and you can call a Vostok function from any language (or read/write properties), and not care what language the object was written in. 

## Userspace

Saturn's userspace will be designed for a graphical environment, using a tiled compositing window manager called Apollo. Apollo currently supports multiple tiles (application windows) in a container that can be split either horizontally or vertically. Containers can be nested inside other containers, allowing you to divide up the screen however you want. You can find Apollo in src/services/apollo.

## Testing

As Saturn is still in its very early stages, there are no releases yet.

## License

Saturn uses the 3-clause BSD license.