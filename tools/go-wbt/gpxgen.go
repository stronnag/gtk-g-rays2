package main

import (
	"encoding/xml"
	gpx "github.com/twpayne/go-gpx"
	"os"
	"io"
	"fmt"
	"time"
	"encoding/binary"
)

func openStdoutOrFile(path string) (io.WriteCloser, error) {
	var err error
	var w io.WriteCloser

	if len(path) == 0 || path == "-" {
		w = os.Stdout
	} else {
		w, err = os.Create(path)
	}
	return w, err
}

func decode_time(t uint32) time.Time {
	s := int(t & 0x3f)
	mi := int((t >> 6) & 0x3f)
	h := int((t >> 12) & 0x1f)
	d := int((t >> 17) & 0x1f)
	mo := time.Month((t >> 22) & 0xf)
	yr := int(2000 + (t >> 26))
	return time.Date(yr, mo, d, h, mi, s, 0, time.UTC)
}

func GPXgen(filename string, bindata []byte) {
	var wp []*gpx.WptType
	j := 0
	k := 0
	var v int32
	g := &gpx.GPX{Version: "1.0", Creator: "go-wbt201-reader"}
	for {
		rec := bindata[k : k+16]
		typ := binary.LittleEndian.Uint16(rec[0:2])
		tbuf := binary.LittleEndian.Uint32(rec[2:6])
		v = int32(binary.LittleEndian.Uint32(rec[6:10]))
		lat := float64(v) / 1e7
		v = int32(binary.LittleEndian.Uint32(rec[10:14]))
		lon := float64(v) / 1e7
		alt := float64(int16(binary.LittleEndian.Uint16(rec[14:16])))
		ts := decode_time(tbuf)

		j += 1
		w0 := gpx.WptType{Lat: lat,
			Lon:  lon,
			Ele:  alt,
			Time: ts,
			Name: fmt.Sprintf("P%05d", j)}
		wp = append(wp, &w0)
		if k > 0 && ((typ & 1) == 1) {
			g.Trk = append(g.Trk,
				[]*gpx.TrkType{&gpx.TrkType{TrkSeg: []*gpx.TrkSegType{&gpx.TrkSegType{TrkPt: wp}}}}...)
			j = 0
			wp = nil
		}
		k += 16
		if k == len(bindata) {
			break
		}
	}
	if len(wp) > 0 {
		g.Trk = append(g.Trk,
			[]*gpx.TrkType{&gpx.TrkType{TrkSeg: []*gpx.TrkSegType{&gpx.TrkSegType{TrkPt: wp}}}}...)
	}

	gfh, err := openStdoutOrFile(filename)
	if err == nil {
		gfh.Write([]byte(xml.Header))
		g.WriteIndent(gfh, " ", " ")
		gfh.Write([]byte("\n"))
		gfh.Close()
	} else {
		fmt.Fprintf(os.Stderr, "gpx reader %s\n", err)
		os.Exit(-1)
	}
}
