# Installation Instructions

To install and set up this library into a custom project, the following steps should be taken:

1. Download the latest release of the library files from the [Releases Page](https://github.com/WPI-ECE2049-Reimagined-MQP/ECE2049-STM32-Adafruit-Graphics-Library/releases) on the right, or create a clone of the repository using Git.

2. The following folders/files will need to be copied into your project. Please copy them into the same Core/Inc or Core/Src folder that appears in your STM32 project. Note that the file structure in the zipped release file is the same as it appears in the cloned repository for uniformity between any chosen method from Step 1:
    * Core/Inc/graphics (folder)
    * Core/Inc/dwt_module.h (file)
    * Core/Src/graphics (folder)
    * Core/Src/dwt_module.c (file)

3. After copying the files into their spots, open your project's .ioc file in STM32CubeMX program, or create one using the Board selector menu. This library is designed for use in the NUCLEO-H533RE development board without TrustZone activated. 

4. Ensure that the *__instruction cache__* (the *__ICACHE__* module) found in the "System Core/ICACHE" dropdown in the STM32CubeMX program is enabled and set to "2-ways set associative cache" for increased performance

5. Enable the *__I2C1__* peripheral. Note: you *__MUST__* enable the I2C1 peripheral since that peripheral connects to the associated clock and data line pins on the Arduino header pinouts, which is required by the Adafruit 1.8" TFT Display Shield.
    <ol type="a">
        <li>Expand the "Connectivity" dropdown menu in the STM32CubeMX program</li>
        <li>Click on the "I2C1" peripheral in that expanded drop down and set the "I2C" dropdown in the "Mode" panel to "I2C"</li>
        <li>In the "Configuration" panel just below the "Mode" panel, select the "NVIC Settings" pane and check the box for "I2C1 Event interrupt" and "I2C1 Error interrupt". This will enable interrupts for the peripheral that allow proper functionality</li>
        <li>In the same configuration panel, select the "GPIO Settings" pane and ensure that two pins are visible, "I2C1_SCL" and "I2C1_SDA" connected to GPIO pins PB6 and PB7, respectively. Click on each entry and set both GPIO pins to use a pull-up resistor and select "Very High" for the "Output Speed" for the pin. If you can't see the dropdown settings appear underneath the pins, you might have to scroll down within the panel or expand the amount of vertical space in the panel</li>
        <li>Enable GPDMA for sending and receiving data via I2C:
            <ol>
                <li>Expand the "System Core" dropdown and select the "GPDMA1" (General Purpose DMA 1) peripheral</li>
                <li>For Channel 6 in the "Mode" panel, select from the dropdown "Standard Request Mode"</li>
                <li>After selecting the mode from the dropdown, select the "CH6" pane in the "Configuration" panel below the "Mode" panel. </li>
                <li>Ensure that "Circular Mode" is disabled</li>
                <li>Set the dropdown for "Request" to "I2C1_TX". You might have to scroll through the options to find it. If the option is greyed out, then you must enable the I2C peripheral.
                <li>Set the dropdown for "Direction" to "Memory To Peripheral"</li>
                <li>Under the "Source Data Setting" section, enable "Source Address Increment After Transfer"</li>
                <li>For both the "Source Data Setting" and "Destination Data Setting" sections, ensure that "Data Width" is set to Byte, "Burst Length" is set to 1, and the "Allocated Port for Transfer" is set to Port 0</li>
                <li>Keep the default for all other settings</li>
                <li>For Channel 5 in the "Mode" panel, select from the dropdown "Standard Request Mode" to enable it</li>
                <li>Set the "Request" dropdown to "I2C1_RX" and set the "Direction" dropdown to "Peripheral to Memory"</li>
                <li>Set the "Destination Address Increment After Transfer" setting in the <b><em>Destination Data Setting</em></b> section to "Enabled", NOT the "Source Data Setting" section</li>
                <li>All other settings should be left to their default values</li>
            </ol>
        </li>
    </ol>
6. Enable the *__SPI2__* peripheral. While the number of the chosen SPI peripheral is not as important for this library, these steps operate on the assumption that the SPI2 peripheral is used. Note: If using the BSP driver for serial *printf()* support and on-board user button and LED control, the SPI1 peripheral will NOT be available to use.
    <ol type="a">
        <li>Expand the "Connectivity" dropdown and select the SPI2 peripheral</li>
        <li>Select from the "Mode" dropdown "Half-Duplex Master". Do not enable the "Hardware NSS" or the "Hardware RDY" signals. Full duplex is not required for this shield unless the SD card slot is enabled, which is currently not supported by this library port.</li>
        <li>Ensure that the "Frame Format" is set to "Motorola" and set the "Data Size" setting to "8 bits" with the "First Bit" setting set to "MSB First"</li>
        <li>Set the prescaler to 2. This will affect the baud rate shown below, which will be set to the correct value when the clock rates are adjusted in a later step</li>
        <li>Ensure that "Clock Polarity (CPOL)" is set to "Low" and that "Clock Phase (CPHA)" is set to "1 Edge"</li>
        <li>Keep the rest of the settings set to their defaults</li>
        <li>Select the "NVIC Settings" pane in the same configuration window and check the box for "SPI2 Global Interrupts"</li>
        <li>Select the "GPIO Settings" pane in the same configuration window and ensure that two pin entries appear named "SPI2_SCK" and "SPI2_MOSI". Ensure that no pullup or pulldown resistors are attached to the pins and set their "Maximum Output Speed" to "Very High"</li>
        <li>Enable GPDMA for SPI transmission</li>
        <ol>
            <li>Expand the "System Core" dropdown and select the GPDMA1 peripheral from the list</li>
            <li>For Channel 7 in the "Mode" panel, select from the dropdown "Standard Request Mode"</li>
            <li>Select the "CH7" pane in the "Configuration" panel below the "Mode" panel</li>
            <li>In the "Request" dropdown, select "SPI2_TX/I2S2_TX" to attach the SPI transmission interface to this channel</li>
            <li>Set the "Prority" to "High" and ensure that the "Direction" dropdown is set to "Memory to Peripheral"</li>
            <li>In the "Source Data Setting" section, set the "Source Address Incrememnt After Transfer" to "Enabled" and ensure that data width is set equal to "Byte"</li>
            <li>All other settings can remain set to their defaults</li>
        </ol>
    </ol>
7. Set up the SPI and I2C clocks.
    <ol type="a">
        <li>Open the "Clock Configuration" tab next to "Pinout & Configuration"</li>
        <li>A window should appear to run the automatic clock issues solver. Click yes. If the window does not show up, a button named "Resolve Clock Issues" should appear at the top of the screen below the tab button. Click that button.</li>
        <li>To make SPI run as fast as the display will support, the clock that corresponds to the SPI2 peripheral needs to be set such that the Baud rate of the peripheral is as close to 15 MHz as possible without creating an improper clock configuration. This is most feasibly achieved by setting the PLL1Q clock to ~30 MHz with the prescaler of 2 set earlier</li>
        <ol>
            <li>Find the PLL1 clock module on the left-hand side of the screen</li>
            <li>Set the box labeled "*N" equal to "X 45" from the dropdown menu. If the screen shows the resolve clock issues button again, don't press it</li>
            <li>Set the box labeled "/Q" equal to "/ 6" from the dropdown. This should resolve any clock issues and set the output of the PLL1Q clock to exactly 30 MHz</li>
            <li>Observe the Baud rate reported in the SPI2 peripheral settings screen to confirm that it is set exactly to 15 MHz</li>
        </ol>
    </ol>
8. Set up the SPI Chip Select and Data/Command select pins
    <ol type="a">
        <li>Using the graphical representation of the STM32H533RE chip, find GPIO pinouts PC7 and PC9 and set their function to be "GPIO_Output"</li>
        <li>Open the GPIO peripheral configuration page from the "System Core" dropdown and select the "GPIO" pane from within the "Configuration" panel</li>
        <li>For PC7, set the "Output Level" to "Low", do not assign a pull-up or pull-down resistor, set the "Maximum Output Speed" to "Very High", and give the pin the label: "DISP_DCX_SEL"</li>
        <li>For PC9, set the "Output Level" to "High", assign it a pull-up resistor, set the "Maximum Output Speed" to "Very High", and give the pin the label: "DISP_CS"</li>
    </ol>
9. In the "Project Manager" tab, set the "Toolchain/IDE" to "CMake" and the "Default Compiler/Linker" to "ST Arm Clang," and choose a location to save the project to if not done already. Then, generate code
10. Open the code in VSCode and import the project with the STM32CubeIDE extension.
11. Configure [CMakeLists.txt](CMakeLists.txt) to enable C++ for embedded development and compile the graphics library
    - Find the section labeled `# Setup compiler settings` and add the following lines after the lines calling `set(...)`

        ```
        set(CMAKE_CXX_STANDARD 17)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
        ```
        This sets the standards that CMake uses to compile C++, which is a requirement to use this library
    - Find the section labeled `# Core project settings` and change the line with `project(${CMAKE_PROJECT_NAME})` to `project(${CMAKE_PROJECT_NAME} C CXX ASM)` to allow C++ to be enabled within the project
    - Find the section labeled `# Enable CMake support for ASM and C languages` and change the `enable_language(...)` line to `enable_language(C CXX ASM)` to enable C++
    - In between the sections labeled `# Create an executable object type` and `# Add STM32CubeMX generated sources`, add the following lines to enable C++ linker support:
        ```
        # CUSTOM: Enable C++ linkage support
        set_target_properties(
            ${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX
        )
        ```
    - In between the sections labeled `# Add STM32CubeMX generated sources` and `# Link directories setup`, add the following lines to correctly set up C++ for embedded development:
        ```
        # CUSTOM: C++ compile options setup
        target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-use-cxa-atexit>
        )
        ```
    - Add the following lines in the `target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)` call under the section labeled `# Add sources to executable` to compile the library files:
        ```
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_GFX.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_SPITFT.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_ST77xx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_ST7735.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_ST7735_API.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Print.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_seesaw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_TFTShield18.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/graphics/Adafruit_TFTShield18_API.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/dwt_module.c
        ```
    - Finally, add the following line in the `target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ...)` section with the label `# Add include paths` to allow direct inclusion of library files:
        ```
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/graphics
        ```
12. Open the [Core/Src/STM32h5xx_it.c](Core/Src/stm32h5xx_it.c) file. Add the following external variable declaration: `extern volatile bool spiTxDone;` just after the `/* USER CODE BEGIN PV */` line at the top of the file. This imports a global variable used by the library to track when SPI transactions are completed. Additioanlly, copy the following code after the `/* USER CODE BEGIN 1 */` section at the very end of the file:

    ```
    void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
        if (hspi->Instance == SPI2) {
            // Handle the completion of SPI transmission here
            // For example, you can set a flag or call another function
            spiTxDone = true;
        }
    }
    ```

    Note: If you have already set up the HAL_SPI_TxCpltCallback function for another SPI peripheral, you only need to add the if statement in the code above to that function:

    ```
    if (hspi->Instance == SPI2) {
        // Handle the completion of SPI transmission here
        // For example, you can set a flag or call another function
        spiTxDone = true;
    }
    ```
13. Initialize the device in [Core/Src/main.c](Core/Src/main.c) with the following function calls *in order*:
    - Call `DWT_Init();` to initialize the more accurate timer system
    - Call `TFTShield18_create(&hi2c1);` to create the Adafruit Seesaw device
    - Call `TFTShield18_begin();` to initialize the Seesaw chip
    - Call `TFTShield18_tftReset(true);` to reset the board and prepare the ST7735 chip for initialization
    - Call `DWT_delayMs(100);` to allow the chip initialization and reset to complete. You will need to import the [Core/Inc/dwt_module.h](Core/Inc/dwt_module.h) file.
    - Call `Display_create(&hspi2, DISP_CS_Pin, DISP_CS_GPIO_Port, DISP_DCX_SEL_Pin, DISP_DCX_SEL_GPIO_Port);` to create the ST7735 driver instance on the NUCLEO board
    - Call `Display_init(INITR_BLACKTAB);` to initialize the ST7735 chip
    - Finally, call `DWT_delayMs(100);` one more time to ensure that initialization completes

If all of these steps are followed correctly and in order, the project should compile just fine! If not, check to make sure that the steps were followed completely, especially in the CMakeLists.txt setup step and the hardware configuration via the STM32CubeMX program. 