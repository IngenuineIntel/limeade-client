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

// CGO doesn't support variadic functions, so here's some wrappers
int LimeadeInitUDP(LimeadeContext *c, const char *host, int port)
{
  return limeade_init(c, LIMEADE_MODE_CLIENT_ETH|LIMEADE_MODE_LOW_COMPRESSION, host, port);
}

int LimeadeInitSSH(LimeadeContext *c, const char *host)
{
  return limeade_init(c, LIMEADE_MODE_CLIENT_SSH|LIMEADE_MODE_LOW_COMPRESSION, host);
}
