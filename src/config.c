// config.c

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include "logging.h"

enum network_mode
{
  NETWORK_MODE_NEITHER = 0,
  NETWORK_MODE_UDP,
  NETWORK_MODE_SSH,
  NETWORK_MODE_MAX,
};

const char *NETWORK_MODE_REPRS[] = {
  "N/A",
  "UDP",
  "SSH",
};

struct prog_config
{
  enum network_mode net_mode;
  char *host;
  int port;
  char *interface;
  int event_send_ms_p;
  int perf_send_ms_p;
  int proc_send_ms_p;
  int log_level;
  char *logfile;
};

static char CONFIG_TARGET_SECTION[]         = "[limeade]";
static char CONFIG_TARGET_NET_MODE[]        = "netmode";
static char CONFIG_TARGET_HOST[]            = "host";
static char CONFIG_TARGET_PORT[]            = "port";
static char CONFIG_TARGET_INTERFACE[]       = "interface";
static char CONFIG_TARGET_EVENT_SEND_MS_P[] = "event_send_ms_p";
static char CONFIG_TARGET_PROC_SEND_MS_P[]  = "proc_send_ms_p";
static char CONFIG_TARGET_PERF_SEND_MS_P[]  = "perf_send_ms_p";
static char CONFIG_TARGET_LOG_LEVEL[]       = "log_level";
static char CONFIG_TARGET_LOGFILE[]         = "logfile";

const struct prog_config DEFAULT_CONFIG = {
  .net_mode        = NETWORK_MODE_UDP,
  .host            = "127.0.0.1",
  .port            = 12046,
  .interface       = NULL,
  .event_send_ms_p = 250,
  .proc_send_ms_p  = 1500,
  .perf_send_ms_p  = 1000,
  .log_level       = 3,
  .logfile         = NULL,
};

#define CONFIG_FILE "limeade-client.ini"
#define GLOBAL_CONF_LOCATION "/etc/limeade/%s"
#define USER_CONF_LOCATION   "%s/.config/%s"

static inline void init_config(struct prog_config *conf)
{
  memset(conf, 0, sizeof(*conf));
  conf->port            = -1;
  conf->event_send_ms_p = -1;
  conf->perf_send_ms_p  = -1;
  conf->proc_send_ms_p  = -1;
  conf->log_level       = -1;
}

#define SKIP_WHITESPACE_UP(cur, rem)\
  for(; *line_start != ' '; cur++, rem--)
#define SKIP_WHITESPACE_DOWN(end)\
  for(; *(end - 1) != ' '; end--);\
  end++
#define match_target(line, rem, target)\
  (sizeof(target) > rem\
  && memcmp(line, target, sizeof(target) - 1) == 0)

inline int match_target_str(const char * const line, int rem, const char * const target)
{
  int i = strlen(target);
  if(i > rem && memcmp(line, target, i) == 0)
    return 1;
  return 0;
}

#define CAST_INT(val_start, target)\
  do{\
  char *endptr;\
  int value = strtol(val_start, &endptr, 10);\
  if(endptr != val_start) target = value;\
  } while(0)

static inline void populate_from_conf(const char * const path, struct prog_config *config)
{
  /* populates configuration with a specific config file
   * returns 0 on success */
  char data[4096];
  char *line_start, *equals, *line_end, *val_start, *val_end;
  int amt_rem, line_len, val_len, have_started_section = 0, have_stopped_section = 0;

  FILE *fd = fopen(path, "r");
  if(!fd)
    return;

  amt_rem = fread(data, 1, sizeof(data), fd);
  fclose(fd);

  if(amt_rem < 0)
    return;

  data[amt_rem] = '\0';

  line_start = data;

  for(;;)
  {
    line_end = memchr(line_start, '\n', amt_rem);
    if(!line_end)
      break;

    *line_end = '\0';

    val_end = line_end;

    SKIP_WHITESPACE_UP(line_start, amt_rem);
    SKIP_WHITESPACE_DOWN(val_end);

    line_len = (line_end - line_start);

    if(match_target(line_start, amt_rem, CONFIG_TARGET_SECTION))
      have_started_section = 1;
    else if(*line_start == '[')
      have_stopped_section = 1;

    if(!have_started_section)
      goto loop_cont;
    if(have_stopped_section)
      break;

    equals = memchr(line_start, '=', val_end - line_start);
    if(!equals)
      goto loop_cont;

    val_start = equals + 1;
    val_len   = (val_end - val_start);

    SKIP_WHITESPACE_UP(val_start, val_len);
  
    if(match_target(line_start, amt_rem, CONFIG_TARGET_NET_MODE))
    {
      for(int i = 0; i < NETWORK_MODE_MAX; i++)
      {
        if(match_target_str(val_start, val_len, NETWORK_MODE_REPRS[i]))
        {
          if(i)
            config->net_mode = i;
          break;
        }
      }
    }

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_HOST))
      config->host = strdup(val_start);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_PORT))
      CAST_INT(val_start, config->port);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_INTERFACE))
      config->interface = strdup(val_start);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_EVENT_SEND_MS_P))
      CAST_INT(val_start, config->event_send_ms_p);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_PERF_SEND_MS_P))
      CAST_INT(val_start, config->perf_send_ms_p);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_PROC_SEND_MS_P))
      CAST_INT(val_start, config->proc_send_ms_p);

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_LOG_LEVEL))
      CAST_INT(val_start, config->log_level); 

    else if(match_target(line_start, amt_rem, CONFIG_TARGET_LOGFILE))
      config->logfile = strdup(val_start);

    loop_cont:

    line_start = line_end + 1;
    amt_rem   -= line_len + 1;
  }
}

static inline void populate_unfilled_conf(struct prog_config *c)
{
  if(!c->host)
    c->host = DEFAULT_CONFIG.host;
  if(c->port <= 0)
    c->port = DEFAULT_CONFIG.port;
  if(c->net_mode <= 0 || c->net_mode > NETWORK_MODE_MAX)
    c->net_mode = DEFAULT_CONFIG.net_mode;
  if(c->event_send_ms_p <= 0)
    c->event_send_ms_p = DEFAULT_CONFIG.event_send_ms_p;
  if(c->perf_send_ms_p <= 0)
    c->perf_send_ms_p = DEFAULT_CONFIG.perf_send_ms_p;
  if(c->proc_send_ms_p <= 0)
    c->proc_send_ms_p = DEFAULT_CONFIG.proc_send_ms_p;
  if(c->log_level < 0)
    c->log_level = DEFAULT_CONFIG.log_level;
  if(!c->logfile)
    c->logfile = DEFAULT_CONFIG.logfile;
}

static inline void populate_from_argv(struct prog_config *c, int argc, char **argv)
{
  log_crit("'populate_from_argv` not implemented");
}

void populate_conf(struct prog_config *config, int argc, char **argv)
{
  /* populates configuration from filesystem, and, if fails, writes
   * default configuration to file system and populates the config as such
   */
  char path_buffer[64];
  char *home = getenv("HOME");

  *config = DEFAULT_CONFIG;

  snprintf(path_buffer, sizeof(path_buffer), GLOBAL_CONF_LOCATION, CONFIG_FILE);
  populate_from_conf(path_buffer, config);

  if(home)
  {
    snprintf(path_buffer, sizeof(path_buffer), USER_CONF_LOCATION, home, CONFIG_FILE);
    populate_from_conf(path_buffer, config);
  }

  populate_from_argv(config, argc, argv);
}

