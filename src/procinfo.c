#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static void usage(const char *a){
    fprintf(stderr,"Usage: %s <pid>\n",a);
    exit(1);
}

static int isnum(const char*s){
    for(;*s;s++)
        if(!isdigit(*s)) return 0;
    return 1;
}

int main(int c,char**v){
    if(c!=2 || !isnum(v[1])) usage(v[0]);

    char path[256];
    char comm[256];
    char state;
    int pid, ppid;
    long utime, stime;
    char line[256];

    snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);
    FILE *f = fopen(path, "r");
    if(!f){
        perror("open stat");
        return 1;
    }

    int ret = fscanf(f,
           "%d (%[^)]) %c %d %*d %*d %*d %*d %*d %*d "
           "%*d %*d %*d %ld %ld",
           &pid, comm, &state, &ppid, &utime, &stime);

    fclose(f);

    if (ret != 6) {
        fprintf(stderr, "Failed to parse /proc/%s/stat\n", v[1]);
        return 1;
    }

    printf("PID:%d\n", pid);
    printf("State:%c\n", state);
    printf("PPID:%d\n", ppid);

    snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
    FILE *g = fopen(path, "r");
    if(!g){
        perror("open cmdline");
        return 1;
    }

    char cmd[256];
    size_t n = fread(cmd, 1, sizeof(cmd)-1, g);
    fclose(g);

    if(n == 0){ 
        strcpy(cmd, "[kernel thread]");
    } else {
        for(size_t i=0;i<n;i++)
            if(cmd[i] == '\0') cmd[i] = ' ';
        cmd[n] = '\0';
    }

    printf("Cmd:%s\n", cmd);

    long total_ticks = utime + stime;
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double total_time = (double) total_ticks / ticks_per_sec;

    printf("CPU:%ld %.3f\n", total_ticks, total_time);

    snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
    FILE *h = fopen(path, "r");
    if(!h){
        perror("open status");
        return 1;
    }

    while(fgets(line, sizeof(line), h)){
        if(strncmp(line, "VmRSS:", 6) == 0){
            int rss;
            sscanf(line, "VmRSS: %d", &rss);
            printf("VmRSS:%d\n", rss);
            break;
        }
    }
    fclose(h);

    return 0;
}
