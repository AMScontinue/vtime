#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct VTimeEntry {
	long long StartTimeSec;
	long long StartTimeNanosec;
	double Rate;
};

void copyAndMove(char **dest, const char *source) {
    strcpy(*dest, source);
    *dest += strlen(source) + 1;  // 移动指针到下一行的起始位置
}

void createVTimeEntry(struct VTimeEntry *entry, long long _StartTimeSec, long long _StartTimeNanosec, double _Rate) {
    entry->StartTimeSec = _StartTimeSec;
    entry->StartTimeNanosec = _StartTimeNanosec;
    entry->Rate = _Rate;
}

int main() {
    time_t current_time = time(NULL);;

    key_t key =2222;
    int shmid = shmget(key, 1024, IPC_CREAT | 0666); // 创建共享内存段
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    char *shared_memory = (char *)shmat(shmid, NULL, 0); // 连接到共享内存
    if (shared_memory == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    struct VTimeEntry entry1,entry2,entry3;
    createVTimeEntry(&entry1,current_time,0,2);
    createVTimeEntry(&entry2,current_time+10,0,3);
    createVTimeEntry(&entry3,current_time+16,0,4);


    int writtenChars=0;
    writtenChars=sprintf(shared_memory, "%lld %lld %f\n", entry1.StartTimeSec,entry1.StartTimeNanosec,entry1.Rate);
    shared_memory+=writtenChars;
    writtenChars=sprintf(shared_memory, "%lld %lld %f\n", entry2.StartTimeSec,entry2.StartTimeNanosec,entry2.Rate);
    shared_memory+=writtenChars;
    writtenChars=sprintf(shared_memory, "%lld %lld %f\n", entry3.StartTimeSec,entry3.StartTimeNanosec,entry3.Rate);

    shmdt(shared_memory); // 分离共享内存

    sleep(100);//注意100秒后删除共享内存段！！！！！！

    shmctl(shmid, IPC_RMID, NULL); // 删除共享内存段

    return 0;
}