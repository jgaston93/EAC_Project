#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h>

#include "scheduler.h"
#include "scimark2.h"


unsigned int algorithm;
unsigned int logical_core;
unsigned int physical_core;
unsigned int sched_metric;
unsigned int activation_threshold;
double std_threshold;
unsigned int sth_threshold;
unsigned int polling_interval;
unsigned int sleep_time;


int main(int argc, char *argv[])
{
	// set up things

	
	FILE *log_file;
	log_file = fopen(argv[2], "w");
	pid_t benchmark_pid, end_pid;
	int status;
	printf("Starting Scheduler\n");
	load_config(argv[1], log_file);

	// fork to create benchmark process
	benchmark_pid = fork();
	
	if(benchmark_pid == 0) {
		// Benchmark thread
		
		fclose(log_file);

		// setup signal handler to sleep process
		if(algorithm != 0) {
			signal(SIGUSR1, sig_handler);
		}

		// set cpu affinity for the benchmark thread
		if(logical_core < 4) {
			cpu_set_t my_set;
			CPU_ZERO(&my_set);
			CPU_SET(logical_core, &my_set); 
			sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
		}		
		// run the benchmark
		scimark2(4);
        	return 0;
	}
	else {
		// Scheduler thread

		// setup things

		double *T1 = malloc(4*sizeof(double)); // holds first poll
		double *T2 = malloc(4*sizeof(double)); // holds second poll
		
		struct timeval start_time;
		struct timeval end_time;
		struct timeval T1_time;
		struct timeval T2_time;

		double delta_time; // Time between polls 

		double gradient; // Change in temperature
		
		unsigned int sleep_thread; // Boolean to sleep thread
				
		printf("\nTime,Max,Core0,Core1,Power\n");
		fprintf(log_file, "\nTime,Max,Core0,Core1,Power\n");
		
		gettimeofday(&start_time, 0);

		// start main scheduler loop
		while(1) {
			// get first readings
			get_metrics(T1);
			gettimeofday(&T1_time, 0);
			
			// sleep for the polling interval
			usleep(polling_interval);

			// get second readings
			get_metrics(T2);
			gettimeofday(&T2_time, 0);

			// calculate gradient if std is used
			if (algorithm == 2 || algorithm == 3) {
				// time elapsed between readings
				delta_time = ((T2_time.tv_sec-T1_time.tv_sec)*1000000.0 + T2_time.tv_usec-T1_time.tv_usec)/1000000.0; // Time in seconds
				gradient = (T2[sched_metric] -T1[sched_metric])/delta_time; // change in temp
			}
			// check if scheduler is active and temp is greater than activation temp
			if(algorithm != 0 && T2[sched_metric] > activation_threshold) {

				// deteremine if the thread should sleep based on the algorithm
				sleep_thread = (algorithm == 1 && T2[sched_metric] > sth_threshold) ||
						((algorithm == 2 || algorithm == 3) && gradient > std_threshold);			
				// sleep thread if necessary
				if(sleep_thread) {
					kill(benchmark_pid, SIGUSR1);
				}
			}

			

			delta_time = ((T2_time.tv_sec-start_time.tv_sec)*1000000.0 + T2_time.tv_usec-start_time.tv_usec)/1000000.0; // Time in seconds

			printf("%f,%u,%u,%u,%f,%f\n", delta_time, (int)T2[0], (int)T2[1], (int)T2[2], T2[3], gradient);
			fprintf(log_file,"%f,%u,%u,%u,%f\n", delta_time, (int)T2[0], (int)T2[1], (int)T2[2], T2[3]);

			// check if the child is finished
			end_pid = waitpid(benchmark_pid, &status, WNOHANG|WUNTRACED);
			if(end_pid == benchmark_pid) {
				break;
			}
		}

		// record finish time
		gettimeofday(&end_time, 0);
		delta_time = ((end_time.tv_sec-start_time.tv_sec)*1000000.0 + end_time.tv_usec-start_time.tv_usec)/1000000.0;
		printf("Total Time: %lf\n", delta_time);
		fclose(log_file);
	}

        return 0;
  
}



