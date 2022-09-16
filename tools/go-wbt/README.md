## go-wbt201

Simple command line tool to extract GPS logs from a WBT-201 GPS.

* USB
* Bluetooth (Linux only)

## Usage

``` shell
$ go-wbt201 -help
Usage of go-wbt [options] [file]
  -b int
    	Baud rate (default 57600)
  -d string
    	Serial Device
  -erase
    	erase logs
  -erase-only
    	only erase logs
  -verbose
    	verbose
```

On Linux, `/dev/ttyUSB0` is probed, on FreeBSD `/dev/cuaU0`; otherwise the device must be specified. For Linux, BT devices may be given as the address or an RFCOM device node (`/dev/rfcommX`):

``` shell
$ go-wbt201 -d 00:0B:0D:87:13:A2 -erase-only
```

If the file name is not specified, it defaults the current time:

``` shell
trk_yyyymmddThhmmss.gpx
```

## Installation

Requires `golang` (Go language compiler).

``` shell
# to build
ninja
# to build and install to ~/.local/bin
ninja install
```
