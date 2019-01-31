/*
 * except.cpp
 *
 *  Created on: 19 Aug 2018
 *      Author: mikee47
 */


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


static void signal_handler(int sig)
{
  switch (sig) {
  case SIGABRT:
    fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", stderr);
    break;
  case SIGFPE:
    fputs("Caught SIGFPE: arithmetic exception, such as divide by zero\n",
    stderr);
    break;
  case SIGILL:
    fputs("Caught SIGILL: illegal instruction\n", stderr);
    break;
  case SIGINT:
    fputs("Caught SIGINT: interactive attention signal, probably a ctrl+c\n",
    stderr);
    break;
  case SIGSEGV:
    fputs("Caught SIGSEGV: segfault\n", stderr);
    break;
  case SIGTERM:
  default:
    fputs("Caught SIGTERM: a termination request was sent to the program\n",
    stderr);
    break;
  }

  _Exit(1);
}

void trap_exceptions()
{
  signal(SIGABRT, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
}

