# WPI ECE 2049 STM32 Adafruit Graphics Library

This repository contains the STM32 project for testing an STM32 port of the [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) and the [Adafruit ST7735](https://github.com/adafruit/Adafruit-ST7735-Library) libraries designed specifically for the WPI ECE 2049 course. This project is not an original project, rather it is an adaptation of these libraries as they are written and published online. The Adafruit GFX library is distributed with the BSD license while the Adafruit ST7735 library is distributed with the MIT license, both of which allow redistribution of their code within projects given the original copyright notice is included. As such, the original BSD license for the Adafruit GFX library is included in the [LICENSE](LICENSE) file while the original MIT license for the Adafruit ST7735 library is included in the [LICENSE_MIT](LICENSE_MIT) file.

## Installation Instructions

For installation instructions, see the [INSTALL.md](INSTALL.md) file.

When altering the implementation or documentation of the API functions, the following command should be run to regenerate the doxygen documentation. This will automatically be reflected in the online version of the documentation upon commit and push. This command should be run from the project root and requires Doxygen to be installed. The latest version can be found at this link: https://www.doxygen.nl/download.html

`doxygen ./Doxyfile`

## API Reference

The API reference manual for the exposed C to C++ API for the display functions can be found at this link: https://vernamlab.org/ECE2049-STM32-Adafruit-Graphics-Library/files.html, which is updated with every push to this repository. You can also read an offline version in the generated Doxygen documentation pdf, found at the [Docs/latex/refman.pdf](Docs/latex/refman.pdf) file. The documentation can also be viewed by cloning the repository and opening the [index.html](Docs/html/index.html) file in the Docs/html folder in a web browser for an offline, web-based view. Two different systems, and therefore APIs, are described in this documentation. Graphics functions are described in the Adafruit_ST7735_API.h file while button and backlight functions are described in the Adafruit_TFTShield18_API.h file.
