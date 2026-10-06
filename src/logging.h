// logging.h

#include<stdbool.h>
#include<stdio.h>

void set_log_level(int level);
void logging_set_colors(bool choice);
void set_logging_output(FILE **choice);
char *logging_stdout_timestamp(void);
char *logging_file_timestamp(void);
void log_dbg(const char *msg, ...);
void log_dbg2(const char *msg, ...);
void log_dbg3(const char *msg, ...);
void log_dbgerr(const char *msg, ...);
void log_info(const char *msg, ...);
void log_warn(const char *msg, ...);
void log_crit(const char *msg, ...);
void log_err(const char *msg, ...);

#define logging_init() logging_set_colors(true)
#define enable_logging() set_log_level(2)
#define enable_verbose_logging() set_log_level(3)
#define enable_really_verbose_logging() set_log_level(4)
#define disable_logging() set_log_level(0)

