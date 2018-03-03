#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "sqlibrary.h"
#include<sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#define PATH "/dev/input/event4"
#define BASE_PERIOD 1000
#define randnum(min, max) ((rand() % (int)(((max) + 1) - (min))) + (min))

const int p_mult[]={12,24,36,48};
const int r_mult=40;

typedef unsigned long long ticks;

long message_counter=0;
unsigned long long sum_of_q_time=0;
int qtime[16000];
int flag =1;
struct thread_msg msg[7];
struct squeue *myqueue1, *myqueue2;
static int messageid =1;
pthread_mutex_t lock;

void* f1(void* args);
void* f2(void* args);
void* receiving_thread_func(void* args);
double calcpi();

//calculates the time difference between two timings
double time_diff(struct timeval current_time, struct timeval past)
{
    double cms, pms;
    cms = (double)current_time.tv_sec*1000 + (double)current_time.tv_usec/1000;
    pms = (double)past.tv_sec*1000 + (double)past.tv_usec/1000;
    return(cms - pms);
}
//calculates the number of CPU cylces
static inline ticks getticks(void)
 {
      unsigned a, d;
      asm("cpuid");
      asm volatile("rdtsc" : "=a" (a), "=d" (d));
      return (((ticks)a) | (((ticks)d) << 32));
 }
//Calculates the queueing time by fining the difference between in-time and out-time
 unsigned long long calc_queuing_time(struct thread_msg* ms){
  int r = 0;
  ticks tick,tick1,tickh;
  unsigned long long  time =0;
  tick = ms->queuing_time;
  tick1 = getticks();
  time = (unsigned)((tick1-tick)/(CLOCKS_PER_SEC));
  ms->queuing_time=time;

  printf("\ntime in MS %llu\n",time);
  return time;
}
//calculates the average queueing time
double find_avg_qtime(){
  double average = sum_of_q_time/message_counter;
  return average;
}
//Calcualtes the standard deviation of the queueing times
double find_std_deviation(double average){
  long sum=0;
  int diff=0;
  for(int l=0;l<message_counter;l++){
  diff = qtime[l]-average;
  sum= sum+(diff*diff);
  }
  sum = sum/message_counter;
  double std = sqrt(sum);
  return std;
}
//thread body function for aperiodic threads
void* f2(void *args)
{
  int fd;
  struct input_event input;
  struct timeval previous_timeval ;
  double time_elapsed;

if((fd = open(PATH, O_RDONLY)) == -1) {
      printf("some error occured while opening device");
      exit(EXIT_FAILURE);
  }

for(int k=4;k<6;k++)
{
  while(read(fd, &input, sizeof(struct input_event)))
  {
    struct thread_msg *mynode;
    mynode  = (struct thread_msg*)malloc(sizeof(struct thread_msg));
    msg[k].msg_id= messageid;
    messageid++;
    ticks inqtick = getticks();
    msg[k].queuing_time = inqtick;
    msg[k].flt_num =calcpi();
    mynode = &msg[k];
    int success;
             if(input.type == 1 && input.code == 272 && input.value == 1)

              {
                  time_elapsed = time_diff(input.time, previous_timeval);

                  if(time_elapsed <= 500){
                  printf("Left Double Click \n");
                  flag = 0;
                  }
                  else
                  {
                  success = sq_write(mynode,myqueue1);
                  if(success==1)
                   printf("Left Single Click\n");
                  previous_timeval = input.time;

                  }

              }
else if(input.type == 1 && input.code == 273 && input.value == 1)

              {

                  success = sq_write(mynode,myqueue2);
                   printf("Right Single Click\n");
                  previous_timeval = input.time;

              }
}

   }
}
//thread body function for periodic threads
void* f1(void *args)
{
pthread_mutex_lock(&lock);
struct timespec mytimespec,myperiod;

 clock_gettime( CLOCK_MONOTONIC, &mytimespec);

 while(flag)
 {
  for(int k=0;k<4;k++)
  {
    myperiod.tv_sec = 0;
    myperiod.tv_nsec = p_mult[msg[k].src_id]* BASE_PERIOD*1000;
    struct thread_msg *mynode;
    mynode  = (struct thread_msg*)malloc(sizeof(struct thread_msg));
    msg[k].msg_id= messageid;

    messageid++;
    ticks inqtick = getticks();
    msg[k].queuing_time = inqtick;
    msg[k].flt_num =calcpi();

  mynode = &msg[k];
  int success;
  if(k <2)
  success = sq_write(mynode,myqueue1);
  else
  success = sq_write(mynode,myqueue2);

  if((mytimespec.tv_nsec + myperiod.tv_nsec)>=1000000000)
     {
       mytimespec.tv_nsec=((mytimespec.tv_nsec+myperiod.tv_nsec)%1000000000);
       mytimespec.tv_sec++;
     }
     else
     {
       mytimespec.tv_nsec=mytimespec.tv_nsec+myperiod.tv_nsec;
     }
     clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mytimespec,0);
     pthread_mutex_unlock(&lock);
 }
 }
}
// thread body of the receiver thread
void* receiving_thread_func(void *args)
{
pthread_mutex_lock(&lock);
struct timespec mytimespec,myperiod;
myperiod.tv_sec = 0;
myperiod.tv_nsec = r_mult* BASE_PERIOD*1000;
int j=0;

unsigned long long  time_diff =0;
 clock_gettime( CLOCK_MONOTONIC, &mytimespec);

 while(1)
 {
struct thread_msg* item;
item  = (struct thread_msg*)malloc(sizeof(struct thread_msg));

 int success1 =0;
 success1 = sq_read(myqueue1,item);
 if (success1 == 1) {
   message_counter = message_counter+1;
   qtime[j]= calc_queuing_time(item);
   sum_of_q_time = sum_of_q_time + qtime[j];
   j++;
 }
 int success2 =0;
 success2 = sq_read(myqueue2,item);
 if (success2 ==1) {
   message_counter = message_counter+1;
   qtime[j]= calc_queuing_time(item);
   sum_of_q_time = sum_of_q_time + qtime[j];
   j++;
 }
 if((mytimespec.tv_nsec + myperiod.tv_nsec)>=1000000000)
 		{
 			mytimespec.tv_nsec=((mytimespec.tv_nsec+myperiod.tv_nsec)%1000000000);
 			mytimespec.tv_sec++;
 		}
 		else
 		{
 			mytimespec.tv_nsec=mytimespec.tv_nsec+myperiod.tv_nsec;
 		}
 		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mytimespec,0);

 pthread_mutex_unlock(&lock);
if(flag==0 && isEmpty(myqueue1) && isEmpty(myqueue2)){
  double avg = find_avg_qtime();
  printf("\n-----------------------------------------------");
  printf ("\ntotal number of messages received: %ld", message_counter);
  printf("\nmean of the queueing time: %lfms",avg );
  printf("\nStandard deviation of the queueing time: %lf",find_std_deviation(avg));
  printf("\n-----------------------------------------------\n");
  exit(-1);
}
 }
}

