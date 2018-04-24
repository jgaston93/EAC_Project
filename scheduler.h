#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

void sig_handler(int signo);
void load_config(char *config_filename, FILE *log_file);
void get_metrics(double *metrics);
#endif
