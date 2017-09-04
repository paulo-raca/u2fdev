# u2fdev
Library for developing U2F devices

This library is meant to be portable across devices and protocols.

# Supported Protocols

- HID, via [hiddev library](https://github.com/paulo-raca/hiddev)

The library has been designed to also support Bluetooth and NFC, but those haven't been implemented yet.

# References

## Specifications

[The U2F specifications](https://fidoalliance.org/download/)

## Other implementations

I've used these other implementations as references to make mine:
- [teensy-u2f](https://github.com/yohanes/teensy-u2f) - Teensy LC U2F key implementation
- [v2f.py](https://github.com/concise/v2f.py) - A virtual U2F device implementation that lets you try, study, and hack U2F
- [stm32-u2f](https://github.com/avivgr/stm32-u2f) - A Universal 2nd Factor (U2F) USB token using STM32 MCUs
