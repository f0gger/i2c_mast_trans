# SNES Controller Firmware

Firmware and Linux kernel driver for connecting a Super Nintendo Entertainment System (SNES) controller to a ROCK64 single-board computer running Lakka Linux.

The project consists of two components that communicate over I²C: an ATmega328P microcontroller that reads the controller inputs, and a Linux kernel module on the ROCK64 that polls the microcontroller and exposes the controller as a Linux input device.

## Project Structure
| File | Platform | Description |
|---|---|---|
| `slaveTrans.c` | ATmega328P | I²C slave firmware that reads controller inputs and sends their state to the master. |
| `masterRecv.c` | ROCK64 / Lakka Linux | Linux kernel module that reads controller state over I²C and registers a Linux input device. |
| `Makefile` | ROCK64 / Linux kernel build environment | Builds the Linux kernel module using the configured kernel source tree and AArch64 cross-compiler. |

## System Architecture
       SNES Controller
              |
              v
        ATmega328P
       (slaveTrans.c)
              |
              | I²C
              | Address: 0x20
              v
          ROCK64
       (masterRecv.c)
              |
              v
      Linux Input Subsystem
              |
              v
       Lakka / RetroArch

## Components
### ATmega328P Firmware

The microcontroller operates as an I²C slave at address 0x20.

Its main responsibilities are:

    Configure the microcontroller's digital input ports.
    Read the controller input states from ports B and D.
    Invert the input values to account for active-low signals.
    Send the input states to the I²C master as two bytes.
    Monitor I²C communication and enter a status LED blinking loop if no response is detected for too long.

The firmware uses the AVR TWI (Two-Wire Interface) peripheral to handle I²C communication.

The first transmitted byte contains the snapshot of port D, and the second contains the snapshot of port B.

### Linux Kernel Module

The Linux kernel module runs on the ROCK64 and communicates with the ATmega328P over I²C.

Its main responsibilities are:

    Register an I²C driver and create an I²C client for the controller.
    Read two bytes of controller state from the microcontroller.
    Translate the individual input bits into gamepad buttons and directional-pad axes.
    Register a Linux input device named Snes Input.
    Report input events to the Linux input subsystem.

The controller is polled approximately every 4 milliseconds using a high-resolution timer and a workqueue.

The driver reports the following controls:    
| SNES control | Linux input event |
|---|---|
| D-pad up | `ABS_HAT0Y = -1` |
| D-pad down | `ABS_HAT0Y = 1` |
| D-pad left | `ABS_HAT0X = -1` |
| D-pad right | `ABS_HAT0X = 1` |
| B | `BTN_SOUTH` |
| Y | `BTN_NORTH` |
| A | `BTN_EAST` |
| X | `BTN_WEST` |
| L | `BTN_TL` |
| R | `BTN_TR` |
| Start | `BTN_START` |
| Select | `BTN_SELECT` |

The directional-pad axes return to 0 when neither direction on the corresponding axis is pressed.

## Configuration

The Linux kernel module defines the following parameters:
| Parameter | Default value | Description |
|---|---|---|
| `slave_addr` | `0x20` | I²C slave address parameter. The current code uses the fixed address `0x20` when creating the I²C client. |
| `i2c_bus` | `1` | Linux I²C adapter number used by the driver. |
| `MS_TIME` | `4` ms | Controller polling interval. |
| `KERN` | `$(HOME)/linux` | Path to the Linux kernel source/build directory. |
| `ARCH` | `arm64` | Target architecture for the kernel module. |
| `CROSS_COMPILE` | `aarch64-linux-gnu-` | Prefix for the AArch64 cross-compilation toolchain. |

**Note:** The current driver declares slave_addr as a module parameter, but the I²C client is created using the fixed address 0x20 in I2C_BOARD_INFO. Changing slave_addr alone will therefore not change the address used by the driver.

## Requirements
### Hardware

    ROCK64 single-board computer.
    Lakka Linux installation with a compatible Linux kernel.
    ATmega328P microcontroller.
    SNES controller and its input wiring.
    I²C connection between the ROCK64 and the ATmega328P, including appropriate power, ground, and logic-level compatibility.

### Software

    Linux kernel source tree or prepared kernel build directory matching the running Lakka kernel.
    AArch64 cross-compilation toolchain (aarch64-linux-gnu-gcc).
    GNU Make.
    AVR toolchain for building slaveTrans.c, including avr-gcc and AVR libc.

## Building the Linux Kernel Module

The provided Makefile builds the Linux kernel module for ARM64.

The KERN variable defaults to $(HOME)/linux, allowing different users to build the project without modifying the Makefile. If the kernel source directory is located elsewhere, its path can be provided when invoking make.

Build the module:
```bash

make
```

Alternatively, specify the kernel directory without editing the Makefile:
```bash

make KERN=/path/to/linux
```

Clean the build artifacts:
```bash

make clean
```

The resulting kernel module is named i2c0-master.ko, based on the obj-m declaration in the Makefile.

Important: Build against the kernel source and configuration corresponding to the kernel running on the ROCK64. A module built against an incompatible kernel may fail to load.

### Building the ATmega328P Firmware

The ATmega firmware is built separately from the Linux kernel module.

For example, if the microcontroller is clocked at 16 MHz:
```bash

avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL \
    -Os -o slaveTrans.elf slaveTrans.c

avr-objcopy -O ihex -R .eeprom \
    slaveTrans.elf slaveTrans.hex
```

Adjust F_CPU to match the actual microcontroller clock. The generated HEX file can then be programmed onto the ATmega328P using a compatible programmer.
Loading the Linux Driver

After building the module, copy i2c0-master.ko to the ROCK64.

Load the module:
```bash

sudo insmod i2c0-master.ko
```

Check kernel messages for initialization and communication information:
```bash

dmesg | tail -n 50
```

Check whether the input device was registered:
```bash

cat /proc/bus/input/devices
```

If the module loads successfully, the controller should appear as Snes Input.

To unload the module:
```bash

sudo rmmod i2c0-master
```

The module name used by rmmod may differ from the output filename if the module is renamed during the build.

## Troubleshooting
| Problem | Things to check |
|---|---|
| Kernel module fails to build | Verify `KERN`, the kernel configuration, and the AArch64 cross-compiler. |
| Module fails to load | Check `dmesg` for kernel version mismatches, missing symbols, or I²C initialization errors. |
| I²C device is not detected | Verify wiring, power, ground, I²C bus number, slave address, and pull-up resistors. |
| Controller inputs are incorrect | Check wiring and bit mappings in `slaveTrans.c` and `masterRecv.c`. |
| Controller is not recognized by Lakka | Confirm that the module loaded and `Snes Input` appears in the Linux input device list. |
| Status LED blinks continuously | Check the I²C connection and whether the master is communicating with the microcontroller. |
