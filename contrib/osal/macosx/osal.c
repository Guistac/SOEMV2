/*
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 */

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <osal.h>

#define USECS_PER_SEC 1000000



/* Returns time from some unspecified moment in past,
 * strictly increasing, used for time intervals measurement. */
void osal_get_monotonic_time(ec_timet *ts)
{
   /* Use clock_gettime to prevent possible live-lock.
    * Gettimeofday uses CLOCK_REALTIME that can get NTP timeadjust.
    * If this function preempts timeadjust and it uses vpage it live-locks.
    * Also when using XENOMAI, only clock_gettime is RT safe */
   clock_gettime(CLOCK_MONOTONIC, ts);
}

ec_timet osal_current_time(void)
{
   struct timespec ts;

   clock_gettime(CLOCK_REALTIME, &ts);
   return ts;
}

void osal_time_diff(ec_timet *start, ec_timet *end, ec_timet *diff)
{
   osal_timespecsub(end, start, diff);
}

void osal_timer_start(osal_timert *self, uint32 timeout_usec)
{
   struct timespec start_time;
   struct timespec timeout;

   osal_get_monotonic_time(&start_time);
   osal_timespec_from_usec(timeout_usec, &timeout);
   osal_timespecadd(&start_time, &timeout, &self->stop_time);
}

boolean osal_timer_is_expired(osal_timert *self)
{
   struct timespec current_time;
   int is_not_yet_expired;

   osal_get_monotonic_time(&current_time);
   is_not_yet_expired = osal_timespeccmp(&current_time, &self->stop_time, <);

   return is_not_yet_expired == FALSE;
}

int osal_usleep(uint32 usec)
{
   struct timespec ts;
   ts.tv_sec = usec / USECS_PER_SEC;
   ts.tv_nsec = (usec % USECS_PER_SEC) * 1000;
   /* usleep is deprecated, use nanosleep instead */
   return nanosleep(&ts, NULL);
}

int osal_monotonic_sleep(ec_timet *ts)
{
   /*
   int result;
   result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts, NULL);
   return result == 0 ? 0 : -1;
   */
   struct timespec now, delay;

   if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
      return -1;

   delay.tv_sec  = ts->tv_sec - now.tv_sec;
   delay.tv_nsec = ts->tv_nsec - now.tv_nsec;

   if (delay.tv_nsec < 0) {
      delay.tv_nsec += 1000000000;
      delay.tv_sec -= 1;
   }

   if (delay.tv_sec < 0 || (delay.tv_sec == 0 && delay.tv_nsec <= 0)) {
      // Target time already passed
      return 0;
   }

   // Loop in case nanosleep is interrupted
   return nanosleep(&delay, &delay);
}

void *osal_malloc(size_t size)
{
   return malloc(size);
}

void osal_free(void *ptr)
{
   free(ptr);
}

int osal_thread_create(void *thandle, int stacksize, void *func, void *param)
{
   int ret;
   pthread_attr_t attr;
   pthread_t *threadp;

   threadp = thandle;
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, stacksize);
   ret = pthread_create(threadp, &attr, func, param);
   if (ret < 0)
   {
      return 0;
   }
   return 1;
}

int osal_thread_create_rt(void *thandle, int stacksize, void *func, void *param)
{
   int ret;
   pthread_attr_t attr;
   struct sched_param schparam;
   pthread_t *threadp;

   threadp = thandle;
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, stacksize);
   ret = pthread_create(threadp, &attr, func, param);
   pthread_attr_destroy(&attr);
   if (ret < 0)
   {
      return 0;
   }
   memset(&schparam, 0, sizeof(schparam));
   schparam.sched_priority = 40;
   ret = pthread_setschedparam(*threadp, SCHED_FIFO, &schparam);
   if (ret < 0)
   {
      return 0;
   }

   return 1;
}

void *osal_mutex_create(void)
{
   pthread_mutexattr_t mutexattr;
   osal_mutext *mutex;
   mutex = (osal_mutext *)osal_malloc(sizeof(osal_mutext));
   if (mutex)
   {
      pthread_mutexattr_init(&mutexattr);
      pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_INHERIT);
      pthread_mutex_init(mutex, &mutexattr);
   }
   return (void *)mutex;
}

