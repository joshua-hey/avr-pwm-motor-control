# AVR PWM Motor Speed Control

Bare-metal C firmware for PWM-based DC motor speed control on an ATmega324PB.
Uses timers and interrupts to generate PWM, measure motor speed via an opto-interrupter,
and display real-time data on an I2C OLED.

## Features
- Timer-driven PWM (compare interrupts)
- External interrupt pulse counting (RPM)
- Real-time display over I2C (SSD1306)
- Measured duty-cycle accuracy (see [report](docs/report.pdf))

## Hardware
- ATmega324PB (16 MHz)
- DC motor + slotted encoder
- Opto-interrupter on INT0
- SSD1306 OLED over I2C

## Files
- Firmware: [`src/main.c`](src/main.c)
- Report + measurements: [`docs/report.pdf`](docs/report.pdf)