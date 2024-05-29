#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>

void alarm_handler(int signum) {
    printf("Received SIGALRM signal\n");
}


int main()
{
    struct timeval tv;
    time_t current_time;
    gettimeofday(&tv, NULL);
    current_time = time(NULL);

    printf("time() says currenttime is: %ld\ngettimeofday() says currenttime is: %ld, Microseconds: %ld\n", current_time, tv.tv_sec, tv.tv_usec);
    
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    printf("clock_gettime says currenttime is: %ld , nanoseconds is: %ld \n", spec.tv_sec, spec.tv_nsec);

    // sleep(20);

    // gettimeofday(&tv, NULL);
    // current_time = time(NULL);

    // printf("time() says currenttime is: %ld\ngettimeofday() says currenttime is: %ld, Microseconds: %ld\n", current_time, tv.tv_sec, tv.tv_usec);

    // struct timespec spec;
    // clock_gettime(CLOCK_REALTIME, &spec);
    // printf("clock_gettime says currenttime is: %ld , nanoseconds is: %ld \n", spec.tv_sec, spec.tv_nsec);

    // struct timespec req, rem;
    // req.tv_sec = 3;  // 休眠1秒
    // req.tv_nsec = 0;

    // syscall(SYS_nanosleep, &req, &rem);

    // // 注册 SIGALRM 信号处理函数
    // signal(SIGALRM, alarm_handler);

    // // 设置定时器，5秒后发送 SIGALRM 信号
    // unsigned int remaining_time = alarm(5);

    // printf("Alarm set for 5 seconds\n");

    // // 进程暂停等待信号
    // pause();

    return 0;
}
