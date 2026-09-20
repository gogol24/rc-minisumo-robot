# RC Minisumo Robot

Remote-controlled minisumo robot with differential drive, built from discrete components on perfboard (August 2025). [Demo](https://www.youtube.com/watch?v=LlYq2QDrr5E)

**Joystick → Arduino Nano → nRF24L01+ (2.4 GHz) → ESP32-S3 → TB6612FNG → 2 DC motors**

**Radio packet.** `int` is 2 bytes on the AVR but 4 on the Xtensa, and the ESP32 pads structs for alignment. Both sides use a packed struct of fixed-width types, with a `static_assert` on its size.

**Debugging.** One motor stalled when driven in one direction. A firmware offset reduced the symptom, but the ESP32's LEDs flickered in sync with that motor and separating the components improved it, pointing to noise coupling into the microcontroller rather than a firmware fault. Next revision: decoupling capacitors and separate supply rails.

**History.** The August 13, 2025 commit ran on the robot. Later commits are cleanup and fixes, untested on hardware.
