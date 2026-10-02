// disk.go

package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"slices"
	"strconv"
	"strings"
	"time"
)

type Logger struct {
	/* Logging system */
	colors bool
	blue string
	green string
	cyan string
	red string
	yellow string
	reset string
	timeFormat string
}

func (l *Logger) SetColors(s bool) {
	/* Toggles between stdout mode & logfile mode
	 * true for stdout, which enables colors & less verbose timestamps
	 * false for logfiles, which disables colors & make timestamps verbose
	 */
	if s == true {
		l.colors = true
		l.blue   = "\033[34m"
		l.cyan   = "\033[36m"
		l.green  = "\033[32m"
		l.red    = "\033[31m"
		l.yellow = "\033[33m"
		l.reset  = "\033[39m"
		l.timeFormat = "15:04:05"
	} else {
		l.colors = false
		l.blue, l.cyan, l.green = "", "", ""
		l.red, l.yellow, l.reset = "", "", ""
		l.timeFormat = "01/02/06 15:04:05"
	}
}

func (l *Logger) GetTime() string {
	/* Creates formatted string for log entries */
	now := time.Now()
	return now.Format(l.timeFormat)
}

func NewLogger() *Logger {
	/* Returns a Logger object
	 * in stdout mode by default */
	var l Logger

	l.SetColors(true)

	return &l
}

func (l *Logger) Dbg(msg string) {
	/* [03:55][DEBUG]: A debugging message. */
	fmt.Printf(
		"[%s%s%s][%sDEBUG%s]: %s\n", l.blue, l.GetTime(),
		l.reset, l.cyan, l.reset, msg,
	);
}

func (l *Logger) Info(msg string) {
	/* [03:56][INFO]:  An informational message. */
	fmt.Printf(
		"[%s%s%s][%sINFO%s]:  %s\n", l.blue, l.GetTime(),
		l.reset, l.green, l.reset, msg,
	);
}

func (l *Logger) Warn(msg string) {
	/* [03:56][WARN]:  A warning message. */
	fmt.Printf(
		"[%s%s%s][%sWARN%s]:  %s\n", l.blue, l.GetTime(),
		l.reset, l.yellow, l.reset, msg,
	);
}

func (l *Logger) Err(msg string) {
	/* [03:57][ERROR]: An error message. */
	fmt.Printf(
		"[%s%s%s][%sERROR%s]: %s\n", l.blue, l.GetTime(),
		l.reset, l.red, l.reset, msg,
	);
}

type Config struct {
	/* INI configurations for the program */
	mode string
	host string
	port int 
}

func DefaultConfig() Config {
	/* Returns default Config */
	ret := Config {
		mode: "UDP",
		host: "127.0.0.1",
		port: 12046,
	}
	return ret
}

// where the conffile belongs
const ConfigDir  = "%s/.config/limeade/"
const ConfigFile = "limeade.ini"

func SerializeConfig(c *Config) error {
	/* Commits Config to INI conffile */

	lines := []string{}

	lines = append(lines, "[limeade]")
	lines = append(lines, fmt.Sprintf("mode=%s", c.mode))
	lines = append(lines, fmt.Sprintf("host=%s", c.host))
	lines = append(lines, fmt.Sprintf("port=%s", strconv.Itoa(c.port)))
	// NOTE
	// all new config fields go here to be added

	home := os.Getenv("HOME")
	if home == "" {
		return errors.New("Couldn't find $HOME")
	}

	dir := fmt.Sprintf(ConfigDir, home)
	err := os.Mkdir(dir, os.ModePerm)

	path := filepath.Join(dir, ConfigFile)
	f, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY, 0640)

	if err != nil {
		return err
	}

	for _, line := range lines {
		fmt.Fprintln(f, line)
	}
	
	defer f.Close()
	return nil
}

func DeserializeConfig() (Config, error) {
	/* Reads Config from INI conffile */
	var ret Config

	ret.mode = ""
	ret.host = ""
	ret.port = -1

	home := os.Getenv("HOME")
	if home == "" {
		return ret, errors.New("Couldn't find $HOME")
	}

	dir := fmt.Sprintf(ConfigDir, home)
	path := filepath.Join(dir, ConfigFile)
	data, err := os.ReadFile(path)
	if err != nil {
		return ret, err
	}

	lines := strings.Split(string(data), "\n")

	for idx, val := range lines {
		lines[idx] = strings.Split(val, "#")[0]
	}

	i := slices.Index(lines, "[limeade]")
	lines = lines[i+1:]
	
	for _, line := range lines {

		if len(line) == 0 {
			continue
		}

		if line[0] == '[' { // next section
			break
		}

		sp := strings.Split(line, "=")
		if len(sp) != 2 {
			continue
		}

		// NOTE
		// all new config fields go here to be parsed
		switch sp[0] {
		case "mode":
			if ret.mode == "" {
				ret.mode = sp[1]
			}
		case "host":
			if ret.host == "" {
				ret.host = sp[1]
			}
		case "port":
			if ret.port == -1 {
				n, err := strconv.Atoi(sp[1])
				if err == nil {
					ret.port = n
				}
			}
		}
	}

	if ret.mode == "" || (ret.mode != "UDP" && ret.mode != "SSH") ||
	   ret.host == "" || (ret.mode == "UDP" && ret.port == 0) {
		var e = errors.New("not all configurations fufilled")
		return ret, e
	}

	return ret, nil
}