double calcpi()// for calculating message pi
{

srand(time(NULL));

int randnumber;// no. of iterations
randnumber=randnum(10,50);


   double x, y;     // Number of iterations and control variables
   double f;           // factor that repeats
   double pi = 1;


   for(x = randnumber; x > 1; x--) {
      f = 2;
      for(y = 1; y < x; y++){
         f = 2 + sqrt(f);
      }

    f = sqrt(f);
      pi = pi * f / 2;
   }


   pi *= sqrt(2)/2;
   pi = 2 / pi;


return pi;
     }

int main()
{
  myqueue1 = sq_create();
  myqueue2 = sq_create();

  pthread_t ptids[4], atids[2];
  pthread_t rtid;
  pthread_attr_t attr;
  struct sched_param param;

  for (int i = 0; i < 4; i++) {
  pthread_attr_init (&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority=2;
  pthread_attr_setschedparam(&attr,&param);
  pthread_create(&ptids[i],&attr, f1, NULL);
  msg[i].src_id=i+1;

}
  for (int i = 0; i < 1; i++) {

    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority=1;
    pthread_attr_setschedparam(&attr,&param);
    pthread_create(&atids[i],&attr,f2,NULL);
    msg[i+5].src_id=i+4;}
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority=3;
    pthread_attr_setschedparam(&attr,&param);
    pthread_create(&rtid,&attr,receiving_thread_func,NULL);
    msg[6].src_id=7;

    for (int i = 0; i < 4; i++) {
        pthread_join(ptids[i],NULL);}

    for (int i = 0; i < 2; i++) {
        pthread_join(atids[i],NULL);}
        pthread_join(rtid,NULL);
        return 0;
}
