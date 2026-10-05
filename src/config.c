// config.c

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "logging.h"

enum network_mode
{
  NETWORK_MODE_NEITHER = 0,
  NETWORK_MODE_UDP,
  NETWORK_MODE_SSH,
};

const char *NETWORK_MODE_REPRS[] = {
  "NEITHER",
  "UDP",
  "SSH",
};

struct prog_config
{
  enum network_mode net_mode;
  char *host;
  int port;
  char *interface;
  int event_send_hz;
  int proc_send_hz;
  int perf_send_hz;
  int log_level;
  char *logfile;
};

static char CONFIG_TARGET_SECTION[]       = "[limeade]";
static char CONFIG_TARGET_NET_MODE[]      = "netmode";
static char CONFIG_TARGET_HOST[]          = "host";
static char CONFIG_TARGET_PORT[]          = "port";
static char CONFIG_TARGET_INTERFACE[]     = "interface";
static char CONFIG_TARGET_EVENT_SEND_HZ[] = "event_send_hz";
static char CONFIG_TARGET_PROC_SEND_HZ[]  = "proc_send_hz";
static char CONFIG_TARGET_PERF_SEND_HZ[]  = "perf_send_hz";
static char CONFIG_TARGET_LOG_LEVEL[]     = "log_level";
static char CONFIG_TARGET_LOGFILE[]       = "logfile";

const struct prog_config DEFAULT_CONFIG = {
  .net_mode      = NETWORK_MODE_UDP,
  .host          = "127.0.0.1",
  .port          = 12046,
  .interface     = NULL,
  .event_send_hz = 4,
  .proc_send_hz  = 2,
  .perf_send_hz  = 1,
  .log_level     = 3,
  .logfile       = NULL,
};

#define CONFIG_FILE "limeade-client.ini"
#define CONFIG_PATH "%s/.config/limeade/"
static char *CONFIG_DIR;
static char *CONFIG_FILE_PATH;

static int populate_config_path(void)
{
  char data[128];
  memset(data, 0, sizeof(data));
  const char *home = getenv("HOME");
  if(!home)
    return -1;

  int amt = snprintf(data, sizeof(data), CONFIG_PATH, home);
  CONFIG_DIR = malloc(amt + 1);
  memcpy(CONFIG_DIR, data, amt);
  strncat(data, CONFIG_FILE, sizeof(CONFIG_FILE));
  amt += sizeof(CONFIG_FILE);
  CONFIG_FILE_PATH = malloc(amt + 1);
  memcpy(CONFIG_FILE_PATH, data, amt);

  return 0;
}

#define CONF_ADD(...)\
{\
  amt = snprintf(data+idx, rem, __VA_ARGS__);\
  if(amt <= 0) return -1;\
  idx += amt;\
  rem -= amt;\
}

int serialize_config(const struct prog_config *config)
{
  /* Writes config into configuration file
   * returns -1 on error
   */
  char data[256];
  int amt, idx = 0, rem = sizeof(data) - 1;

  if(config->net_mode < 1 || config->net_mode > 2)
  {
    log_dbgerr("invalid network mode supplied to config");
    return -1;
  }
  if(!config->host)
  {
    log_dbgerr("host not supplied to config");
    return -1;
  } else if(config->port < 0)
  {
    log_dbgerr("port not supplied to config");
    return -1;
  }

  CONF_ADD("%s\n", CONFIG_TARGET_SECTION);
  CONF_ADD("%s=%s\n", CONFIG_TARGET_NET_MODE, NETWORK_MODE_REPRS[config->net_mode]);
  CONF_ADD("%s=%s\n", CONFIG_TARGET_HOST, config->host);
  CONF_ADD("%s=%i\n", CONFIG_TARGET_PORT, config->port);
  if(config->interface)
  CONF_ADD("%s=%s\n", CONFIG_TARGET_INTERFACE, config->interface);
  if(config->event_send_hz > 0)
  CONF_ADD("%s=%i\n", CONFIG_TARGET_EVENT_SEND_HZ, config->event_send_hz);
  if(config->proc_send_hz > 0)
  CONF_ADD("%s=%i\n", CONFIG_TARGET_PROC_SEND_HZ, config->proc_send_hz);
  if(config->perf_send_hz > 0)
  CONF_ADD("%s=%i\n", CONFIG_TARGET_PERF_SEND_HZ, config->perf_send_hz);
  if(config->log_level > 0)
  CONF_ADD("%s= %i\n", CONFIG_TARGET_LOG_LEVEL, config->log_level);
  if(config->logfile)
  CONF_ADD("%s=%s\n", CONFIG_TARGET_LOGFILE, config->logfile);

  log_dbg2("generated config:\n%s", data);
  log_dbg3("generated config length: %i", strlen(data));

  if(!CONFIG_FILE_PATH)
  {
    log_dbg3("CONFIG_FILE_PATH hasn't been populated yet, doing so now");
    if(populate_config_path() != 0)
      return -1;
    log_dbg3(CONFIG_FILE_PATH);
  }

  FILE *fd = fopen(CONFIG_FILE_PATH, "w");
  if(!fd)
  {
    log_dbgerr("failed to open config");
    return -1;
  }

  size_t ret = fwrite(data, 1, idx, fd);
  fclose(fd);

  if(ret <= 0)
  {
    log_dbgerr("failed to write into file");
    return -1;
  }

  log_dbg3("config serialization successful");
  return 0;

}

