#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>

typedef long (*syscall_fn_t)(long, long, long, long, long, long, long);

static syscall_fn_t next_sys_call = NULL;

struct VTimeEntry {
	long long StartTimeSec;
	long long StartTimeNanosec;
	double Rate;
};

void createVTimeEntry(struct VTimeEntry *entry, long long _StartTimeSec, long long _StartTimeNanosec, double _Rate) {
    entry->StartTimeSec = _StartTimeSec;
    entry->StartTimeNanosec = _StartTimeNanosec;
    entry->Rate = _Rate;
}


/*-----global variables-----*/
int gshmid;

static long getVcurrtime(long Rtime);

static void getVfuturetime(struct timespec *sleeptime);

static void getVcurrtimeNano(struct timespec *Rtime);

//获取共享内存头地址
char* getShmHeadAddr(){
    char *shared_memory = (char *)shmat(gshmid, NULL, 0); 
    if (shared_memory == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}

//返回vTimeEntries数组首项地址，参数是entries条目数
struct VTimeEntry* readShareMem(int *entryNum){
    // 释放动态分配的数组并初始化清空
    struct VTimeEntry* vTimeEntries = NULL;
    *entryNum = 0;

    // 连接到共享内存，获取起始地址指针
    char *shared_memory = getShmHeadAddr();

    printf("-----\nMessage from shared memory:\n%s\n", shared_memory);

    // 复制共享内存中的内容到临时缓冲区
    char *tempBuffer = strdup(shared_memory);

    char *line = strtok(tempBuffer, "\n");
    while (line != NULL) {
        // 分配内存以容纳新元素
        vTimeEntries = (struct VTimeEntry *)realloc(vTimeEntries, ((*entryNum) + 1) * sizeof(struct VTimeEntry));
        if (vTimeEntries == NULL) {
            fprintf(stderr, "Memory reallocation failed\n");
            return NULL;
        }

        // 构造并添加新entry
        struct VTimeEntry entry;
        sscanf(line, "%lld %lld %lf", &entry.StartTimeSec, &entry.StartTimeNanosec, &entry.Rate);
        vTimeEntries[*entryNum] = entry;
        (*entryNum)++;
        line = strtok(NULL, "\n");
    }

    free(tempBuffer);// 释放临时变量
    shmdt(shared_memory); // 分离共享内存

    return vTimeEntries;
}


void initShareMem() {
    // 获取 shmid
    long key;
    const char *key_str = getenv("SHM_KEY");
    if (key_str != NULL) {
        // 使用 strtol 函数将字符串转换为整数
        char *endptr;
        key = strtol(key_str, &endptr, 10);
        if (*endptr != '\0') {
            printf("Conversion error, invalid characters in SHM_KEY\n");
        } //else printf("Value of KEY environment variable (as integer): %ld\n", key);
    } else {
        printf("SHM_KEY environment variable not found\n");
        key=2222;//for test
    }

    gshmid = shmget(key, 1024, 0666);
    if (gshmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
}

// Compare two timespec structs
int compare_timespec(struct timespec t1, struct timespec t2) {
    if (t1.tv_sec > t2.tv_sec) {
        return 1;
    } else if (t1.tv_sec < t2.tv_sec) {
        return -1;
    } else {
        if (t1.tv_nsec > t2.tv_nsec) {
            return 1;
        } else if (t1.tv_nsec < t2.tv_nsec) {
            return -1;
        } else {
            return 0; // Equal
        }
    }
}

// Add two timespec structs
struct timespec add_timespec(struct timespec t1, struct timespec t2) {
    struct timespec result;
    result.tv_sec = t1.tv_sec + t2.tv_sec;
    result.tv_nsec = t1.tv_nsec + t2.tv_nsec;

    if (result.tv_nsec >= 1000000000) { // Check for carry in nanoseconds
        result.tv_sec++; // Add 1 second
        result.tv_nsec -= 1000000000; // Subtract 1 billion nanoseconds
    }

    return result;
}

// Subtract timespec structs (end - start)
struct timespec subtract_timespec(struct timespec start, struct timespec end) {
    struct timespec result;

    if (end.tv_nsec < start.tv_nsec) {
        result.tv_sec = end.tv_sec - start.tv_sec - 1;
        result.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        result.tv_sec = end.tv_sec - start.tv_sec;
        result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return result;
}

// Divide timespec by a double
struct timespec divide_timespec(struct timespec dividend, double divisor) {
    struct timespec result;
    unsigned long long total_nsecs_dividend, total_nsecs_result;

    // Convert dividend to total nanoseconds
    total_nsecs_dividend = (unsigned long long)dividend.tv_sec * 1000000000ULL + dividend.tv_nsec;

    // Perform division in nanoseconds
    total_nsecs_result = total_nsecs_dividend / divisor;

    // Extract seconds and nanoseconds from the result
    result.tv_sec = total_nsecs_result / 1000000000ULL;
    result.tv_nsec = total_nsecs_result % 1000000000ULL;

    return result;
}

static long hook_function(long a1, long a2, long a3,
			  long a4, long a5, long a6,
			  long a7)
{
	// printf("output from hook_function: syscall number %ld\n", a1);
    // printf("a1: %ld a2: %ld a3: %ld a4: %ld a5: %ld a6: %ld a7: %ld\n", a1,a2,a3,a4,a5,a6,a7);

    /*-----time()-----*/
	if(a1==201){
		long Rtime=next_sys_call(a1, a2, a3, a4, a5, a6, a7);
		long Vtime=getVcurrtime(Rtime);
        if(a2!=0){
            time_t *ptr = (time_t *)a2;
            *ptr = Vtime;
        }
		return Vtime;
	}

    /*-----gettimeofday()-----*/
    if(a1==96){
        next_sys_call(a1, a2, a3, a4, a5, a6, a7);
        struct timeval *ptr = (struct timeval *)a2;
        long Rtime= ptr->tv_sec;      //attention:还没处理微秒哈！！！
        long Vtime=getVcurrtime(Rtime);
        ptr->tv_sec = Vtime;
        return 0;
    }

    /*-----clock_gettime()-----*/
    if(a1==228){
        next_sys_call(a1, a2, a3, a4, a5, a6, a7);
        printf("in clock_gettime(), a1: %ld a2: %ld a3: %ld a4: %ld a5: %ld a6: %ld a7: %ld\n", a1,a2,a3,a4,a5,a6,a7);
        if(a2==0){//CLOCK_REALTIME
            struct timespec *ptr = (struct timespec *)a3;
            getVcurrtimeNano(ptr);
        }else{//a2=1,CLOCK_MONOTONIC,暂不处理
            ;
        }
        return 0;
    }

    /*-----settimeofday()-----*/
    if(a1==164){
        printf("settimeofday() is not permitted here\n");
        return 0;
    }

    /*-----clock_nanosleep()-----*/
    if(a1==230){
        //printf("in clock_nanosleep, a1: %ld a2: %ld a3: %ld a4: %ld a5: %ld a6: %ld a7: %ld\n", a1,a2,a3,a4,a5,a6,a7);
        struct timespec *ptr = (struct timespec *)a4;
        if(a2==0){
            if(a3==0){//a2=0,a3=0,主要情况           
                getVfuturetime(ptr);
            }else{//attention: 没测试,需注意
                struct timespec current_time;
                if (clock_gettime(CLOCK_REALTIME, &current_time) == -1) {
                    perror("clock_gettime");
                    return 1;
                }
                ptr->tv_sec -= current_time.tv_sec;
                ptr->tv_nsec -= current_time.tv_nsec;
                getVfuturetime(ptr);
                a3=0;
            }
        }else{//attention: a2=1,不知道怎么处理，什么是系统单调时钟
            ;
        }
        printf("new sleep time_spec: %ld seconds, %ld nanoseconds\n", ptr->tv_sec, ptr->tv_nsec);
        next_sys_call(a1, a2, a3, a4, a5, a6, a7);      
        return 0;
    }

	//执行原来的syscall
	return next_sys_call(a1, a2, a3, a4, a5, a6, a7);
}

int __hook_init(long placeholder __attribute__((unused)),
		void *sys_call_hook_ptr)
{
    //获取shmid
    initShareMem();
	//保存原来的syscall
	next_sys_call = *((syscall_fn_t *) sys_call_hook_ptr);
	//把hook_function替代原来的syscall
	*((syscall_fn_t *) sys_call_hook_ptr) = hook_function;

	return 0;
}

static long getVcurrtime(long Rtime){
    printf("-----\nRtime: %ld\n", Rtime);
    
    // 读取共享内存
    int entryNum = 0;
    struct VTimeEntry* vTimeEntries = readShareMem(&entryNum);

    // 打印读取的数据
    printf("Read %d entries from shared memory:\n", entryNum);
    for (int i = 0; i < entryNum; i++) {
        printf("Entry %d: StartTimeSec=%lld, StartTimeNanosec=%lld, Rate=%lf\n",
               i + 1, vTimeEntries[i].StartTimeSec, vTimeEntries[i].StartTimeNanosec, vTimeEntries[i].Rate);
    }

    // 虚拟时间计算
    // attention, 这里只计算了秒
    long startPoint = vTimeEntries[0].StartTimeSec; // 记录vt0
    double VTpast = 0.0;

    for (int i = 0; i < entryNum; ++i){
        if (vTimeEntries[i].StartTimeSec >= Rtime) break;
        // 记录当前segment的结束时间点
        long endTime;
        if(i == entryNum - 1){
            endTime = Rtime;
        } else {
            //判断Rtime和nextST哪个大
            long nextST = vTimeEntries[i + 1].StartTimeSec;
            endTime = (nextST < Rtime) ? nextST : Rtime;
        }
        VTpast += (endTime - vTimeEntries[i].StartTimeSec) / vTimeEntries[i].Rate;
    }

	long Vtime=startPoint+VTpast;// attention:这里是long=long+double，相当于截断小数，注意一下
    printf("-----\nVirtual time past: %f, Virtual time: %ld\n", VTpast, Vtime);

	return Vtime;

}

static void getVcurrtimeNano(struct timespec *Rtime){
    printf("-----\nRtime:sec: %ld,nanosec: %ld\n", Rtime->tv_sec, Rtime->tv_nsec);
    
    // 读取共享内存
    int entryNum = 0;
    struct VTimeEntry* vTimeEntries = readShareMem(&entryNum);

    // 打印读取的数据
    printf("Read %d entries from shared memory:\n", entryNum);
    for (int i = 0; i < entryNum; i++) {
        printf("Entry %d: StartTimeSec=%lld, StartTimeNanosec=%lld, Rate=%lf\n",
               i + 1, vTimeEntries[i].StartTimeSec, vTimeEntries[i].StartTimeNanosec, vTimeEntries[i].Rate);
    }

    // 记录startpoint
    struct timespec startPoint;
    startPoint.tv_sec = vTimeEntries[0].StartTimeSec; 
    startPoint.tv_nsec = vTimeEntries[0].StartTimeNanosec; 

    // 排除startPoint比现在时间大的情况
    if(compare_timespec(startPoint,*Rtime) > 0) return;

    // 记录虚拟时间计算值
    struct timespec VTpast = {0, 0};

    for (int i = 0; i < entryNum; ++i){
        struct timespec currEntry;
        currEntry.tv_sec = vTimeEntries[i].StartTimeSec; 
        currEntry.tv_nsec = vTimeEntries[i].StartTimeNanosec; 

        if (compare_timespec(currEntry,*Rtime) >= 0) break;
        // 记录当前segment的结束时间点
        struct timespec endTime;
        if(i == entryNum - 1){
            endTime = *Rtime;
        } else {
            //判断Rtime和nextEntry哪个大
            struct timespec nextEntry;
            nextEntry.tv_sec = vTimeEntries[i+1].StartTimeSec; 
            nextEntry.tv_nsec = vTimeEntries[i+1].StartTimeNanosec; 
            endTime = (compare_timespec(nextEntry,*Rtime) <= 0) ? nextEntry : *Rtime;
        }
        // 算出当前区间有效时间长度，除以TDF，并叠加
        struct timespec currValidTime = subtract_timespec(currEntry,endTime);
        struct timespec currValidVT = divide_timespec(currValidTime, vTimeEntries[i].Rate);
        VTpast = add_timespec(VTpast,currValidVT);
    }

	struct timespec Vtime = add_timespec(startPoint,VTpast);
    printf("-----\nVirtual time past: %ld, nanosec: %ld\n", VTpast.tv_sec, VTpast.tv_nsec);
    printf("-----\nVirtual time : %ld, nanosec: %ld\n", Vtime.tv_sec, Vtime.tv_nsec);

	*Rtime = Vtime;

}

//attention: 还没处理微秒哈
static void getVfuturetime(struct timespec *sleeptime){
    long Rtime=time(NULL);
    printf("-----\nRtime: %ld,sleeplength: %ld\n", Rtime, sleeptime->tv_sec);

    // 读取共享内存
    int entryNum = 0;
    struct VTimeEntry* vTimeEntries = readShareMem(&entryNum);

    // 打印读取的数据
    printf("Read %d entries from shared memory:\n", entryNum);
    for (int i = 0; i < entryNum; i++) {
        printf("Entry %d: StartTimeSec=%lld, StartTimeNanosec=%lld, Rate=%lf\n",
               i + 1, vTimeEntries[i].StartTimeSec, vTimeEntries[i].StartTimeNanosec, vTimeEntries[i].Rate);
    }

    long sleepEnd = Rtime + sleeptime->tv_sec;
    double newSleepLength = 0.0;

    printf("-----\nRtime: %ld,sleepEnd: %ld\n", Rtime,sleepEnd);

    // 遍历数组，找到Rtime和sleepEnd所在的时间区间
    int startIdx = 0;
    int endIdx = entryNum;
    for (int i = 0; i < entryNum; ++i) {
        double segST=vTimeEntries[i].StartTimeSec;
        if (Rtime >= segST) {
            startIdx = i;
        }
        if (sleepEnd <= segST) {
            endIdx = i;
            break;
        }
    }

    // 从Rtime到sleepEnd开始计算新的Sleeplength
    for (int i = startIdx; i < endIdx; ++i) {
        double currentRate = vTimeEntries[i].Rate;
        double segmentStart = (Rtime >= vTimeEntries[i].StartTimeSec) ? Rtime : vTimeEntries[i].StartTimeSec;
        double segmentEnd = ( i == entryNum-1 || sleepEnd <= vTimeEntries[i + 1].StartTimeSec) ? sleepEnd : vTimeEntries[i + 1].StartTimeSec;
        printf("segmentStart: %f,segmentEnd: %f\n", segmentStart,segmentEnd);
        newSleepLength += (segmentEnd - segmentStart) / currentRate;
    }

    printf("-----\nnewSleeplength: %f\n", newSleepLength);

    sleeptime->tv_sec = (long)newSleepLength;

}
