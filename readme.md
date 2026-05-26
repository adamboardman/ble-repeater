A simple BLE packet repeater
============================

The repeater will use these UUID's:

```
PRIMARY_SERVICE, 298cfeca-a10d-49ee-8a74-e513547f7ef7
CHARACTERISTIC, a8d99167-e58c-4a0c-9565-e2f1a7fbc05d, READ | WRITE | DYNAMIC,
```

The idea is that you set these just inside the BLE range within your building, to allow devices using the above UUIDs to be used when there
are not enough devices about to join everyone together.

They should be generally sleeping, only waking up when there are messages to send along. This means that the heartbeat
led blinking only works when woken up by there being messages to deliver. To tell if a repeater node is functioning you
can drag a piece of metal across the bottom quarter of one side to join pins 23(GND) & 24(GP18) which will fire off a
message into the network 'Sill Alive', and also cause some blinking of the LED. Avoid the top half as you'd be shorting
the power and the exit to USB for update flashing which can be done by carefully pulling pin 34(GP28) to ground.

Building
========

The Pico_w is perfectly sufficient, and as the cheaper device it would seem more likely to be used dotted about the
place as repeaters. 

Command Line
------------

To build on the command line for the Pico_w you could type something along the lines of:

```
ble-repeater$ mkdir build
ble-repeater$ cd build
ble-repeater/build$ cmake .. -DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
ble-repeater/build$ make
```

Or for a Pico2_w:

```
ble-repeater$ mkdir build2350
ble-repeater$ cd build2350
ble-repeater/build2350$ cmake .. -DPICO_PLATFORM=rp2350-arm-s -DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
ble-repeater/build2350$ make
```

The Pico2_w can be made to work, but it requires a patch to libdeflate to stop it trying to use neon:
```
--- a/lib/arm/cpu_features.h
+++ b/lib/arm/cpu_features.h
@@ -79,7 +79,7 @@ static inline u32 get_arm_cpu_features(void) { return 0; }
* r226563 for arm64), hardware floating point support is sufficient.
  */
  #if (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)) && \
-       (HAVE_NEON_NATIVE || (GCC_PREREQ(6, 1) && defined(__ARM_FP)))
+       (HAVE_NEON_NATIVE || (GCC_PREREQ(6, 1) && defined(__ARM_FP))) && defined(ARCH_ARM64)
#  define HAVE_NEON_INTRIN     1
#  include <arm_neon.h>
#else
```

There is probably a better fix but this at least lets you build the project.


CLion
-----

If using CLion you'll need to set 'CMake options' to something along the lines of the following:

```
-DPICO_SDK_PATH=~/pico-pi/pico-sdk -DPICOTOOL_FETCH_FROM_GIT_PATH=~/pico-pi/picotool
```

For step through debugging 'Edit configurations...' to then add '+' and pick 'Open OCD Download & Run', select your
target 'ble_repeater', Executable binary should be 'ble_repeater' or 'ble_repeater.elf' you may need to
select this directly from the cmake-build-debug directory, bundled GDB is fine, Board config file should be a new file
you'll probably need to create in your local copy of the raspberry pi openocd 'openocd/tcl/board/pico.cfg'. I set
Download to 'Always', the rest of the defaults were fine.

I created a new openocd/tcl/board/pico.cfg containing:

```
# SPDX-License-Identifier: GPL-2.0-or-later
# Attempt to make clion happy

source [find interface/cmsis-dap.cfg]
adapter speed 5000

set CHIPNAME rp2040
source [find target/rp2040.cfg]
```

I also had to set the openocd to my locally built version, 'Settings' -> 'Build, Execution, Deployment' -> 'Embedded
Development' -> Open OCD location: '/usr/local/bin/openocd'. Though it populated itself with that value once CLion
noticed it was missing.

Debugging
---------

When running attached to a linux computer via the USB port you can view what is going on with:

```
sudo minicom -D /dev/ttyACM0 -b 115200
```

or

```
screen -L /dev/ttyACM0 115200
```

Bluetooth debugging
-------------------

Useful tools for debugging BTLE things from a linux desktop:

Interactive tool:

```
bluetoothctl 
```

Commands such as 'scan on', 'connect XX:XX:XX:XX:XX:XX' can be useful to report info about a specific device. For
example both my android devices merrily report their names, even if their ble is using a random address.

Scan for BTLE devices:

```
sudo hcitool lescan
```

Interactively talk to BTLE device:

```
gatttool -b  XX:...:XX -I
[XX:...:XX][LE]> connect
[XX:...:XX][LE]> primary
[XX:...:XX][LE]> char-desc
```

* primary - lists UIDs supported
* char-desc lists handles and UIDs

