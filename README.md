# Servo HID Controller (PC to STM32)
A Python GUI that controls a DS3218 servo motor over USB HID using STM32F103 microcontroller

## Project overview
This project implements a USB HID–based control system for a servo motor.
A Python Tkinter GUI running on a PC sends angle and incremental motion commands to an STM32 microcontroller over USB HID. The STM32 firmware decodes HID reports and drives the servo accordingly.
![GUI](/docs/GUI.png)


## System overview
- Windows Host: Python Tkinter GUI
- HID: Custom USB Communication Protocol
- Microcontroller: STM32F103RCT6
- Actuator: DS3218 Digital Servo

## Repository structure
```
pc/
  └─ gui/              # Python GUI (USB HID host)
firmware/
  └─ stm32/            # STM32CubeMX / CubeIDE project
docs/
  └─ hid_protocol.md   # HID report format and commands

```