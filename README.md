# ESP32 R502-Interface
The R502 is a fingerprint identification module, developed by GROW Technology.
The offical documentation can be downloaded here: https://www.dropbox.com/sh/epucei8lmoz7xpp/AAAmon04b1DiSOeh1q4nAhzAa?dl=0

This is an **ESP-IDF** component developed for the **esp32** to interface with the R502 module via UART

## Documentation
The project is fully documented using Doxygen
doxygen.conf is the Doxygen configuration file for the project
Run `doxygen doxygen.conf` in the root directory to build the html/ directory, then open `html/index.html` in your browser of choice to see the documentation for the project

## Code Style
The source code is developed following the ESP-IDF Development Framework Style Guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html#espressif-iot-development-framework-style-guide

## Unit Tests
The `tests/` directory contains all unit tests for the project, using the Unity test framework provided by ESP-IDF. For examples on how to run the tests see the ESP-IDF unit test sample code: https://github.com/espressif/esp-idf/tree/master/examples/system/unit_test

## How to Use
* Create an instance of the R502Interface class
* Call init on the object to initialize UART hardware
* Send commands to the module using the R502 interface

## Contribute
Contact me over GitHub if you want to contribute to the project

-------------------------

## Development questions
    UART
        esp32 uart buffer size? How often should I read from uart?
        If I receive multiple responses, will I get all of them when I read
        from uart? When do I begin losing responses?
        Should I wait for the response after every sent command?
            Simplest, yes. I don't think it will damage performance

        How long does it take to respond? avg, max


Research
    If you send consecutive packets too quickly, it just won't respond to the 
    second packet


Tests

A component called R502, which has a number of associated tests
    Test simple response for a basic command
    Test sending multiple responses at a certain timeframe
    Test receiving data from multiple command responses at once
    Test fingerprint data retrevial and a bunch of other things

For BWL
    Write tests for all the components I've written
        Wifi-manager
        ota-manager

What about tests for an actual product, not a specific component?
    Like tests for the bulbs
    Or tests for crmx?
        crmx would be moved into a component

    I guess they want me to develop with lots of components


Seperate this into a component
Create a new higher level project that acts as a tester for the component
    That will literally just run it, and then execute the tests
