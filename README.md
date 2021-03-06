# Summer 2017 ARM11 Group Project (Group 1)
This project was originally created for the first year Computing summer 2017
group project task at Imperial College, London.

The group consists of the following members: Aurel Bílý, Alexis I. Marinoiu,
Andrei V. Șerbănescu, Niklas Vangerow.

## Overview
Originally this repository contained the ARM11 emulator and assembler that we
were tasked to create, but these have been removed with future cohorts in mind.

The repository is structured in the following way

 - `/nes` - The piNES NES emulator. Build instructions are in this directory.
 - `/tcp_client` - The pi-side NES controller driver.

## Extension - NES emulator "piNES"
For the extension we have decided to create an emulator of the NES (Nintendo
Entertainment System). Currently the goal is to make this emulator work with the
SDL library for possible cross-platform compatibility. However, we are also
planning to use the GPIO pins of a Raspberry Pi to interface with a genuine NES
controller.

This project is quite complex, but it can be separated into relatively
well-isolated modules as follows:

 - CPU - Central Processing Unit - Decodes and executes instructions
 - MMU - Memory Mapper Unit - Maps virtual memory addresses to physical memory
 - APU - Audio Processing Unit - Generates waveforms for sound output
 - PPU - Picture Processing Unit - Handles blitting of tiles and sprites
 - IO - Input / Output - Emulator interface via GPIO pins

Instructions for compilation of the extension can be found in `/nes/README.md`.

## Branching model

To make things easier for everyone, we use the git-flow-avh branching model. You
can read about it
[here](https://jeffkreeftmeijer.com/2010/why-arent-you-using-git-flow/).

To set up on Mac with brew:

```
$ brew install git-flow-avh
```

Don't know about the rest.

[Git Flow AVH Edition
Repository](https://github.com/petervanderdoes/gitflow-avh)

## Code Style

We are using the [Google C/C++
Style](https://google.github.io/styleguide/cppguide.html). We don't have to
follow it *exactly*, however, it is expected that you at least invoke
clang-format prior to committing so that we don't have merge conflicts.

The Google C style was chosen simply because we were using the Google style for
Java. These styles do differ in some ways, so it may help to read the document
linked above.

## Licence
Licensed under MIT; you can find the licence text in `LICENSE.txt` as well as
`LICENSE-charsub.txt` for a character substitution version.
