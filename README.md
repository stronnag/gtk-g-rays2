## Overview

This is gtk-g-ray2, a tool to access the Wintec WBT-201 from free
operating systems.

gtk-g-gray2 v 2.0 and later is effectively Linux only as it uses `udev` for device discovery.

## Installation

For modern systems, with `$HOME/.local/bin` on `$PATH` (recommended):

```
# once, to create the build directory:
meson build --prefix=$HOME/.local --strip
# Build and install
ninja install -C build
```

Otherwise for a system install, set `--prefix=/usr` to install in `/usr/bin`, or omit to install in `/user/local/bin`.

```
# once, to create the build directory:
meson build --strip [--prefix=/usr]
# Build
ninja -C build
# Install as root into system directories
sudo ninja -C build install
```

By default, device names of `/dev/ttyUSB0` and "bluetooth" are offered. You can change the devices available by editing the file `$HOME/.config/g-rays2/g-rays2rc`. Note: gtk-rays-2 creates the file on first run if it does not exist.

e.g.

```
cat /home/jrh/.config/g-rays2/g-rays2rc
[g-rays2]
devices = /dev/ttyUSB0;/dev/rfcomm0
```

After the key `devices = ` there is a semi-colon (;) separated list of
possible device names.

e.g.

```
devices = /dev/ttyUSB0;/dev/ttyUSB1;/dev/ttyUSB2;/dev/rfcomm0;/dev/rfcomm1
```

Any device name of just "bluetooth" or containing the string "rfcomm" will be cause bluetooth discovery to (try to) find the real device (new in 2.00).

From version 0.99, the preferences dialogue also allows the user to set and modify the serial device list.

## Requirements

* gtk+-3
* libglade
* cairo
* udev

As cairo is a gtk+-3.0 dependency, there is no longer a 'non-cairo` option.


## Support

This is somewhat unsupported and offered for legacy / completeness / posterity; Github issues / pull requests will be considered.

(c) 2007-2022 Jonathan Hudson. GPL2 or later.
