# EmbeddedPongRF

A wireless two-player Pong game implementation for STM32 microcontrollers with RF communication capabilities.

## Overview

EmbeddedPongRF is an embedded systems project that implements the classic Pong game on STM32F429ZI Discovery boards. The game features both single-player modes with AI opponents of varying difficulties and a wireless two-player mode using nRF24L01+ modules for communication between devices.

## Features

- Multiple game modes:
  - AI vs AI (computer plays against itself)
  - Human vs AI (play against computer opponent)
  - Local two-player (play on a single device)
  - Wireless two-player (play across two connected devices)
- Adjustable AI difficulty levels
- Real-time wireless communication between boards
- Scoring system with visual feedback
- Randomized ball trajectories
- Multiple simultaneous balls (up to 8)
- Visual feedback for goals using onboard LEDs

## Tech Stack

### Hardware

- **Microcontroller**: STM32F429ZI Discovery Board
- **Display**: 2.4" LCD display integrated on the Discovery board
- **Wireless Module**: nRF24L01+ 2.4GHz RF transceiver (SPI interface)
- **Input**:
  - Onboard pushbutton
  - 6 external pushbuttons for game control

### Software

- **Framework**: Mbed OS
- **Languages**: C++, C
- **Libraries**:
  - LCD_DISCO_F429ZI: Handles display functionality
  - DebouncedInterrupt: Manages input debouncing
  - nRF24L01P: Controls RF communication
  - mbed: Core microcontroller functions
- **Development Environment**: Keil Studio Cloud

## Game Modes

1. **AI vs AI**: Both paddles are controlled by AI algorithms. Triggered by pressing the onboard button.
2. **Human vs AI**: Bottom paddle is controlled by human player, top paddle by AI. Triggered by Button 1.
3. **Human vs Human (Local)**: Both paddles controlled by human players on the same device. Triggered by Button 2.
4. **Human vs Human (Wireless)**: Each player controls a paddle on their own device. Triggered by Button 3.

## Controls

### Player 1 (Top Paddle)
- Button 1: Move Left
- Button 3: Move Right
- Button 2: Pause/Resume

### Player 2 (Bottom Paddle)
- Button 4: Move Left
- Button 6: Move Right
- Button 5: Pause/Resume

### General Controls
- Onboard Button: Spawn new ball (during game) / Reset game (during pause) / Start AI vs AI mode (from menu)

## Wireless Communication Protocol

The project uses a master-slave architecture for wireless play:
- Master device manages game state and sends 32-byte packets containing ball positions, paddle positions, and scores
- Slave device receives game state and sends 1-byte packets containing paddle position
- Communication occurs via nRF24L01+ modules operating at 2.4GHz

## Setup and Configuration

1. Connect external buttons to the specified GPIO pins
2. Connect nRF24L01+ modules to the SPI interfaces
3. Set the `MASTER` define to 1 for master device or 0 for slave device
4. Adjust difficulty settings via `AI1_DIFFICULTY` and `AI2_DIFFICULTY` defines

## Building and Deployment

The project was developed using Keil Studio Cloud and can be compiled and deployed using the Mbed CLI or the Mbed Studio IDE.