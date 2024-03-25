# LocoNetMbed

This is a preliminary Alpha port of MRRWA Arduino LocoNet library to MBED Platform
(in fact there is no library _per se_, just an example program with an internal library).

**WARNING**: _This software is experimental and absolutely untested_.

_**Do not use it if you are not sure of what you ar doing.**_

## Motivation
The original LocoNet is highly coupled to the original Arduino-UNO platform and one
must start a new port for each new board. The mbed platfrom offers an unified view of
many ARM-based boards because it works at a higher level. This library will expand the
upstream LocoNet library to any mbed-enabled board (consult the list on mbed site).

Once and for all.


## Setup

You will need an mbed enabled board and attach it to any of the LocoNet shields/boards
available out there (see original LocoNet library).

It will be much easier if your board has Arduino-UNO comptaible pinout
(like the the STM32 Nucleo boards).

You must configure in your program the LocoNet Tx and Rx pins (defaults correspond to
the original Arduino-UNO pinout).

## Changes in code

I have tried to make minimal changes on the upstream library. The most notable changes are:

 * Eliminate all the `#ifdef`s regarding architecture.
 * Split the code in several files: each class has its corresponding `.cpp`, `.h` pair
   (for example, the fast clock code resides now in `LocoNetFastClock.cpp` and
   `LocoNetFastClock.h`).
 * No direct manipulation of timers, interrupts, etc. The code uses instead the mbed facilities:
   `InterruptIn` (the former _input capture_), and `Ticker` (the former _timer output compare interrupt_).
 * Add the method `LocoNetClass::init(TxPin, RxPin)` since in mbed the Rx pin in not fixed anymore, it can
   be any free I/O pin supporting the `InterruptIn` class. It only depends on your hardware setup.

## Further steps.

* Move the code to a `Thread` safe paradigm using mbed facilities (Threads, Queues, Events, and friends).
* Put it all on some higher abstracion layer.

## Status

* 2023-03-25:
 ** Reding from LocoNetBus: OK.
 ** Sending to LocoNetBus: FAILS (no detected by other hardware).
