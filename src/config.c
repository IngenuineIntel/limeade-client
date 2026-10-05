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
  char data[1024];
  memset(data, 0, sizeof(data));
  int amt, idx = 0, rem = sizeof(data) - 1;

  CONF_ADD("[limeade]\n");
  CONF_ADD("netmode=%s\n",       NETWORK_MODE_REPRS[config->net_mode]);
  CONF_ADD("host=%s\n",          config->host);
  CONF_ADD("port=%i\n",          config->port);
  if(config->interface)
  CONF_ADD("interface=%s\n",     config->interface);
  CONF_ADD("event_send_hz=%i\n", config->event_send_hz);
  CONF_ADD("proc_send_hz=%i\n",  config->proc_send_hz);
  CONF_ADD("perf_send_hz=%i\n",  config->perf_send_hz);
  CONF_ADD("log_level=%i\n",     config->log_level);
  if(config->logfile)
  CONF_ADD("logfile=%s\n",       config->logfile);

  log_dbg2("generated config:");
  log_dbg2(data);

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
  log_dbg3("config opened");

  size_t ret = fwrite(data, 1, sizeof(data), fd);
  fclose(fd);

  if(ret <= 0)
  {
    log_dbgerr("failed to write into file");
    return -1;
  }
  log_dbg3("config serialization successful");
  return 0;

}

int deserialize_config(const struct prog_config *out)
{
  /* Reads config from configuration file
   * returns -1 on error
   */
  return 0;
}

void release_config(struct prog_config *config)
{
  free(config->host);
  free(config->interface);
}

void release_config_path(void)
{
  free(CONFIG_FILE_PATH);
}

#define LOAD_DEFAULT_CONFIG() serialize_config(&DEFAULT_CONFIG)

