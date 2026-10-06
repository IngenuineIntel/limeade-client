// logging.c
// manager for all logging

#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<time.h>
#include<unistd.h>

#include "logging.h"

// standard ANSI colors
static char ANSI_BLUE[]   = "\033[34m";
static char ANSI_CYAN[]   = "\033[36m";
static char ANSI_GREEN[]  = "\033[32m";
static char ANSI_RED[]    = "\033[31m";
static char ANSI_YELLOW[] = "\033[33m";
static char ANSI_RESET[]  = "\033[39m";

// nothing string to assign colors to when colors are disabled
static char color_nop[] = "";

static char *log_blue, *log_cyan, *log_green, *log_red, *log_yellow, *log_reset = color_nop;

// formatted timestamps are put in here to avoid repeated heap allocations
static char logging_timestamp_buffer[40];

// log levels:
//
// level 0: no logging
// level 1: info   - error
// level 2: debug  - error
// level 3: debug2 - error
// level 4: debug3 - error
//
// rules about these logging levels
//
// CRITICAL is an error you can't recover from
// INFO is general knowledge about the program's actions
// DEBUG is menial status stuff (like a 200 status code)
// DEBUG2 is really menial stuff like how large outgoing packets are
// DEBUG3 is like the little printouts you write when you're looking for a segfault
// ideally, in the event of a segfault, it should be able to be located solely
// by DEBUG3 logs
//
// only functions called by main can call CRITICAL, the functions they call can
// run INFO & ERROR, but any functions they call can only run DEBUGS
//
static int  log_level = 3;
static bool logging_colors_enabled = false;
static FILE **logging_output = &stdout;

void set_log_level(int level)
{
  log_level = level;
}

void logging_set_colors(bool choice)
{
  logging_colors_enabled = choice;
  if(choice)
  {
    log_blue   = ANSI_BLUE;
    log_cyan   = ANSI_CYAN;
    log_green  = ANSI_GREEN;
    log_red    = ANSI_RED;
    log_yellow = ANSI_YELLOW;
    log_reset  = ANSI_RESET;
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

#define LOG_INNER(lvl, msg, ...)         \
if(log_level >= lvl)                     \
{                                        \
  va_list arg;                           \
  va_start(arg, msg);                    \
  fprintf(*logging_output, __VA_ARGS__); \
  vfprintf(*logging_output, msg, arg);   \
  fprintf(*logging_output, "\n");        \
}

#define LOG_TIME()\
  (logging_colors_enabled ? logging_stdout_timestamp() : logging_file_timestamp())

void log_dbg(const char *msg, ...)
{
  LOG_INNER(2, msg, "[%s%s%s][%sDBG%s]: ", log_blue, LOG_TIME(), log_reset, log_cyan, log_reset);
}

void log_dbg2(const char *msg, ...)
{
  LOG_INNER(3, msg, "[%s%s%s][%sDBG2%s]: ", log_blue, LOG_TIME(), log_reset, log_cyan, log_reset);
}

void log_dbgerr(const char *msg, ...)
{
  LOG_INNER(4, msg, "[%s%s%s][%sDBGERR%s]: ", log_blue, LOG_TIME(), log_reset, log_red, log_reset);
}

void log_dbg3(const char *msg, ...)
{
  LOG_INNER(4, msg, "[%s%s%s][%sDBG3%s]: ", log_blue, LOG_TIME(), log_reset, log_cyan, log_reset);
}

void log_info(const char *msg, ...)
{
  LOG_INNER(1, msg, "[%s%s%s][%sINFO%s]: ", log_blue, LOG_TIME(), log_reset, log_green, log_reset);
}

void log_warn(const char *msg, ...)
{
  LOG_INNER(1, msg, "[%s%s%s][%sWARN%s]: ", log_blue, LOG_TIME(), log_reset, log_yellow, log_reset);
}

void log_crit(const char *msg, ...)
{
  LOG_INNER(1, msg, "[%s%s%s][%sCRIT%s]: ", log_blue, LOG_TIME(), log_reset, log_red, log_reset);
}

void log_err(const char *msg, ...)
{
  LOG_INNER(1, msg, "[%s%s%s][%sERRR%s]: ", log_blue, LOG_TIME(), log_reset, log_red, log_reset);
}

#define logging_init() logging_set_colors(true)

/*
int main()
{
  logging_init();
  enable_really_verbose_logging();

  log_dbg("this is a debugging message!");
  log_dbg2("this is a debugging message!");
  log_dbg3("this is a debugging message!");
  log_dbgerr("a mostly harmless error!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

  logging_set_colors(false);
   log_dbg("this is a debugging message!");
  log_dbg2("this is a debugging message!");
  log_dbg3("this is a debugging message!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

 logging_set_colors(true);
  log_dbg("this is a debugging message!");
  log_dbg2("this is a debugging message!");
  log_dbg3("this is a debugging message!");
  log_info("this is an informational message!");
  log_warn("this is a warning message!");
  log_crit("this is a critical warning!");
  log_err("this is an error message!");

}
*/
