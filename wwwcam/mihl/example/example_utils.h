/**
 *
 * @file example_utils.h
 *
 * HTTP embedded server library
 * Copyright (C) 2006-2007  Olivier Singla
 * http://mihl.sourceforge.net/
 *
 */

#include <unistd.h>
#include <signal.h>

static int exit_now = 0;
static void sig_int(int sig)
{
	exit_now = 1;
}

/**
 * TBD
 * 
 * @param msec TBD
 */
static inline void delay( int msec ) {
    usleep( msec*1000 );
}

/**
 * TBD
 */
static void log_cb(int level, const char *msg)
{
	const char *l = "DEBUG";

	switch (level) {
		case MIHL_LOG_ERROR:
			l = "ERROR";
			break;
		case MIHL_LOG_WARNING:
			l = "WARNING";
			break;
		case MIHL_LOG_INFO:
			l = "INFO";
			break;
		case MIHL_LOG_INFO_VERBOSE:
			l = "INFO_VERBOSE";
			break;
		case MIHL_LOG_DEBUG:
			l = "DEBUG";
			break;
	}

	fprintf(stderr, "[%s] %s\n", l, msg);
}

static void help( int port ) {
	printf( "Point your browser to: http://localhost:%d\n", port );
	printf( "Ctrl+C - exit application\n");

	signal(SIGINT, sig_int);
	mihl_log_callback = log_cb;
}

