// http://www.ncad.co.jp/~komata/c-kouza28.htm

#include        <stdio.h>
#include		<stdlib.h>
#include        <sys/types.h>
#include        <sys/wait.h>
#include		<unistd.h>
#include        <pthread.h>
#include		<string.h>

void *counter(void *arg)
{
int     i;
pid_t   pid;
pthread_t	thread_id;

        pid=getpid();
    thread_id=pthread_self();

        for(i=0;i<10;i++){
        sleep(1);
                printf("[%d][%ld]%d\n",pid,thread_id,i);
        }

    return(arg);
}

void main()
{
pid_t   p_pid;
pthread_t	thread_id1,thread_id2;
int	status;
void 	*result;

    p_pid=getpid();

        printf("[%d]start\n",p_pid);

    status=pthread_create(&thread_id1,NULL,counter,(void *)NULL);
    if(status!=0){
        fprintf(stderr,"pthread_create : %s",strerror(status));
    }
    else{
        printf("[%d]thread_id1=%ld\n",p_pid,thread_id1);
    }

    status=pthread_create(&thread_id2,NULL,counter,(void *)NULL);
    if(status!=0){
        fprintf(stderr,"pthread_create : %s",strerror(status));
    }
    else{
        printf("[%d]thread_id2=%ld\n",p_pid,thread_id2);
    }

    pthread_join(thread_id1,&result);
    printf("[%d]thread_id1 = %ld end\n",p_pid,thread_id1);
    pthread_join(thread_id2,&result);
    printf("[%d]thread_id2 = %ld end\n",p_pid,thread_id2);

        printf("[%d]end\n",p_pid);
}
