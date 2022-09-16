package main

import (
	"flag"
	"fmt"
	"go.bug.st/serial"
	"log"
	"os"
	"strconv"
	"strings"
	"time"
)

type SerDev interface {
	Read(buf []byte) (int, error)
	Write(buf []byte) (int, error)
	Close() error
}

var (
	_baud    = flag.Int("b", 57600, "Baud rate")
	_device  = flag.String("d", "", "Serial Device")
	_erase   = flag.Bool("erase", false, "erase logs")
	_ferase  = flag.Bool("erase-only", false, "only erase logs")
	_verbose = flag.Bool("verbose", false, "verbose")
)

func check_device() (string, int) {
	var baud int
	ss := strings.Split(*_device, "@")
	name := ss[0]
	if len(ss) > 1 {
		baud, _ = strconv.Atoi(ss[1])
	} else {
		baud = *_baud
	}
	if name == "" {
		for _, v := range []string{"/dev/ttyUSB0", "/dev/cuaU0"} {
			if _, err := os.Stat(v); err == nil {
				name = v
				baud = *_baud
				break
			}
		}
	}
	if name != "" {
		log.Printf("Using device %s\n", name)
	}
	return name, baud
}

func Readline(s SerDev) ([]byte, error) {
	b := make([]byte, 1)
	var arr []byte
	var n int
	var err error
	for {
		n, err = s.Read(b)
		if n == 1 && err == nil {
			if b[0] == 10 {
				break
			}
			arr = append(arr, b[0])
		} else {
			fmt.Println(err)
			break
		}
	}
	if *_verbose {
		fmt.Fprintf(os.Stderr, "Line:%s\n", string(arr))
	}
	return arr, err
}

func Readdata(s SerDev, n int) ([]byte, error) {
	b := make([]byte, 16)
	var arr []byte
	var err error
	nb := 0
	j := 0
	for {
		j, err = s.Read(b)
		arr = append(arr, b[0:j]...)
		nb += j
		if nb == n {
			break
		}
	}
	return arr, err
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage of go-wbt [options] [file]\n")
		flag.PrintDefaults()
	}
	flag.Parse()
	adata := flag.Args()
	var fname string

	if len(adata) == 0 {
		fname = fmt.Sprintf("trk_%s.gpx", time.Now().Format("20060102T150405"))
	} else {
		fname = adata[0]
	}

	var sd SerDev
	var err error

	name, baud := check_device()
	if name != "" {
		if len(name) == 17 && name[2] == ':' && name[8] == ':' && name[14] == ':' {
			sd = NewBT(name)
			time.Sleep(1 * time.Second)
			Readline(sd)
		} else {
			mode := &serial.Mode{BaudRate: baud}
			sd, err = serial.Open(name, mode)
			if err != nil {
				log.Fatal(err)
			}
			if strings.Contains(name, "rfcomm") {
				//	time.Sleep(2 * time.Second)
				Readline(sd)
			}
		}
		defer sd.Close()
	} else {
		os.Exit(127)
	}

	ok := false
	var buf []byte
	for j := 0; j < 5; j++ {
		sd.Write([]byte("@AL\r\n"))
		for i := 0; i < 10; i++ {
			buf, _ = Readline(sd)
			fmt.Println(string(buf))
			if strings.HasPrefix(string(buf), "@AL") {
				ok = true
				fmt.Printf("WBT201 login OK (%d)\n", i)
				break
			}
		}
		if ok {
			break
		}
	}

	if !ok {
		fmt.Println("WBT201 login fails")
		return
	}

	if *_ferase == false {
		end := 0
		start := 0

		for try := 0; try < 3; try += 1 {
			sd.Write([]byte("@AL,5,2\r\n"))
			buf, _ = Readline(sd)
			parts := strings.Split(string(buf), ",")
			if len(parts) == 4 {
				end, _ = strconv.Atoi(parts[3])
				break
			}
		}
		if end > 0 {
			sd.Write([]byte("@AL,5,1\r\n"))
			buf, _ = Readline(sd)
			parts := strings.Split(string(buf), ",")
			if len(parts) == 4 {
				start, _ = strconv.Atoi(parts[3])
			}
			fmt.Println("log = ", start, end)
			if end > 0 {
				var bindata []byte
				for {
					s := fmt.Sprintf("@AL,5,3,%d\r\n", start)
					sd.Write([]byte(s))
					sz := 4096
					if end < 4096 {
						sz = end
					}
					d, _ := Readdata(sd, sz)
					dl := len(d)
					s1, _ := Readline(sd)
					s2, _ := Readline(sd)
					fmt.Printf("read %d at %d (%s, %s)\n", dl, start, string(s1), string(s2))
					end -= dl
					start += dl
					bindata = append(bindata, d...)
					if end == 0 {
						break
					}
				}
				if len(bindata) > 0 {
					GPXgen(fname, bindata)
				}
			}
		} else {
			fmt.Println("No logs")
		}
	}
	if *_erase || *_ferase {
		fmt.Println("Erasing")
		sd.Write([]byte("@AL,5,6\r\n"))
		buf, _ = Readline(sd)
	}
}
