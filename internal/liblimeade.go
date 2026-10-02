// liblimeade.go
// compatibility for liblimeade C library

package main

/*
#cgo LDFLAGS: -lz -llimeade
#include "liblimeade_go_compat.h"
*/
import "C"
import (
	"errors"
	"unsafe"
)

type LimeadeContext *C.LimeadeContext
type LimeadePacketFlags *C.LimeadePacketFlags
type LimeadePacketData *C.LimeadePacketData
type LimeadeRecvd *C.LimeadeRecvd

func LimeadeError(i C.int) error {
	if i == C.LIMEADE_OK {
		return nil
	}
	return errors.New(C.GoString(C.LIMEADE_ERROR_REPRS[i]))
}

func LimeadeInit(c Config) (LimeadeContext, error) {
	var ret C.LimeadeContext
	var code C.int

	cstr_host := C.CString(c.host)
	if c.mode == "UDP" {
		code = C.LimeadeInitUDP(&ret, cstr_host, C.int(c.port))

	} else if c.mode == "SSH" {
		code = C.LimeadeInitSSH(&ret, cstr_host)

	} else {
		return &ret, errors.New("invalid configuration")
	}
	defer C.free(unsafe.Pointer(cstr_host))

	return &ret, LimeadeError(code)
}

func LimeadeConnect(ctx LimeadeContext) error {
	return LimeadeError(C.limeade_connect(ctx))
}

func LimeadeDestruct(ctx LimeadeContext) {
	C.limeade_destruct(ctx)
}

