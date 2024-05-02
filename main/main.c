#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
int main()
{
    struct timeval tv;
    time_t current_time;
    time_t current_time_ptr;
    gettimeofday(&tv, NULL);
    current_time = time(NULL);

    printf("time() says currenttime is: %ld\ngettimeofday() says currenttime is: %ld, Microseconds: %ld\n", current_time, tv.tv_sec, tv.tv_usec);

    sleep(20);

    gettimeofday(&tv, NULL);
    current_time = time(NULL);

    printf("time() says currenttime is: %ld\ngettimeofday() says currenttime is: %ld, Microseconds: %ld\n", current_time, tv.tv_sec, tv.tv_usec);

    return 0;
}
