// config.c

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

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

static inline void init_config(struct prog_config *conf)
{
  memset(conf, 0, sizeof(*conf));
  conf->port            = -1;
  conf->event_send_ms_p = -1;
  conf->perf_send_ms_p  = -1;
  conf->proc_send_ms_p  = -1;
  conf->log_level       = -1;
}

#define CAST_INT(val_start, target)\
  do{\
  char *endptr;\
  int value = strtol(val_start, &endptr, 10);\
  if(endptr != val_start) target = value;\
  } while(0)

static inline void populate_from_conf(const char * const path, struct prog_config *config)
{
  char data[4097];
  char *cursor, *data_end;
  int in_limeade_section = 0;

  FILE *fd = fopen(path, "r");
  if(!fd)
    return;

  size_t data_size = fread(data, 1, sizeof(data) - 1, fd);
  fclose(fd);
  data[data_size] = '\0';
  cursor = data;
  data_end = data + data_size;

  while(cursor < data_end)
  {
    char *line_end = memchr(cursor, '\n', data_end - cursor);
    char *next_line = line_end ? line_end + 1 : data_end;
    if(!line_end)
      line_end = data_end;

    char *line = cursor;
    while(line < line_end && (*line == ' ' || *line == '\t' || *line == '\r'))
      line++;
    while(line_end > line
       && (line_end[-1] == ' ' || line_end[-1] == '\t' || line_end[-1] == '\r'))
      line_end--;
    *line_end = '\0';

    if(line < line_end && *line != '#' && *line != ';')
    {
      if(*line == '[')
      {
        in_limeade_section = strcmp(line, CONFIG_TARGET_SECTION) == 0;
      }
      else if(in_limeade_section)
      {
        char *equals = memchr(line, '=', line_end - line);
        if(equals)
        {
          char *key_end = equals;
          char *value_start = equals + 1;
          while(key_end > line
             && (key_end[-1] == ' ' || key_end[-1] == '\t'))
            key_end--;
          *key_end = '\0';
           while(value_start < line_end
             && (*value_start == ' ' || *value_start == '\t'))
            value_start++;

          if(strcmp(line, CONFIG_TARGET_NET_MODE) == 0)
          {
            for(int i = 1; i < NETWORK_MODE_MAX; i++)
            {
              if(strcmp(value_start, NETWORK_MODE_REPRS[i]) == 0)
              {
                config->net_mode = i;
                break;
              }
            }
          }
          else if(strcmp(line, CONFIG_TARGET_HOST) == 0)
            config->host = strdup(value_start);
          else if(strcmp(line, CONFIG_TARGET_PORT) == 0)
            CAST_INT(value_start, config->port);
          else if(strcmp(line, CONFIG_TARGET_INTERFACE) == 0)
            config->interface = strdup(value_start);
          else if(strcmp(line, CONFIG_TARGET_EVENT_SEND_MS_P) == 0)
            CAST_INT(value_start, config->event_send_ms_p);
          else if(strcmp(line, CONFIG_TARGET_PROC_SEND_MS_P) == 0)
            CAST_INT(value_start, config->proc_send_ms_p);
          else if(strcmp(line, CONFIG_TARGET_PERF_SEND_MS_P) == 0)
            CAST_INT(value_start, config->perf_send_ms_p);
          else if(strcmp(line, CONFIG_TARGET_LOG_LEVEL) == 0)
            CAST_INT(value_start, config->log_level);
          else if(strcmp(line, CONFIG_TARGET_LOGFILE) == 0)
            config->logfile = strdup(value_start);
        }
      }
    }
    cursor = next_line;
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
  /*** CLI argument parsing is not yet supported ***/
}

void populate_conf(struct prog_config *config, int argc, char **argv)
{
  /* populates configuration from filesystem, and, if fails, writes
   * default configuration to file system and populates the config as such
   */
  char path_buffer[64];

  *config = DEFAULT_CONFIG;

  snprintf(path_buffer, sizeof(path_buffer), GLOBAL_CONF_LOCATION, CONFIG_FILE);
  populate_from_conf(path_buffer, config);

  populate_from_argv(config, argc, argv);
}

