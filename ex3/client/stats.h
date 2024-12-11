#ifndef __STATS__
#define __STATS__

#include <sys/types.h>

struct statistic {
  double tsum;
  double tsumsq;
  double tmin;
  double tmax;
  size_t psend;
  size_t precive;
};

void st_init(struct statistic *stat);
void st_send(struct statistic *stat);
void st_recive(struct statistic *stat, const struct timeval *triptime);
void st_print(struct statistic *stat);

#endif // !__STATS__
