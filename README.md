Bare-metal experimentation tutorial (aarch64, raspi3)
=====================================================

This repo is mostly intended as notes to myself, while I try to understand how
to run bare-metal programs on the Raspberry Pi 3 (or `raspi3`), using a mix of
Arm64 (`aarch64`) assembly and C code.

My initial goal is to understand how the exception levels (`EL`) work in order
to come up with small, yet interesting pieces of assembly code that we will be
able to formally verify (using a research tool under development). I'll try to
explain all the concepts on the way, so that you can follow along.


How to follow along
-------------------

Each step of this journey has a corresponding directory `step-XX`, including a
`README.md` file with explanations and instructions. Each of these directories
will be self-contained, and built on top of the previous step's directory. The
explanations at any given step will however assume all knowledge gained during
previous steps. (Comments in the code will however be kept as long as they are
sensible, as reminders for certain points.)

My advice is hence to go through all the steps in order, reading corresponding
`README.md` files, and looking at the source code. Of course, it may be a good
idea to follow along by creating your own repo, and by making your code evolve
at each step.


Index of available project steps
--------------------------------

- [Step 0](./step-00/README.md): installing a cross-compiler
- [Step 1](./step-01/README.md): building and running a minimal image
- [Step 2](./step-02/README.md): inspecting the machine state with GDB
- [Step 3](./step-03/README.md): determining the exception level
- [Step 4](./step-04/README.md): setting up a C environment
- [Step 5](./step-05/README.md): debug printing (in C) via an UART
- [Step 6](./step-06/README.md): a minimal debugging shell
