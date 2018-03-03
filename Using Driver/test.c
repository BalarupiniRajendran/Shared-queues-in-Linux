#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "thread_msg.h"
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <semaphore.h>
#include <linux/errno.h>
#include <errno.h>

#define QUEUE1_PATH "/dev/dataqueue1"
#define QUEUE2_PATH "/dev/dataqueue2"
#define PATH "/dev/input/event4"
#define BASE_PERIOD 1000
#define randnum(min, max) ((rand() % (int)(((max) + 1) - (min))) + (min))

const int p_mult[]={12,24,36,48};
const int r_mult=40;
int isempty1=0;
int isempty2=0;

typedef unsigned long long ticks;

long message_counter=0;
unsigned long long sum_of_q_time=0;
int qtime[16000];
int flag =1;
struct thread_msg msg[7];
static int messageid =1;
pthread_mutex_t lock;

void* f1(void* args);
void* f2(void* args);
double calcpi();

sem_t sem1,sem2;

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
//Calculates the queueing time by finding the difference between in-time and out-time
 unsigned long long calc_queueing_time(struct thread_msg* ms){
  ticks tick,tick1;
  unsigned long long  time =0;
  tick = ms->queueing_time;
  tick1 = getticks();
  time = (unsigned)((tick1-tick)/(CLOCKS_PER_SEC));
  ms->queueing_time=time;

  printf("\nTime in MS %llu\n",time);
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
  int l;
  for(l=0;l<message_counter;l++){
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
  printf("inside f2 function\n");
  int fd;
  struct input_event input;
  struct timeval previous_timeval ;
  double time_elapsed;


if((fd = open(PATH, O_RDONLY)) == -1) {
      printf("some error occured while opening device\n");
      exit(EXIT_FAILURE);
  }


  while(read(fd, &input, sizeof(struct input_event)))
  {

    struct thread_msg *mynode;

    int success;
             if(input.type == 1 && input.code == 272 && input.value == 1)

              {
                  time_elapsed = time_diff(input.time, previous_timeval);

                  if(time_elapsed <= 500){
                  printf("Left Double Click \n");
                  flag = 0;
                  }
                  else
                  { sem_wait(&sem1);
                    mynode  = (struct thread_msg*)malloc(sizeof(struct thread_msg));
		    msg[4].msg_id= messageid;
		    messageid++;
		    ticks inqtick = getticks();
		    msg[4].queueing_time = inqtick;
		    msg[4].flt_num =calcpi();
		    mynode = &msg[4];
                    int fd=0,r;
                    fd = open(QUEUE1_PATH, O_RDWR);
                    if(fd==-1){
                      printf("file dataqueue1 does not exist or currently used by another user\n");
                    }
                    r = write(fd,(void*)mynode,sizeof(struct thread_msg));
                    if(r==-1){
                      printf("q1 is full\n");
                    }
                    close(fd);
                    if(r>0){
                      success=1;
                    }
                    if(success==1)
                    printf("Left Single Click\n");
                    previous_timeval = input.time;

                    sem_post(&sem1);

                  }

              }
                  else if(input.type == 1 && input.code == 273 && input.value == 1)

              {     sem_wait(&sem2);
                    mynode= (struct thread_msg*)malloc(sizeof(struct thread_msg));
		    msg[5].msg_id= messageid;
		    messageid++;
		    ticks inqtick = getticks();
		    msg[5].queueing_time = inqtick;
		    msg[5].flt_num =calcpi();
		    mynode = &msg[5];
                  int fd=0,r;
                  fd = open(QUEUE2_PATH, O_RDWR);
                  if(fd==-1){
                    printf("file dataqueue2 does not exist or currently used by another user\n");
                  }
                  r = write(fd,(void*)mynode,sizeof(struct thread_msg));
                  if(r==-1){
                    printf("q2 is full\n");
                  }
                  close(fd);
                  if(r>0){
                    success=1;
                  }
                  if(success==1){
                    printf("Right Single Click\n");}
                    previous_timeval = input.time;
                    sem_post(&sem2);


}

   }
return 0;
}
//thread body function for periodic threads
void* f1(void *args)
{
  printf("inside f1 function\n");
pthread_mutex_lock(&lock);
printf("\nafter lock\n");
struct timespec mytimespec,myperiod;
int k;
 clock_gettime( CLOCK_MONOTONIC, &mytimespec);
printf("\nbefore flag\n");
 while(flag)
 {
   printf("\nafter flag\n");
  for(k=0;k<4;k++)
  {
    printf("\ninside while loop\n");
    myperiod.tv_sec = 0;
    myperiod.tv_nsec = p_mult[msg[k].src_id]* BASE_PERIOD*1000;
    struct thread_msg *mynode;
    mynode  = (struct thread_msg*)malloc(sizeof(struct thread_msg));
    printf("after allocating memory for mynode\n");
    msg[k].msg_id= messageid;

    messageid++;
    ticks inqtick = getticks();
    msg[k].queueing_time = inqtick;
    msg[k].flt_num =calcpi();

  mynode = &msg[k];

  if(k <2){
    printf("inside first if\n");

int fd=0,r;
fd = open(QUEUE1_PATH, O_RDWR);
if(fd==-1){
  printf("file dataqueue1 does not exist or currently used by another user\n");
}
r = write(fd,(void*)mynode,sizeof(struct thread_msg));
if(r==-1){
  printf("q1 is full\n");
}
close(fd);
}
  else{
    int fd=0,r;
    fd = open(QUEUE2_PATH, O_RDWR);
    if(fd==-1){
      printf("file dataqueue1 does not exist or currently used by another user\n");
    }
    r = write(fd,(void*)mynode,sizeof(struct thread_msg));
    if(r==-1){
      printf("q2 is full\n");
    }
    close(fd);

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
 }
 }
return 0;
}


// thread body of the receiver thread
void* receiving_thread_func(void *args)
{
pthread_mutex_lock(&lock);
struct timespec mytimespec,myperiod;
myperiod.tv_sec = 0;
myperiod.tv_nsec = r_mult* BASE_PERIOD*1000;
int j=0;

 clock_gettime( CLOCK_MONOTONIC, &mytimespec);

 while(1)
 {
struct thread_msg* item;
item  = (struct thread_msg*)malloc(sizeof(struct thread_msg));

 int success1 =0;
 
int fdq1,rq1;
fdq1= open(QUEUE1_PATH, O_RDWR);
if (fdq1 == -1) {
      printf(
          "file either does not exist or is currently used by another user \n");
    }
    rq1= read(fdq1,(void*)item,sizeof(struct thread_msg));
    if(rq1>0){
      success1=1;
    }
 
 if (success1 == 1) {
   message_counter = message_counter+1;
   qtime[j]= calc_queueing_time(item);
   sum_of_q_time = sum_of_q_time + qtime[j];
   j++;
 }
 int success2 =0;

 int fdq2,rq2;
 fdq2= open(QUEUE2_PATH, O_RDWR);
 if (fdq2 == -1) {
       printf(
           "file either does not exist or is currently used by another user \n");
     }
     rq2= read(fdq2,(void*)item,sizeof(struct thread_msg));
     if(rq2>0){
       success2=1;
     }

 if (success2 ==1) {
   message_counter = message_counter+1;
   qtime[j]= calc_queueing_time(item);
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
if(flag==0 && isempty1 && isempty2){
  double avg = find_avg_qtime();
  printf("\n-----------------------------------------------");
  printf("\nTotal number of messages received: %ld", message_counter);
  printf("\nMean of the queueing time: %lfms",avg );
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
 
printf("inside main\n");
  pthread_t ptids[4], atids[2];
  pthread_t rtid;
  pthread_attr_t attr;
  struct sched_param param;
  sem_init(&sem2, 0, 1);
  sem_init(&sem1, 0, 1);
  int i;

  for (i = 0; i < 4; i++) {
  pthread_attr_init (&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority=2;
  pthread_attr_setschedparam(&attr,&param);
  pthread_create(&ptids[i],&attr, f1, NULL);
  msg[i].src_id=i+1;

}
  for (i = 0; i < 1; i++) {

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

    for (i = 0; i < 4; i++) {
        pthread_join(ptids[i],NULL);}

    for (i = 0; i < 2; i++) {
        pthread_join(atids[i],NULL);}
        pthread_join(rtid,NULL);

        return 0;
}
