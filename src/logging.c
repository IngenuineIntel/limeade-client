// logging.c

#include<stdbool.h>
#include<stdio.h>
#include<time.h>
#include<unistd.h>

static char ansi_blue[]   = "\033[34m";
static char ansi_cyan[]   = "\033[36m";
static char ansi_green[]  = "\033[32m";
static char ansi_red[]    = "\033[31m";
static char ansi_yellow[] = "\033[33m";
static char ansi_reset[]  = "\033[39m";

static char color_nop[] = "";

static char *log_blue, *log_cyan, *log_green, *log_red, *log_yellow, *log_reset;

static char logging_timestamp_buffer[40];

static bool logging_colors_enabled = false;
static FILE **logging_output = &stdout;

void logging_set_colors(bool choice)
{
  logging_colors_enabled = choice;
  if(choice)
  {
    log_blue   = ansi_blue;
    log_cyan   = ansi_cyan;
    log_green  = ansi_green;
    log_red    = ansi_red;
    log_yellow = ansi_yellow;
    log_reset  = ansi_reset;
  } else
  {
    log_blue = log_cyan = log_green = log_red = log_yellow = log_reset = color_nop;
  }
}

void set_logging_output(FILE **choice)
{
  if(!choice)
    logging_output = &stdout;
  else
    logging_output = choice;
}


char *logging_stdout_timestamp(void)
{
  time_t now = time(NULL);
  strftime(logging_timestamp_buffer, sizeof(logging_timestamp_buffer),
      "%H:%M:%S", localtime(&now));
  return logging_timestamp_buffer;
}

char *logging_file_timestamp(void)
{
  time_t now = time(NULL);
  strftime(logging_timestamp_buffer, sizeof(logging_timestamp_buffer),
      "%d-%m-%Y %H:%M:%S", localtime(&now));
  return logging_timestamp_buffer;
}


#define LOG_INNER(...)\
  fprintf(*logging_output, __VA_ARGS__);\

#define LOG_TIME()\
  (logging_colors_enabled ? logging_stdout_timestamp() : logging_file_timestamp())

void log_dbg(const char *msg)
{
  LOG_INNER("[%s%s%s][%sDEBUG%s]: %s\n", log_blue, LOG_TIME(), log_reset, log_cyan, log_reset, msg);
}

void log_info(const char *msg)
{
  LOG_INNER("[%s%s%s][%sINFO%s]:  %s\n", log_blue, LOG_TIME(), log_reset, log_green, log_reset, msg);
}

void log_warn(const char *msg)
{
  LOG_INNER("[%s%s%s][%sWARN%s]:  %s\n", log_blue, LOG_TIME(), log_reset, log_yellow, log_reset, msg);
}

void log_crit(const char *msg)
{
  LOG_INNER("[%s%s%s][%sCRIT%s]:  %s\n", log_blue, LOG_TIME(), log_reset, log_red, log_reset, msg);
}

void log_err(const char *msg)
{
  LOG_INNER("[%s%s%s][%sERROR%s]: %s\n", log_blue, LOG_TIME(), log_reset, log_red, log_reset, msg);
}

#define logging_init() logging_set_colors(true)

int main()
{
  logging_init();

  log_dbg("this is a debugging message!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

  logging_set_colors(false);

  log_dbg("this is a debugging message!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

  logging_set_colors(true);

  log_dbg("this is a debugging message!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

}

