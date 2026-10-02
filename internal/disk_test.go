// disk_test.go

package main

import (
	"testing"
	"strconv"
)

func TestLogger(t *testing.T) {
	l := NewLogger()
	l.SetColors(true)
	l.Dbg("A colorful debugging message")
	l.Info("A colorful informational message")
	l.Warn("A colorful warning message")
	l.Err("A colorful error message")
	l.SetColors(false)
	l.Dbg("A colorless debugging message")
	l.Info("A colorless informational message")
	l.Warn("A colorless warning message")
	l.Err("A colorless error message")
}

func TestSerializer(t *testing.T) {
	d := DefaultConfig()

	err := SerializeConfig(&d)
	if err != nil {
		t.Errorf("error: %v", err)
	}

	c, err := DeserializeConfig()
	if err != nil {
		t.Errorf("error: %v", err)
	}

	t.Logf("mode: %s -> %s", d.mode, c.mode)
	t.Logf("host: %s -> %s", d.host, c.host)
	t.Logf("port: %s -> %s", strconv.Itoa(d.port), strconv.Itoa(c.port))
}

