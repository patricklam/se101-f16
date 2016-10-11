# Example Projects
This directory contains a suite of example projects. You are welcome to take a look at the code and learn from it.

## Rock-Paper-Scissors (Project\_RPS)
Mini-game emulating rock-paper-scissors. You should peek at Wire\_Util.ino. It might inspire some useful ways to abstract away I2C communication, making it simpler to work with the chips/EEPROM on the Orbit Booster pack!

Components Used:
 - Accelerometer for shake detection.
 - OLED Screen and button for user interface.

## File Storage Project (Project\_Storage)
![Project Storage: Displaying file system usage.](../Images/Project-Storage.jpg)

This example project implements a simple flat-file storage system on the Tiva Board.

Components Used:
 - Device EEPROM, via the TivaWare DriverLib API.
 - Orbit Booster Pack OLED for Display.
 - Serial (UART) for receiving file operations.

Implemented Features:
 - Adding/Retrieving Files via Serial I/O.
 - Basic flat-file system functionality (Add/Remove/Read/Stat/Reset)
 - Use OLED Screen to show information about file system.

TODO:
 - (Maybe) use buttons to lock/unlock file system.