void osal_mutex_destroy(void *mutex)
{
   pthread_mutex_destroy((osal_mutext *)mutex);
   osal_free(mutex);
}

void osal_mutex_lock(void *mutex)
{
   pthread_mutex_lock((osal_mutext *)mutex);
}

void osal_mutex_unlock(void *mutex)
{
   pthread_mutex_unlock((osal_mutext *)mutex);
}









#ifdef AAA
#define

int osal_usleep(uint32 usec)
{
   struct timespec ts;
   ts.tv_sec = usec / USECS_PER_SEC;
   ts.tv_nsec = (usec % USECS_PER_SEC) * 1000;
   /* usleep is deprecated, use nanosleep instead */
   return nanosleep(&ts, NULL);
}

ec_timet osal_current_time(void)
{
   struct timeval current_time;
   ec_timet return_value;

   osal_gettimeofday(&current_time, 0);
   return_value.sec = current_time.tv_sec;
   return_value.usec = current_time.tv_usec;
   return return_value;
}

int osal_gettimeofday(struct timeval *tv, struct timezone *tz)
{
   struct timespec ts;
   int return_value;
   (void)tz; /* Not used */

   /* Use clock_gettime to prevent possible live-lock.
    * Gettimeofday uses CLOCK_REALTIME that can get NTP timeadjust.
    * If this function preempts timeadjust and it uses vpage it live-locks.
    * Also when using XENOMAI, only clock_gettime is RT safe */
   return_value = clock_gettime(CLOCK_MONOTONIC, &ts);
   tv->tv_sec = ts.tv_sec;
   tv->tv_usec = ts.tv_nsec / 1000;
   return return_value;
}

void osal_time_diff(ec_timet *start, ec_timet *end, ec_timet *diff)
{
   if (end->usec < start->usec)
   {
      diff->sec = end->sec - start->sec - 1;
      diff->usec = end->usec + 1000000 - start->usec;
   }
   else
   {
      diff->sec = end->sec - start->sec;
      diff->usec = end->usec - start->usec;
   }
}

void osal_timer_start(osal_timert *self, uint32 timeout_usec)
{
   struct timeval start_time;
   struct timeval timeout;
   struct timeval stop_time;
   
   osal_gettimeofday(&start_time, 0);
   timeout.tv_sec = timeout_usec / USECS_PER_SEC;
   timeout.tv_usec = timeout_usec % USECS_PER_SEC;
   timeradd(&start_time, &timeout, &stop_time);
   
   self->stop_time.sec = stop_time.tv_sec;
   self->stop_time.usec = stop_time.tv_usec;
}

boolean osal_timer_is_expired(osal_timert *self)
{
   struct timeval current_time;
   struct timeval stop_time;
   int is_not_yet_expired;
   
   osal_gettimeofday(&current_time, 0);
   stop_time.tv_sec = self->stop_time.sec;
   stop_time.tv_usec = self->stop_time.usec;
   is_not_yet_expired = timercmp(&current_time, &stop_time, <);
   
   return is_not_yet_expired == FALSE;
}




void *osal_malloc(size_t size)
{
   return malloc(size);
}

void osal_free(void *ptr)
{
   free(ptr);
}

int osal_thread_create(void *thandle, int stacksize, void *func, void *param)
{
   int ret;
   pthread_attr_t attr;
   pthread_t *threadp;
   
   threadp = thandle;
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, stacksize);
   ret = pthread_create(threadp, &attr, func, param);
   if (ret < 0)
   {
      return 0;
   }
   return 1;
}

int osal_thread_create_rt(void *thandle, int stacksize, void *func, void *param)
{
   int ret;
   pthread_attr_t attr;
   struct sched_param schparam;
   pthread_t *threadp;
   
   threadp = thandle;
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, stacksize);
   ret = pthread_create(threadp, &attr, func, param);
   pthread_attr_destroy(&attr);
   if (ret < 0)
   {
      return 0;
   }
   memset(&schparam, 0, sizeof(schparam));
   schparam.sched_priority = 40;
   ret = pthread_setschedparam(*threadp, SCHED_FIFO, &schparam);
   if (ret < 0)
   {
      return 0;
   }
   
   return 1;
}
#endif
