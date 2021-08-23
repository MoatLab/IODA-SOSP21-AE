#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include "atomic.h"

enum {
    READ = 1,
    WRITE = 0,
};

/*
 * Global variables
 */
int LARGEST_REQUEST_SIZE = 65536; /* blocks */
int MEM_ALIGN = 4096*8; /* bytes */
int nr_workers = 256;
int printlatency = 1; /* print every io latency */
int64_t maxio = 10000000; /* max # of IOs allowed in the trace */
int respecttime = 1;
int block_size = 512; /* sector size in bytes */
int64_t DISKSZ = 0;

int fd;
int64_t nr_tt_ios;
int64_t latecount = 0;
int64_t slackcount = 0;
uint64_t starttime;
void *buff;

int64_t *oft; /* array to store the offset column of the trace */
int *reqsize; /* array to store the request size column of the trace */
int *reqflag; /* array to store the request type column of the trace */
float *timestamp; /* array to store the timestamp column of thhe trace */

FILE *metrics; /* csv format: offset,size,type,latency(us) */

pthread_mutex_t lock; /* only for writing to logfile */
int64_t jobtracker = 0;

static int64_t get_disksz(int devfd)
{
    int64_t sz;

    ioctl(devfd, BLKGETSIZE64, &sz);
    printf("Disk size is %.1f MB\n", sz / 1024 / 1024.0);

    return sz;
}

void prepare_metrics(char *logfile)
{
    if (printlatency == 1 && nr_tt_ios > maxio) {
        printf("Too many I/Os in the trace file (%ld), maximum allowed: %ld!\n",
               nr_tt_ios, maxio);
        exit(EXIT_FAILURE);
    }

    if (printlatency == 1) {
        metrics = fopen(logfile, "w");
        if (!metrics) {
            printf("Error creating metrics file: %s!\n", logfile);
            exit(EXIT_FAILURE);
        }
    }
}

