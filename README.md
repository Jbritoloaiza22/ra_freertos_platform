# ra_freertos_platform

A learning-oriented project built on the Renesas RA ecosystem to explore FreeRTOS, peripheral integration, and graphical user interfaces using LVGL.

This repository was created as a hands-on platform for understanding how to develop embedded applications on Renesas RA microcontrollers using the Flexible Software Package (FSP), FreeRTOS, and third-party middleware.

Features
FreeRTOS-based application architecture
LVGL graphical user interface
ST7789 TFT display integration
UART communication
Relay control
Power monitoring using a PZEM module
Event-driven task management
Modular driver abstraction
Hardware
Renesas EK-RA2A1 Evaluation Kit
ST7789 TFT Display
PZEM Power Monitoring Module
Relay Module
Software Stack
Renesas FSP
FreeRTOS
LVGL
e² studio
Project Goal

The purpose of this project is educational.

It demonstrates how multiple software and hardware components can be integrated into a complete embedded system while working within the constraints of a low-power MCU.

Topics explored include:

RTOS task design
Driver integration
Peripheral communication
GUI development on resource-constrained devices
Memory optimization
Embedded software architecture
What Makes This Interesting?

One of the main challenges was integrating LVGL on the RA2A1 MCU, which provides only 32 KB of SRAM.

The project demonstrates that a responsive graphical interface can be implemented on a low-memory device through careful resource management and software design.

Repository Structure
tasks/          Application tasks
drivers/        Hardware abstraction and drivers
lvgl/           LVGL integration
freertos/       RTOS configuration
hal_data/       FSP-generated files
Getting Started
Install Renesas e² studio.
Install the required Renesas FSP version.
Clone this repository.
Open the project in e² studio.
Build and flash the firmware to the EK-RA2A1 board.
Disclaimer

This project was developed for learning and experimentation purposes.

It is intended to serve as a reference for engineers interested in learning about Renesas RA microcontrollers, FreeRTOS, and embedded GUI development using LVGL.
