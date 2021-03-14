## Overview
This project use STM32H7 microcontroller that consists of 2 cores: CM7 and CM4. Multicore communication has been implemented and it allows sharing data between 2 cores.
In this project, CM7 core is responsible for executing all commands that come from CM4 core. The CM4 is responsible for communication via ethernet or interaction with the user. The CM4 core use FreeRTOS.

This project can act as 2 separated projects. It depends on the define flag: TCP_SERVER_ON.

* TCP_SERVE_ON 0 - First type of the project uses OLED display and buttons for the navigation in the menu. On the OLED display has been implemented a menu
that allows displaying temperature, humidity, and pressure. The parameters are measured by BME280 sensor. Users can also input a current height above sea level that
is used to calculate an absolute pressure. All of this is implemented on the CM4 core. 

* TCP_SERVE_ON 1 - Second type of the project where the TCP Client has been implemented on the CM4 core. The TCP task receives commands then it analyzes them and sends them to
CM7 core where they are executed. Then CM7 core response to CM4 with status or special data like temperature, humidity, or pressure. When CM4 gets a response from CM7, it sends
it to the user. In QT has been implemented a dedicated application that allows to connect with the device and print such parameters as temperature, humidity or pressure.

In this project are used also other commands that allow for example toggle a LEDs. The option with TCP communication simulates how user can wirelessly control lights in the house and get
information from the sensors. The part which uses a OLED display simulates how to implement a device that prints sensor information and also interacts with the user.

## Technical overview
* STM32H7 microcontroler
* Multicore Communication
* FreeRTOS
* Ethernet
* TCP Client
* OLED Display
* BME280
* C programming language
