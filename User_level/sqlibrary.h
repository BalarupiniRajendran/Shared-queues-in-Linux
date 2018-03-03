#include<stdio.h>
#include<stdlib.h>
#define MAX 10

//Defining the structure of message written in queue
struct thread_msg{
	int msg_id;
	int src_id;
	unsigned long long queuing_time;
	double flt_num;
};

//Defining the structure of the queue
struct squeue{
  int front;
  int rear;
  struct thread_msg *queuen[MAX];
};
//Creating the queue by allocating memeory and initializing front and rear of the queue
struct squeue* sq_create(){
  struct squeue *sq;
  sq = (struct squeue*)malloc(sizeof(struct squeue));
  sq->front=-1;
  sq->rear=-1;
  return sq;
}

//Checking if the queue is empty
int isEmpty(struct squeue* sq)
{
    if(sq->front==-1)
        return 1;
    else
        return 0;
}
//Checking if the queue is full
int isFull(struct squeue* sq)
{
    if((sq->front==0 && sq->rear==MAX-1) || (sq->front==sq->rear+1))
        return 1;
    else
        return 0;
}
//Inserting a queue node at the rear of the queue
int sq_write(struct thread_msg *qn, struct squeue* sq)
{
    if( isFull(sq) )
    {
        return -1;
    }
    if(sq->front == -1 )
        sq->front=0;

    if(sq->rear==MAX-1)
        sq->rear=0;
    else
        sq->rear=sq->rear+1;
    sq->queuen[sq->rear]= qn;
    
    return 1;
}
//Reading the element in the front from the queue
int sq_read(struct squeue* sq, struct thread_msg *item)
{

    if( isEmpty(sq) )
    {
        return -1;
    }
    *item=*sq->queuen[sq->front];
    if(sq->front==sq->rear)
    {
        sq->front=-1;
        sq->rear=-1;
    }
    else if(sq->front==MAX-1)
        sq->front=0;
    else
        sq->front=sq->front+1;

    return 1;
}
//Freeing the allocated memory for queue
void sq_delete(struct squeue* sq){
  free(sq);
}
