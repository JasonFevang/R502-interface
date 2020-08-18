# ESP32 R502-Interface
The R502 is a fingerprint identification module, developed by GROW Technology.
The offical documentation can be downloaded here: https://www.dropbox.com/sh/epucei8lmoz7xpp/AAAmon04b1DiSOeh1q4nAhzAa?dl=0

It is also in the docs/ directory

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

## License
MIT License

## Possible Errors in R502 Documentation
The offical documentation as of when this library was written in in the docs/ directory, these are the important errors

"Data Package Length can be 32, 64, 128, 256 bytes"
    Not true, through experimentation, can only be set to 128 or 256 bytes based on what is returned in ReadSysPara
"Maximum value of the length paramter in a package is 256"
    Not true, can be up to 258, including the checksum