// signal handler to put benchmark process to sleep
void sig_handler(int signo)
{
	if (signo == SIGUSR1)
		usleep(sleep_time);
}


// loads configuration data from the specified file
void load_config(char *config_filename, FILE *log_file)
{
	FILE *config_file;
	int num_read;
	config_file = fopen(config_filename, "r");
	num_read = fscanf(config_file, "Algorithm: %d\n", &algorithm);
	num_read = fscanf(config_file, "Activation Threshold: %u\n", &activation_threshold);
	num_read = fscanf(config_file, "STD Threshold: %lf\n", &std_threshold);
	num_read = fscanf(config_file, "STH Threshold: %u\n", &sth_threshold);
	num_read = fscanf(config_file, "Polling Interval: %u\n", &polling_interval);
	num_read = fscanf(config_file, "Sleeping Interval: %u\n", &sleep_time);
	num_read = fscanf(config_file, "Core Affinity: %u", &logical_core);
	printf("Algorithm: %u\n", algorithm);
	fprintf(log_file, "Algorithm: %u\n", algorithm);
	printf("Polling Interval: %f ms\n", polling_interval*1e-3);
	fprintf(log_file, "Polling Interval: %f ms\n", polling_interval*1e-3);
	if(algorithm != 0) {
		printf("Sleep Interval: %f ms\n", sleep_time*1e-3);
		fprintf(log_file, "Sleep Interval: %f ms\n", sleep_time*1e-3);
		printf("Activation Threshold: %u degrees celcius\n", activation_threshold);
		fprintf(log_file, "Activation Threshold: %u degrees celcius\n", activation_threshold);
	}
	if(algorithm == 1) {
		printf("STH Threshold: %u degrees celcius\n", sth_threshold);
		fprintf(log_file, "STH Threshold: %u degrees celcius\n", sth_threshold);
	}else if(algorithm == 2) {
		printf("STD Threshold : %f degrees celcius\n ", std_threshold);
		fprintf(log_file, "STD Threshold : %f degrees celcius\n ", std_threshold);
	}
	printf("Core Affinity: %d\n", logical_core);
	fprintf(log_file, "Core Affinity: %d\n", logical_core);
	if(logical_core == 0 || logical_core == 2) {
		physical_core = 1;
		sched_metric = 1;
	}else if (logical_core == 1 || logical_core == 3) {
		physical_core = 2;
		sched_metric = 2;
	}else {
		sched_metric = 0;
	}
	if(algorithm == 3) {
		sched_metric = 3;
	}
}
	


void get_metrics(double *metrics)
{

	// setup things
	FILE *max_tempfile;
	FILE *core0_tempfile;
	FILE *core1_tempfile;
	FILE *power_file;
	
	int num_read;

	max_tempfile = fopen("/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp1_input", "r"); // Max temperature
	core0_tempfile = fopen("/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp2_input", "r"); // Core 0 temperature
	core1_tempfile = fopen("/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp3_input", "r"); // core 1 temperature
	power_file = fopen("/sys/class/power_supply/BAT0/power_now", "r"); // CPU power draw
	
	// read values from files
	num_read = fscanf(max_tempfile, "%lf", &metrics[0]);
	num_read = fscanf(core0_tempfile, "%lf", &metrics[1]);
	num_read = fscanf(core1_tempfile, "%lf", &metrics[2]);
	num_read = fscanf(power_file, "%lf", &metrics[3]);
	
	// convert from milli and micro
	metrics[0] = metrics[0] * 1e-3; // millidegrees to degrees
	metrics[1] = metrics[1] * 1e-3; // millidegrees to degrees
	metrics[2] = metrics[2] * 1e-3; // millidegrees to degrees
	metrics[3] = metrics[3] * 1e-6; // microwatts to watts
	
	// clean up
	fclose(max_tempfile);
	fclose(core0_tempfile);
	fclose(core1_tempfile);
	fclose(power_file);

}