int deserialize_config(struct prog_config *out)
{
  /* Reads config from configuration file
   * returns -1 on error
   */
  char data[512];

  void *line_start, *equals, *line_end, *val_start, *val_end;
  int amt_rem, line_len, val_len, have_started_section = 0, have_stopped_section = 0;

  out->net_mode  = NETWORK_MODE_NEITHER;
  out->host      = NULL;
  out->port      = -1;
  out->interface = NULL;
  out->event_send_hz = -1;
  out->proc_send_hz  = -1;
  out->perf_send_hz  = -1;
  out->log_level     = -1;
  out->logfile       = NULL;


  FILE *fd = fopen(CONFIG_FILE_PATH, "r");
  if(!fd)
  {
    log_dbgerr("failed to open config for reading @ %s", CONFIG_FILE_PATH);
    return -1;
  }

  amt_rem = fread(data, 1, sizeof(data), fd);
  fclose(fd);

  if(amt_rem <= 0)
  {
    log_dbgerr("failed to read config");
    return -1;
  }

  line_start = data;

  for(;;)
  {
    while(*(char*)line_start == ' ')
    {
      line_start++;
      amt_rem--;
    }

    line_end = memchr(line_start, '\n', amt_rem);
    if(!line_end)
      break;

    val_end = line_end;

    while(*(char*)(val_end - 1) == ' ')
      val_end--;
    val_end++;

    line_len = (line_end - line_start);

    if(memcmp(line_start, CONFIG_TARGET_SECTION, sizeof(CONFIG_TARGET_SECTION) - 1) == 0)
    {
      log_dbg3("found '%s'", CONFIG_TARGET_SECTION);
      have_started_section = 1;
    }
    else if(memcmp(line_start, "[", 1) == 0)
    {
      log_dbg3("found some other config section");
      have_stopped_section = 1;
    }

    if(have_started_section == 0)
      goto loop_cont;

    if(have_stopped_section)
      break;
    
    equals = memchr(line_start, '=', val_end - line_start);
    if(!equals)
      goto loop_cont;

    val_start = equals + 1;
    while(*(char*)val_start == ' ')
      val_start++;

    if(memcmp(line_start, CONFIG_TARGET_NET_MODE, sizeof(CONFIG_TARGET_NET_MODE) - 1) == 0)
    {
      for(int i = 0; i < 3; i++)
        if(memcmp(val_start, NETWORK_MODE_REPRS[i], strlen(NETWORK_MODE_REPRS[i])) == 0)
          out->net_mode = i;

      if(out->net_mode == NETWORK_MODE_NEITHER)
        log_dbg3("network mode ('%s') parsed as invalid", CONFIG_TARGET_NET_MODE);
      else
        log_dbg3("network mode ('%s') parsed as '%s'", CONFIG_TARGET_NET_MODE, NETWORK_MODE_REPRS[out->net_mode]);

    } else if(!out->host && memcmp(line_start, CONFIG_TARGET_HOST, sizeof(CONFIG_TARGET_HOST) - 1) == 0)
    {
      int sz = val_end - val_start - 1;
      out->host = malloc(sz + 1);
      memcpy(out->host, val_start, sz);
      out->host[sz] = '\0';
      log_dbg3("host ('%s') parsed as %s", CONFIG_TARGET_HOST, out->host);

    } else if(memcmp(line_start, CONFIG_TARGET_PORT, sizeof(CONFIG_TARGET_PORT) - 1) == 0)
    {
      char *endptr;
      long port = strtol(val_start, &endptr, 10);
      if(endptr == val_start)
        log_dbg3("port ('%s') value could not be parsed as integer");
      else
      {
        out->port = port;
        log_dbg3("port ('%s') parsed as %i", CONFIG_TARGET_PORT, port);
      }

    } else if(!out->interface && memcmp(line_start, CONFIG_TARGET_INTERFACE, sizeof(CONFIG_TARGET_INTERFACE) - 1) == 0)
    {
      int sz = val_end - val_start;
      out->interface = malloc(sz + 1);
      memcpy(out->interface, val_start, sz);
      out->host[sz] = '\0';
      log_dbg3("interface ('%s') parsed as %s", CONFIG_TARGET_INTERFACE, out->host);

    } else if(memcmp(line_start, CONFIG_TARGET_EVENT_SEND_HZ, sizeof(CONFIG_TARGET_EVENT_SEND_HZ) - 1) == 0)
    {
      char *endptr;
      long freq = strtol(val_start, &endptr, 10);
      if(endptr == val_start)
        log_dbg3("event send frequency ('%s') could not be parsed as integer");
      else
      {
        out->event_send_hz = freq;
        log_dbg3("event send frequency ('%s') parsed as %i", CONFIG_TARGET_EVENT_SEND_HZ, freq);
      }

    } else if(memcmp(line_start, CONFIG_TARGET_PROC_SEND_HZ, sizeof(CONFIG_TARGET_PROC_SEND_HZ) - 1) == 0)
    {
      char *endptr;
      long freq = strtol(val_start, &endptr, 10);
      if(endptr == val_start)
        log_dbg3("proc send frequency ('%s') could not be parsed as integer");
      else
      {
        out->proc_send_hz = freq;
        log_dbg3("proc send frequency ('%s') parsed as %i", CONFIG_TARGET_PROC_SEND_HZ, freq);
      }

    } else if(memcmp(line_start, CONFIG_TARGET_PERF_SEND_HZ, sizeof(CONFIG_TARGET_PERF_SEND_HZ) - 1) == 0)
    {
      char *endptr;
      long freq = strtol(val_start, &endptr, 10);
      if(endptr == val_start)
        log_dbg3("perf send frequency ('%s') could not be parsed as integer");
      else
      {
        out->perf_send_hz = freq;
        log_dbg3("perf send frequency ('%s') parsed as %i", CONFIG_TARGET_PERF_SEND_HZ, freq);
      }

    } else if(memcmp(line_start, CONFIG_TARGET_LOG_LEVEL, sizeof(CONFIG_TARGET_LOG_LEVEL) - 1) == 0)
    {
      char *endptr;
      long lvl = strtol(val_start, &endptr, 10);
      if(endptr == val_start)
        log_dbg3("log level ('%s') could not be parsed as interger");
      else
      {
        out->log_level = lvl;
        log_dbg3("log level ('%s') parsed as %i", CONFIG_TARGET_LOG_LEVEL, lvl);
      }

    } else if(!out->logfile && memcmp(line_start, CONFIG_TARGET_LOGFILE, sizeof(CONFIG_TARGET_LOGFILE) - 1) == 0)
    {
      int sz = val_end - val_start;
      out->logfile = malloc(sz + 1);
      memcpy(out->logfile, val_start, sz);
      out->logfile[sz] = '\0';
      log_dbg3("logfile ('%s') parsed as %s", CONFIG_TARGET_LOGFILE, out->logfile);
    } else
      log_dbg3("key=val line didn't mean anything");

    loop_cont:

    line_start = ++line_end;
    amt_rem -= line_len + 1;
  }
  // TODO check for needed values
  return 0;
}

#define CONF_SET_ATTR_DEFAULT(attr)\
{\
  config->attr = DEFAULT_CONFIG.attr;\
  ret++;\
}

void release_config(struct prog_config *config)
{
  free(config->host);
  free(config->interface);
}

void release_config_path(void)
{
  free(CONFIG_DIR);
  free(CONFIG_FILE_PATH);
}

#define SERIALIZE_DEFAULT_CONFIG() serialize_config(&DEFAULT_CONFIG)

int main()
{
  logging_init();
  set_log_level(4);
  SERIALIZE_DEFAULT_CONFIG();
  struct prog_config config;
  int ret =deserialize_config(&config);
  log_info("deserialize returned %i", ret);
  
}

