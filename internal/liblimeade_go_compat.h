// liblimeade_go_compat.h
// compatibility for CGO

#include<liblimeade/liblimeade.h>

// "could not determine what C.free refers to"
#include <stdlib.h>

// CGO cannot identify strucs without typedefs
typedef struct limeade_context LimeadeContext;
typedef struct limeade_packet_flags LimeadePacketFlags;
typedef struct limeade_packet_data LimeadePacketData;
typedef struct limeade_recvd LimeadeRecvd;

typedef struct limeade_knock LimeadeKnock;
typedef struct limeade_recognize LimeadeRecognize;
typedef struct limeade_intro LimeadeIntro;
typedef struct limeade_ack LimeadeAck;
typedef struct limeade_indiv_event LimeadeIndivEvent;
typedef struct limeade_events LimeadeEvents;
typedef struct limeade_indiv_proc LimeadeIndivProc;
typedef struct limeade_proc_generic LimeadeProcGeneric;
typedef struct limeade_proc_update LimeadeProcUpdate;
typedef struct limeade_perf LimeadePerf;
typedef struct limeade_commandeer LimeadeCommandeer;
typedef struct limeade_exited LimeadeExited;

// CGO doesn't support variadic functions, so here's some wrappers
// while this is janky in a C context, this works for Go. So...
int LimeadeInitUDP(LimeadeContext *c, const char *host, int port)
{
  return limeade_init(c, LIMEADE_MODE_CLIENT_ETH|LIMEADE_MODE_LOW_COMPRESSION, host, port);
}

int LimeadeInitSSH(LimeadeContext *c, const char *host)
{
  return limeade_init(c, LIMEADE_MODE_CLIENT_SSH|LIMEADE_MODE_LOW_COMPRESSION, host);
}

int LimeadeSendKnock(LimeadeContext *c, const LimeadeKnock d)
{
  return limeade_send(c, LIMEADE_PACKET_KNOCK, d);
}

int LimeadeSendAwaitKnock(LimeadeContext *c, const LimeadeKnock d)
{
  return limeade_send_await(c, LIMEADE_PACKET_KNOCK, d);
}

int LimeadeSendRecognize(LimeadeContext *c, const LimeadeRecognize d)
{
  return limeade_send(c, LIMEADE_PACKET_RECOGNIZE, d);
}

int LimeadeSendAwaitRecognize(LimeadeContext *c, const LimeadeRecognize d)
{
  return limeade_send_await(c, LIMEADE_PACKET_RECOGNIZE, d);
}

int LimeadeSendIntro(LimeadeContext *c, const LimeadeIntro d)
{
  return limeade_send(c, LIMEADE_PACKET_INTRO, d);
}

int LimeadeSendAwaitIntro(LimeadeContext *c, const LimeadeIntro d)
{
  return limeade_send_await(c, LIMEADE_PACKET_INTRO, d);
}

int LimeadeSendAck(LimeadeContext *c, const LimeadeAck d)
{
  return limeade_send(c, LIMEADE_PACKET_ACK, d);
}

int LimeadeSendAwaitAck(LimeadeContext *c, const LimeadeAck d)
{
  return limeade_send_await(c, LIMEADE_PACKET_ACK, d);
}

int LimeadeSendEvents(LimeadeContext *c, const LimeadeEvents d)
{
  return limeade_send(c, LIMEADE_PACKET_EVENTS, d);
}

int LimeadeSendAwaitEvents(LimeadeContext *c, const LimeadeEvents d)
{
  return limeade_send_await(c, LIMEADE_PACKET_EVENTS, d);
}

int LimeadeSendProcGeneric(LimeadeContext *c, const LimeadeProcGeneric d)
{
  return limeade_send(c, LIMEADE_PACKET_PROC_GENERIC, d);
}

int LimeadeSendAwaitProcGeneric(LimeadeContext *c, const LimeadeProcGeneric d)
{
  return limeade_send_await(c, LIMEADE_PACKET_PROC_GENERIC, d);
}

int LimeadeSendProcUpdate(LimeadeContext *c, const LimeadeProcUpdate d)
{
  return limeade_send(c, LIMEADE_PACKET_PROC_UPDATE, d);
}

int LimeadeSendAwaitProcUpdate(LimeadeContext *c, const LimeadeProcUpdate d)
{
  return limeade_send_await(c, LIMEADE_PACKET_PROC_UPDATE, d);
}

int LimeadeSendPerf(LimeadeContext *c, const LimeadePerf d)
{
  return limeade_send(c, LIMEADE_PACKET_PERF, d);
}

int LimeadeSendAwaitPerf(LimeadeContext *c, const LimeadePerf d)
{
  return limeade_send_await(c, LIMEADE_PACKET_PERF, d);
}

int LimeadeSendCommandeer(LimeadeContext *c, const LimeadeCommandeer d)
{
  return limeade_send(c, LIMEADE_PACKET_COMMANDEER, d);
}

int LimeadeSendAwaitCommandeer(LimeadeContext *c, const LimeadeCommandeer d)
{
  return limeade_send_await(c, LIMEADE_PACKET_COMMANDEER, d);
}

int LimeadeSendExited(LimeadeContext *c, const LimeadeExited d)
{
  return limeade_send(c, LIMEADE_PACKET_EXITED, d);
}

int LimeadeSendAwaitExited(LimeadeContext *c, const LimeadeExited d)
{
  return limeade_send_await(c, LIMEADE_PACKET_EXITED, d);
}

