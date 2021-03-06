<p align="center">
<img src="https://github.com/patrick-lafferty/saturn/blob/master/saturn_logo.png">
</p>

# Saturn
<img src="https://img.shields.io/badge/License-BSD%203--Clause-blue.svg">

Saturn is a new operating system I started in Oct 2017. It is a 64-bit microkernel with multitasking and IPC based around asynchronous message
passing. With the exception of LLVM's libc++ and FreeType, the entire
system and services are being written from scratch by me in C++. Saturn is not
just the kernel, it encompases all the services and drivers needed
to provide a complete desktop system.

The repository is organized into simple sections. The entire kernel is
under src/kernel, all services including Apollo are under src/services,
and applications can be found in src/applications. 

# Features

| ![Latest Screenshot](https://github.com/patrick-lafferty/saturn/blob/master/screenshots/Dsky.PNG) |
| :-: |
| *Dsky application demonstrating Gemini interpreter* |

## Apollo

Apollo is Saturn's UI framework. Apollo has a tiled window manager that divides up the screen into tiles. One of the main goals of Apollo is to support rapid UI prototyping. To 
accomplish this, Apollo uses a declarative layout language called Mercury.
By editing Mercury files you can easily create and modify an application's UI.

```lisp
(grid

    (rows (fixed-height 50) (proportional-height 1))
    (columns (fixed-width 50) (proportional-width 1))

    (items
        
        (label (caption "This is a label")
            (alignment (vertical center))
            (padding (horizontal 10))
            (meta (grid (column 1))))

        (list-view
            (meta (grid (row 1) (column-span 2)))
            (item-source (bind entries))

            (item-template
                (label (caption (bind content))
                    (background (bind background))
                    (font-colour (bind fontColour)))))))
```

Apollo also makes heavy use of databinding. Certain UI elements like Labels
expose properties that are 'bindable', such as caption and background. You
can define properties in your application and then 'bind' them to elements,
and when your values change it automatically updates the appropriate UI element.

Note that the example also shows item templates. You can create a collection
of some user-defined struct, and by defining an item template you tell
Apollo how to create UI elements from that struct.

## Vostok

Vostok is Saturn's Virtual Object System. It allows you to access objects
exposed to the virtual file system just like files. Objects can have properties
as well as callable functions, providing an RPC mechanism. By opening an object
and writing arguments to one of its functions, you can invoke a function call
in a separate process and read the results, just by using the filesystem.
In the Dsky screenshot above, /system/hardware/cpu is actually a Vostok object,
with a nested Id object that has a brand property. 

A core feature of Vostok is [introspection](https://en.wikipedia.org/wiki/Type_introspection). By
reading a Vostok object, you can discover all of the properties and functions they expose, as
well as their types. By reading a function you get the function's signature, ie its input and
output types.

/system/hardware/pci/host0/2/0 is the first PCI function on the third device.
As you can see from the screenshot, by reading an object itself, you can find out what an object supports and how
to interact with it.

# Latest Screenshot

<p align="center">
<img src="https://github.com/patrick-lafferty/saturn/blob/master/screenshots/Transcript.PNG" height="400">
</p>

This screenshot shows the new Taskbar app that lists open applications and
highlights the app with input focus. It also shows the Transcript app which
creates structured logs events from the EventSystem, and allows you to filter
the logs using a Gemini expression.

# Testing

As Saturn is still in its very early stages, there are no releases yet.

# Building

Saturn is currently undergoing a major overhaul and thus isn't runnable, but the build system has now been improved to the point where user builds are possible. This section is just for reference for the time being.

## Requirements
- clang >= 5.0
- svn
- yasm >= 1.3
- cmake >= 3.11
- patch >= 2.7.6
- automake exactly 1.15
- autoconf exactly 2.69
- grub2-mkrescue

## Setup

`git clone https://github.com/patrick-lafferty/saturn.git`

`cd saturn`

`make`


## Running

Qemu:

`make qemu`

Bochs:

`make bochs`

VirtualBox:

`make virtualbox`

# License

Saturn uses the 3-clause BSD license.