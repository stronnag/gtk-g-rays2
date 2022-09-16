## g-rays2-cmd

Simple command line application to view/set WBT-201 GPS configuration.

```
$ ./g-rays2-cmd --help
Usage:
  g-rays2-cmd [OPTION...] - configure wbt-201 GPS

Help Options:
  -?, --help        Show help options

Application Options:
  -v, --verbose     be verbose
  -V, --version     version info
  -l, --list        show current
  -f, --from-file   get settings frrom file
  -n, --names       show settings names
  -s, --set         set data (name=val), repeat as necessary

```

e.g.

``` shell
$ g-rays2-cmd --set log_type=5 --set time_mode_int=60 \
              --set dist_mode_int=10  --list /dev/ttyUSB0
auto_sleep: 600
over_speed: 120
log_type: 5
low_speed_limit: 1
high_speed_limit: 2000
heading_mode_int: 5
low_speed: 20
middle_speed: 60
high_speed: 100
speed_mode_time: 5
low_middle_int: 10
middle_high_int: 15
high_high_int: 20
time_mode_int: 60
dist_mode_int: 10
```

To save / restore settings:

```
$ g-rays2-cmd --list /dev/rfcomm0 > mysettings.txt
...
$ g-rays2-cmd --from-file mysettings.txt [--list] /dev/rfcomm0
```

To install:

``` shell
$ make
$ cp /g-rays2-cmd /somewhere/on/the/path
```

Author: (c) 2007 Jonathan Hudson <jh+gps@daria.co.uk>

Licence: GPL V2 or later
