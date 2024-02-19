#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <signal.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

sem_t semaphore1_8, semaphore2_8;
sem_t *semaphore3_6, *semaphore4_6;
sem_t semaphore5_2, semaphore6_14, semaphore7;
pthread_mutex_t running_threads_lock = PTHREAD_MUTEX_INITIALIZER;
int running_threads = 0;

void *thread8(void *id)
{
    int thread_id = *(int *)id;
    if (thread_id == 5)
    {

        info(BEGIN, 8, 5);
        sem_post(&semaphore1_8);
        sem_wait(&semaphore2_8);
        info(END, 8, 5);
    }
    else if (thread_id == 1)
    {
        sem_wait(&semaphore1_8);
        info(BEGIN, 8, thread_id);
        info(END, 8, thread_id);
        sem_post(&semaphore2_8);
    }
    else if (thread_id == 4)
    {
        sem_wait(semaphore4_6);
        info(BEGIN, 8, 4);
        info(END, 8, 4);
        sem_post(semaphore3_6);
    }
    else
    {
        info(BEGIN, 8, thread_id);
        info(END, 8, thread_id);
    }

    pthread_exit(0);
}

void *thread2(void *id)
{
    int thread_id = *(int *)id;
    if (thread_id == 14)
    {
        sem_wait(&semaphore6_14);
    }
    sem_wait(&semaphore5_2);

    pthread_mutex_lock(&running_threads_lock);
    running_threads++;
    pthread_mutex_unlock(&running_threads_lock);

    if (running_threads >= 40 && thread_id != 14)
    {
        info(BEGIN, 2, thread_id);
        if (running_threads == 42)
        {
            sem_post(&semaphore6_14);
        }
        sem_wait(&semaphore7);
    }
    else
    {
        info(BEGIN, 2, thread_id);
    }

    info(END, 2, thread_id);

    if (thread_id == 14)
    {
        for (int i = 0; i < 3; i++)
        {
            sem_post(&semaphore7);
        }
    }
    sem_post(&semaphore5_2);

    pthread_exit(0);
}

void *thread6(void *id)
{
    int thread_id = *(int *)id;
    if (thread_id == 2)
    {
        info(BEGIN, 6, 2);
        sem_post(semaphore4_6);
        info(END, 6, 2);
    }
    else if (thread_id == 1)
    {
        sem_wait(semaphore3_6);
        info(BEGIN, 6, 1);
        info(END, 6, 1);
    }
    else
    {
        info(BEGIN, 6, thread_id);
        info(END, 6, thread_id);
    }
    pthread_exit(0);
}

void declare_threads_for_process8()
{

    pthread_t threads8[5];
    int thread_ids[5];
    for (int i = 0; i < 5; i++)
    {
        thread_ids[i]=i + 1;
        pthread_create(&threads8[i], NULL, thread8, &thread_ids[i]);
    }

    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads8[i], NULL);
    }

    sem_destroy(&semaphore1_8);
    sem_destroy(&semaphore2_8);
}

void decleare_threads_for_process2()
{
    sem_init(&semaphore5_2, 0, 4);
    sem_init(&semaphore6_14, 0, 0);
    sem_init(&semaphore7, 0, 0);
    pthread_mutex_init(&running_threads_lock, NULL);
    pthread_t threads2[43];
    int threads2_ids2[43];

    info(BEGIN, 2, 0);
    for (int i = 0; i < 43; i++)
    {
        threads2_ids2[i] = i+1;
        pthread_create(&threads2[i], NULL, thread2, &threads2_ids2[i]);
    }
    for (int i = 0; i < 43; i++)
    {
        pthread_join(threads2[i], NULL);
    }
    sem_destroy(&semaphore5_2);
    sem_destroy(&semaphore6_14);
    sem_destroy(&semaphore7);
    pthread_mutex_destroy(&running_threads_lock);
}

void declare_threads_for_process_6()
{

    pthread_t threads6[5];

    int threads6_ids[5];

    for (int i = 0; i < 5; i++)
    {
        threads6_ids[i] = i+1;
        pthread_create(&threads6[i], NULL, thread6, &threads6_ids[i]);
    }
    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads6[i], NULL);
    }
}

void declare_process_4_and_5()
{
    pid_t p4, p5;

    info(BEGIN, 3, 0);
    p4 = fork();

    if (p4 == 0)
    {
        info(BEGIN, 4, 0);
        info(END, 4, 0);
        exit(0);
    }
    else
    {
        p5 = fork();

        if (p5 == 0)
        {
            info(BEGIN, 5, 0);
            info(END, 5, 0);
            exit(0);
        }
        else
        {
            wait(NULL);
            wait(NULL);
            info(END, 3, 0);
            exit(0);
        }
    }
}

void declare_process_7_and_8()
{
    pid_t p7, p8;
    p7 = fork();

    if (p7 == 0)
    {
        info(BEGIN, 7, 0);

        p8 = fork();

        if (p8 == 0)
        {
            info(BEGIN, 8, 0);
            declare_threads_for_process8();
            info(END, 8, 0);
        }
        else
        {
            wait(NULL);
            info(END, 7, 0);
            exit(0);
        }
    }
    else
    {
        wait(NULL);
        wait(NULL);
        info(END, 2, 0);
        exit(0);
    }
}

void declare_process_6_and_9()
{
    pid_t p6, p9;
    p6 = fork();

    if (p6 == 0)
    {
        info(BEGIN, 6, 0);

        declare_threads_for_process_6();
        p9 = fork();

        if (p9 == 0)
        {
            info(BEGIN, 9, 0);
            info(END, 9, 0);
            exit(0);
        }
        else
        {
            wait(NULL);
            info(END, 6, 0);
            exit(0);
        }
    }
    else
    {
        wait(NULL);
        wait(NULL);
        info(END, 1, 0);
    }
}

int main()
{
    init();
    semaphore3_6 = sem_open("/semaphore3_6", O_CREAT, 0600, 0);
    semaphore4_6 = sem_open("/semaphore4_6", O_CREAT, 0600, 0);

    pid_t p2, p3;

    info(BEGIN, 1, 0);

    p2 = fork();

    if (p2 == 0)
    {
        decleare_threads_for_process2();

        p3 = fork();

        if (p3 == 0)
        {
            declare_process_4_and_5();
        }
        else
        {
            declare_process_7_and_8();
        }
    }
    else
    {
        declare_process_6_and_9();
    }
    sem_destroy(semaphore3_6);
    sem_destroy(semaphore4_6);
    return 0;
}