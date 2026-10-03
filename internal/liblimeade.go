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

type LimeadeKnock *C.LimeadeKnock
type LimeadeRecognize *C.LimeadeRecognize
type LimeadeIntro *C.LimeadeIntro
type LimeadeAck *C.LimeadeAck
type LimeadeIndivEvent *C.LimeadeIndivEvent
type LimeadeEvents *C.LimeadeEvents
type LimeadeIndivProc *C.LimeadeIndivProc
type LimeadeProcGeneric *C.LimeadeProcGeneric
type LimeadeProcUpdate *C.LimeadeProcUpdate
type LimeadePerf *C.LimeadePerf
type LimeadeCommandeer *C.LimeadeCommandeer
type LimeadeExited *C.LimeadeExited

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

// This, unfortunately, is the way it must be
func LimeadeSendKnock(ctx LimeadeContext, d LimeadeKnock) error {
	return LimeadeError(C.LimeadeSendKnock(ctx, *d))
}

func LimeadeSendAwaitKnock(ctx LimeadeContext, d LimeadeKnock) error {
	return LimeadeError(C.LimeadeSendAwaitKnock(ctx, *d))
}

func LimeadeSendRecognize(ctx LimeadeContext, d LimeadeRecognize) error {
	return LimeadeError(C.LimeadeSendRecognize(ctx, *d))
}

func LimeadeSendAwaitRecognize(ctx LimeadeContext, d LimeadeRecognize) error {
	return LimeadeError(C.LimeadeSendAwaitRecognize(ctx, *d))
}

func LimeadeSendIntro(ctx LimeadeContext, d LimeadeIntro) error {
	return LimeadeError(C.LimeadeSendIntro(ctx, *d))
}

func LimeadeSendAwaitIntro(ctx LimeadeContext, d LimeadeIntro) error {
	return LimeadeError(C.LimeadeSendAwaitIntro(ctx, *d))
}

func LimeadeSendEvents(ctx LimeadeContext, d LimeadeEvents) error {
	return LimeadeError(C.LimeadeSendEvents(ctx, *d))
}

func LimeadeSendAwaitEvents(ctx LimeadeContext, d LimeadeEvents) error {
	return LimeadeError(C.LimeadeSendAwaitEvents(ctx, *d))
}

func LimeadeSendProcGeneric(ctx LimeadeContext, d LimeadeProcGeneric) error {
	return LimeadeError(C.LimeadeSendProcGeneric(ctx, *d))
}

func LimeadesendAwaitProcGeneric(ctx LimeadeContext, d LimeadeProcGeneric) error {
	return LimeadeError(C.LimeadeSendAwaitProcGeneric(ctx, *d))
}

func LimeadeSendProcUpdate(ctx LimeadeContext, d LimeadeProcUpdate) error {
	return LimeadeError(C.LimeadeSendProcUpdate(ctx, *d))
}

func LimeadeSendAwaitProcUpdate(ctx LimeadeContext, d LimeadeProcUpdate) error {
	return LimeadeError(C.LimeadeSendAwaitProcUpdate(ctx, *d))
}

func LimeadeSendPerf(ctx LimeadeContext, d LimeadePerf) error {
	return LimeadeError(C.LimeadeSendPerf(ctx, *d))
}

func LimeadeSendAwaitPerf(ctx LimeadeContext, d LimeadePerf) error {
	return LimeadeError(C.LimeadeSendAwaitPerf(ctx, *d))
}

func LimeadeSendCommandeer(ctx LimeadeContext, d LimeadeCommandeer) error {
	return LimeadeError(C.LimeadeSendCommandeer(ctx, *d))
}

func LimeadeSendAwaitCommandeer(ctx LimeadeContext, d LimeadeCommandeer) error {
	return LimeadeError(C.LimeadeSendAwaitCommandeer(ctx, *d))
}

func LimeadeSendExited(ctx LimeadeContext, d LimeadeExited) error {
	return LimeadeError(C.LimeadeSendExited(ctx, *d))
}

func LimeadeSendAwaitExited(ctx LimeadeContext, d LimeadeExited) error {
	return LimeadeError(C.LimeadeSendAwaitExited(ctx, *d))
}
