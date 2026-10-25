// logging.h

#pragma once

#include<stdbool.h>
#include<stdio.h>

// FIXME this interface makes no sense

void set_log_level(int level);
void logging_set_colors(bool choice);
void set_logging_output(FILE **choice);
void set_logging_file(const char * const path);
char *logging_stdout_timestamp(void);
char *logging_file_timestamp(void);
void log_dbg(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_dbg2(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_dbg3(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_dbgerr(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_info(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_warn(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_crit(const char *msg, ...) __attribute__((format(printf, 1, 2)));
void log_err(const char *msg, ...) __attribute__((format(printf, 1, 2)));

#define LOGGING_INIT(out) set_logging_file(out)
#define logging_init() logging_set_colors(true)
#define enable_logging() set_log_level(2)
#define enable_verbose_logging() set_log_level(3)
#define enable_really_verbose_logging() set_log_level(4)
#define disable_logging() set_log_level(0)

