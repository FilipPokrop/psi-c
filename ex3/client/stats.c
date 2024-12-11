#include "stats.h"
#include "float.h"
#include <math.h>
#include <stdio.h>

void st_init(struct statistic *stat) {
  stat->tsum = 0.0;
  stat->tsumsq = 0.0;
  stat->tmin = DBL_MAX;
  stat->tmax = -DBL_MAX;
  stat->psend = 0;
  stat->precive = 0;
}

void st_send(struct statistic *stat) { stat->psend++; }

void st_recive(struct statistic *stat, const struct timeval *triptime) {
  stat->precive++;
  double tt = (double)triptime->tv_sec + (double)triptime->tv_usec / 1000000;
  stat->tsum += tt;
  stat->tsumsq += tt * tt;
  if (tt < stat->tmin)
    stat->tmin = tt;
  if (tt > stat->tmax)
    stat->tmax = tt;
}

void st_print(struct statistic *stat) {
  if (stat->psend > 0) {
    double lost = (double)(stat->psend - stat->precive) / stat->precive * 100;
    printf("%zu packet transmited, %zu recived, lost %f%%\n", stat->psend,
           stat->precive, lost);
  }
  if (stat->precive > 0) {
    double avg = stat->tsum / stat->precive;
    double std = sqrt(stat->tsumsq / stat->precive - avg * avg);
    printf("rtt min/avg/max/std %f/%f/%f/%f\n", stat->tmin, avg, stat->tmax,
           std);
  }
}
