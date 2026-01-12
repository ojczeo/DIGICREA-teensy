# DIGICREA MaxMSP Teensy Companion Patch (`teensy_2.maxpat`)

This is a Max/MSP patch designed to communicate with the DIGICREA Teensy hardware project. It handles MIDI communication and/or audio synthesis logic.

## Functionality
* **MIDI Interface:** Receives MIDI notes or control data (CC) from the Teensy device.
* **Control Logic:** May provide a UI to send configuration commands back to the Teensy (e.g., changing Arp patterns, sync settings).
* **Audio Generation:** (If applicable) Contains oscillators or samplers triggered by the Teensy.

## Requirements
* **Max 8** (or Cycling '74 Max Runtime).
* **Connected Teensy Device** running the `arp_4_en` firmware.

## How to Use
1.  **Connect Hardware:** Ensure your Teensy is connected via USB and recognized by your OS.
2.  **Open Patch:** Double-click `teensy_2.maxpat` to open it in Max.
3.  **Configure MIDI:**
    * Look for a `[midiin]` or `[ctlin]` object, or a dropdown menu labeled "MIDI Device".
    * Select the **Teensy MIDI** device from the list.
    * If the audio engine is off, click the **Audio On/Off** (e.g., `[ezdac~]`) speaker icon to enable sound.
4.  **Interaction:**
    * Use the on-screen controls to interact with the hardware.
    * If the patch expects serial data instead of MIDI, ensure the correct Serial Port (e.g., `usbmodem` or `COM3`) is selected in the generic serial object `[serial]`.

## Troubleshooting
* **No Sound/Data:** Go to **Options > MIDI Setup** in Max and ensure the Teensy input/output ports are checked.
* **Serial Port Issues:** If the patch uses `[serial]`, click the "print" message message attached to it to list available ports in the Max Console, then select the correct index.
