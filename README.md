VfyPwd
Command
    header
        0  0xEF
        1  0x01
    ADDER
        2  0xFF
        3  0xFF
        4  0xFF
        5  0xFF
    PID
        6  0x01
    LENGTH
        7  0x00
        8  0x07
    DATA
        Instruction Code
        9  0x13
        Password
        10 0x00
        11 0x00
        12 0x00
        13 0x00
    SUM
        14 0x00
        15 0x1B 

    sum = 0x01+0x07+0x13=0x1B

Response
    Header
        0 0xEF
        1 0x01
    ADDER
        2 0xFF
        3 0xFF
        4 0xFF
        5 0xFF
    PID
        6 0x07
    LENGTH
        7 0x00
        8 0x03
    DATA
        Confirmation Code
        9 0x00
    SUM
        10 0x00
        11 0x0A

    sum = 0x07 + 0x03 = 0x0A

Development questions
    UART
        esp32 uart buffer size? How often should I read from uart?
        If I receive multiple responses, will I get all of them when I read
        from uart? When do I begin losing responses?

        How long does it take to respond? avg, max


Research
    If you send consecutive packets too quickly, it just won't respond to the 
    second packet


-------------------------
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