int64_t read_trace(char ***req, char *tracefile)
{
    char line[1024];
    int64_t nr_lines = 0, i = 0;
    int ch;

    /* first, read the number of lines */
    FILE *trace = fopen(tracefile, "r");
    if (trace == NULL) {
        printf("Cannot open trace file: %s!\n", tracefile);
        exit(EXIT_FAILURE);
    }

    while (!feof(trace)) {
        ch = fgetc(trace);
        if (ch == '\n') {
            nr_lines++;
        }
    }
    printf("The trace (%s) contains [%lu] IOs\n", tracefile, nr_lines);

    rewind(trace);

    /* then, start parsing */
    if ((*req = malloc(nr_lines * sizeof(char *))) == NULL) {
        fprintf(stderr, "Memory allocation failed (line: %d)!\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), trace) != NULL) {
        line[strlen(line) - 1] = '\0';
        if (((*req)[i] = malloc((strlen(line) + 1) * sizeof(char))) == NULL) {
            fprintf(stderr, "Memory allocation failed (line: %d)!\n", __LINE__);
            exit(EXIT_FAILURE);
        }

        strcpy((*req)[i], line);
        i++;
    }

    fclose(trace);
    return nr_lines;
}

void parse_io(char **reqs)
{
    char *one_io;
    int64_t i = 0;

    oft = malloc(nr_tt_ios * sizeof(int64_t));
    reqsize = malloc(nr_tt_ios * sizeof(int));
    reqflag = malloc(nr_tt_ios * sizeof(int));
    timestamp = malloc(nr_tt_ios * sizeof(float));
    one_io = malloc(1024);

    if (!oft || !reqsize || !reqflag || !timestamp || !one_io) {
        printf("Memory allocation failed (line: %d)!\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < nr_tt_ios; i++) {
        memset(one_io, 0, 1024);
        strcpy(one_io, reqs[i]);

        /* Column 1: Request arrival time in "ms" */
        timestamp[i] = atof(strtok(one_io, " "));
        /* Column 2: Device number (not used) */
        strtok(NULL, " ");
        /* Column 3: Offset within the device in blocks/sectors */
        oft[i] = atoll(strtok(NULL, " "));
        oft[i] *= block_size;
        oft[i] %= DISKSZ;
        /* make sure 4KB aligned */
        oft[i] = oft[i] / 4096 * 4096;
        assert(oft[i] >= 0);
        /* Column 4: Request size in blocks/sectors */
        reqsize[i] = atoi(strtok(NULL, " ")) * block_size;
        reqsize[i] = reqsize[i] / 4096 * 4096;
        /* 5. Request type: 0 for write and 1 for read */
        reqflag[i] = atoi(strtok(NULL, " "));

        //printf("%.2f,%ld,%d,%d\n", timestamp[i], oft[i], reqsize[i],reqflag[i]);
    }

    free(one_io);
}

void *perform_io()
{
    int64_t cur_idx;
    int mylatecount = 0;
    int myslackcount = 0;
    struct timeval t1, t2;
    useconds_t sleep_time;
    int ret;

    while (1) {
        cur_idx = atomic_fetch_inc(&jobtracker);
        if (cur_idx >= nr_tt_ios) {
            /* Done with all the I/Os */
            break;
        }

        myslackcount = 0;
        mylatecount = 0;

        /* Respect the I/O arrival time */
        if (respecttime == 1) {
            gettimeofday(&t1, NULL);
            int64_t elapsedtime = t1.tv_sec * 1e6 + t1.tv_usec - starttime;
            if (elapsedtime < (int64_t)(timestamp[cur_idx] * 1000)) {
                sleep_time = (useconds_t)(timestamp[cur_idx] * 1000) - elapsedtime;
                if (sleep_time > 100000) { /* need to wait for >100ms */
                    myslackcount++;
                }
                usleep(sleep_time);
            } else { /* Late for playing the I/O on time */
                mylatecount++;
            }
        }

        // do the job
        gettimeofday(&t1, NULL);
        float submission_ts = (t1.tv_sec * 1e6 + t1.tv_usec - starttime) / 1000;

        if (reqflag[cur_idx] == WRITE) {
            ret = pwrite(fd, buff, reqsize[cur_idx], oft[cur_idx]);
            if (ret < 0) {
                printf("Cannot write size %d to offset %lu! ret=%d\n",
                       reqsize[cur_idx], oft[cur_idx], ret);
            }
        } else if (reqflag[cur_idx] == READ) {
            ret = pread(fd, buff, reqsize[cur_idx], oft[cur_idx]);
            if (ret < 0) {
                printf("Cannot read size %d to offset %"PRId64", ret=%d,"
                       "errno=%d!\n", (reqsize[cur_idx] / 512), oft[cur_idx],
                       ret, errno);
            }
        } else {
            printf("Bad request type (%d)!\n", reqflag[cur_idx]);
            exit(EXIT_FAILURE);
        }

        gettimeofday(&t2, NULL);

        /* Coperd: I/O latency in us */
        int64_t lat = (t2.tv_sec - t1.tv_sec) * 1e6 + (t2.tv_usec - t1.tv_usec);
        if (printlatency == 1) {
            /*
             * Be consistent with fio latency log format:
             * 1: timestamp in ms
             * 2: latency in us
             * 3: r/w type [0 for w, 1 for r] (this is opposite of fio)
             * 4: I/O size in bytes
             * 5: offset in bytes
             */

            /* FIXME: might cause a lot of contention here */
            pthread_mutex_lock(&lock);
            fprintf(metrics, "%.3f,%ld,%d,%d,%ld,%.3f\n", timestamp[cur_idx],
                    lat, reqflag[cur_idx], reqsize[cur_idx], oft[cur_idx],
                    submission_ts);
            pthread_mutex_unlock(&lock);
        }

        atomic_add(&latecount, mylatecount);
        atomic_add(&slackcount, myslackcount);
    }

    return NULL;
}

void *pr_progress()
{
    int64_t progress, np;
    int64_t cur_late_cnt, cur_slack_cnt;

    while (1) {
        progress = atomic_read(&jobtracker);
        cur_late_cnt = atomic_read(&latecount);
        cur_slack_cnt = atomic_read(&slackcount);

        np = (progress > nr_tt_ios) ? nr_tt_ios : progress;
        printf("Progress: %.2f%% (%lu/%lu), Late rate: %.2f%% (%lu), Slack"
               "rate: %.2f%% (%lu)\r", 100 * (float)np / nr_tt_ios, progress,
               nr_tt_ios, 100 * (float)cur_late_cnt / nr_tt_ios, cur_late_cnt,
               100 * (float)cur_slack_cnt / nr_tt_ios, cur_slack_cnt);
        fflush(stdout);

        if (progress > nr_tt_ios) {
            break;
        }

        sleep(1);
    }
    printf("\n\n All done!\n");

    return NULL;
}

void do_replay(void)
{
    pthread_t track_thread;
    struct timeval t1, t2;
    float totaltime;
    int t;

    printf("\n\nStart replaying I/Os...\n");

    /* allocate worker threads */
    pthread_t *tid = malloc(nr_workers * sizeof(pthread_t));
    if (tid == NULL) {
        printf("Malloc failed at (line: %d)!\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    assert(pthread_mutex_init(&lock, NULL) == 0);

    gettimeofday(&t1, NULL);
    starttime = t1.tv_sec * 1000000 + t1.tv_usec;
    for (t = 0; t < nr_workers; t++) {
        assert(pthread_create(&tid[t], NULL, perform_io, NULL) == 0);
    }
    assert(pthread_create(&track_thread, NULL, pr_progress, NULL) == 0);

    /* Wait for all threads to finish */
    for (t = 0; t < nr_workers; t++) {
        pthread_join(tid[t], NULL);
    }
    pthread_join(track_thread, NULL); //progress

    gettimeofday(&t2, NULL);

    /* Metrics */
    totaltime = (t2.tv_sec - t1.tv_sec) * 1e3 + (t2.tv_usec - t1.tv_usec) / 1e3;
    printf("==============================\n");
    printf("Total run time: %.3f ms\n", totaltime);

    if (respecttime == 1) {
        printf("Late rate: %.2f%%\n", 100 * (float)atomic_read(&latecount) / nr_tt_ios);
        printf("Slack rate: %.2f%%\n", 100 * (float)atomic_read(&slackcount) / nr_tt_ios);
    }

    fclose(metrics);
    assert(pthread_mutex_destroy(&lock) == 0);

    //run statistics
    //system("python statistics.py");
}

int main (int argc, char **argv)
{
    char device[64];
    char tracefile[128], logfile[128];
    char **request;

    if (argc != 4) {
        printf("\n\nUsage: ./replayer [/dev/md0] [tracefile] [logfile]\n\n");
        exit(EXIT_FAILURE);
    } else {
        sprintf(device, "%s", argv[1]);
        printf("Disk ==> %s\n", device);
        sprintf(tracefile, "%s", argv[2]);
        printf("Trace ==> %s\n", tracefile);
        sprintf(logfile, "%s", argv[3]);
        printf("Logfile ==> %s\n", logfile);
    }

    fd = open(device, O_DIRECT | O_RDWR);
    if (fd < 0) {
        printf("Cannot open device: %s\n", device);
        exit(EXIT_FAILURE);
    }

    DISKSZ = get_disksz(fd);

    if (posix_memalign(&buff, MEM_ALIGN, LARGEST_REQUEST_SIZE * block_size)) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    /* Preparation work */
    nr_tt_ios = read_trace(&request, tracefile);

    parse_io(request);

    prepare_metrics(logfile);

    /* Replay */
    do_replay();

    free(buff);

    return 0;
}
