# DEMO CODE FOR FINAL PROJECT

This branch contains a skeleton of the game logic we can adapt for the final project. Right now it only implements the accelerometer and tilt module, which issues a strike if the user tils the device too much.

Folder structure:
- `main.c` - initializes all MCU peripherals, timers, printf support, etc
- `game.c` - where the actual game logic is run
- `periph/` - code for initializing MCU peripherals
- `components/` - code for interfacing with a specific sensor (e.g. accelerometer, switch, display)
- `modules/` - code that implements a submodule in the game, for example, the wire-cutting minigame could have its own file here

The code for reusable components and submodules tries to mimic an object oriented programming style by using structs to contain data with functions that operate on those structs.