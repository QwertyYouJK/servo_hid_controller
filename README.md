# Servo HID Controller (PC → STM32)

A PC-to-microcontroller control system that drives a DS3218 servo motor using USB HID.
A Python Tkinter GUI sends commands over USB HID to an STM32F103 microcontroller, and the firmware converts those commands into PWM output for servo positioning.

![GUI](/docs/GUI.png)

## Overview

This project has two parts:
- **PC host (Python GUI):** sends absolute angle commands and incremental motion commands
- **STM32 firmware:** receives HID reports, parses commands, and updates the PWM signal driving the servo

Why HID:
- Works over USB without custom drivers in most cases (appears as a HID device)
- Simple report-based messaging for small control payloads

## System overview

- **Host OS:** Windows
- **GUI:** Python + Tkinter
- **USB transport:** HID (custom report format)
- **Microcontroller:** STM32F103RCT6
- **Actuator:** DS3218 digital servo
- **PWM output:** TIM2_CH3 (pin 16)

## Repository structure
```bash
pc/
└── gui/                    
    └── custom_hid.py                   # Python GUI (USB HID host)
    └── requirements.txt                # Python requirements
firmware/
└── ...                                 # STM32CubeMX / CubeIDE project
docs
└── DS3218 Communication Protocol.txt   # HID interface communication protocol
└── GUI.png                             # Python GUI image
└── DS3218_circuit.png                  # Simple Circuit diagram image
```

## Hardware setup

- Servo signal wire connected to TIM2_CH3 (pin 16) for PWM generation
- Servo powered by external 5V
- Common ground between the external 5V supply and the STM32 board
- STM32 connected to a **SEGGER J-Link for programming/debug and board power

![Simple circuit diagram](/docs/DS3218_circuit.png)

## Result

- Commands received by the STM32 match the report format described in `docs/DS3218 Communication Protocol.txt`
- STM32 parse command and control channel 3 PWM
- Pressing increment controls updates the servo position or input the angle directly to specify angle