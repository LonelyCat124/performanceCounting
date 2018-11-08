#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <errno.h>
#include <error.h>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <pthread.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <hwloc.h>

#define N 16777216

static long perf_event_open_mine(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags){
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}


int main(){

  int CPUS;

  hwloc_topology_t sTopology;

  if (hwloc_topology_init(&sTopology) == 0 &&
    hwloc_topology_load(sTopology) == 0){
      CPUS = hwloc_get_nbobjs_by_type(sTopology, HWLOC_OBJ_CORE);
      hwloc_topology_destroy(sTopology);
  }
  omp_set_num_threads(CPUS);
  int retval;
  unsigned long int tid;
  int *array;
  int handles[CPUS];
  int handles2[CPUS];
  int handles3[CPUS];
  int handles4[CPUS];
  pfm_initialize();

  struct perf_event_attr pe[CPUS];
  struct perf_event_attr pe2[CPUS];
  struct perf_event_attr pe3[CPUS];
  struct perf_event_attr pe4[CPUS];
  for(int i =0; i < CPUS; i++){
  memset(&pe[i],0,sizeof(struct perf_event_attr));
  memset(&pe2[i],0,sizeof(struct perf_event_attr));
  memset(&pe3[i],0,sizeof(struct perf_event_attr));
  memset(&pe4[i],0,sizeof(struct perf_event_attr));

//  pe[i].type = PERF_TYPE_HARDWARE;
//  pe[i].type = PERF_TYPE_RAW;
  pe[i].type = PERF_TYPE_HW_CACHE;
  pe[i].config = ( PERF_COUNT_HW_CACHE_NODE ) | ( PERF_COUNT_HW_CACHE_OP_READ << 8 ) | ( PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16 );
  pe[i].size = sizeof(struct perf_event_attr);
  pe[i].disabled =1;
  pe[i].exclude_kernel=1;
  pe[i].exclude_hv=1;

/*  pe2[i].type = PERF_TYPE_HARDWARE;
  pe2[i].config = PERF_COUNT_HW_CACHE_MISSES;*/
  pe2[i].type  = PERF_TYPE_HW_CACHE;
  pe2[i].config = ( PERF_COUNT_HW_CACHE_NODE ) | ( PERF_COUNT_HW_CACHE_OP_READ << 8 ) | ( PERF_COUNT_HW_CACHE_RESULT_MISS << 16 );
  pe2[i].size = sizeof(struct perf_event_attr);
  pe2[i].disabled =1;
  pe2[i].exclude_kernel=1;
  pe2[i].exclude_hv=1;
  
  pe3[i].type == PERF_TYPE_RAW;
  pfm_get_perf_event_encoding("OFFCORE_REQUESTS:L3_MISS_DEMAND_DATA_RD",PFM_PLM3,&pe3[i],NULL,NULL );
  pe3[i].size = sizeof(struct perf_event_attr);
  pe3[i].disabled =1;
  pe3[i].exclude_kernel=1;
  pe3[i].exclude_hv=1;

  pe4[i].type == PERF_TYPE_RAW;
  int x = pfm_get_perf_event_encoding("OFFCORE_RESPONSE_0:DMND_DATA_RD:L3_MISS_LOCAL:SNP_ANY",PFM_PLM3,&pe4[i],NULL,NULL);
  if(x != PFM_SUCCESS) printf("%s\n", pfm_strerror(x));
  pe4[i].size = sizeof(struct perf_event_attr);
  pe4[i].disabled =1;
  pe4[i].exclude_kernel=1;
  pe4[i].exclude_hv=1;
}

  array = _mm_malloc(sizeof(int)*N,512);
  #pragma omp parallel default(none) shared(array)
{
//  if(omp_get_thread_num() == 21){
    #pragma omp for schedule(static)
//    #pragma omp master
    for(int i = 0; i < N; i++){
      array[i] = i;
    }
//  }
}
 
  #pragma omp parallel default(none) shared(array, handles, stderr,pe,pe2, handles2, pe3,pe4,handles3,handles4,CPUS)
  {
    cpu_set_t cpuset;
    int core_id = omp_get_thread_num();
    pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if(CPU_COUNT(&cpuset) > 1) printf("Affinity not set\n");
    for(int i = 0; i < CPUS; i++){
      if(CPU_ISSET(i, &cpuset)){
        core_id = i;
        break;
      }
    }
    for(int i = 0; i < omp_get_num_threads(); i++){
      if(i == omp_get_thread_num()){
        handles[core_id] = perf_event_open_mine(&pe[core_id],0,core_id,-1,0);
        handles2[core_id] = perf_event_open_mine(&pe2[core_id],0,core_id,-1,0);
        handles3[core_id] = perf_event_open_mine(&pe3[core_id],0,core_id,-1,0);
        handles4[core_id] = perf_event_open_mine(&pe4[core_id],0,core_id,-1,0);
        if(handles[core_id]  == -1 || handles2[core_id] == -1 || handles3[core_id] == -1 || handles4[core_id] == -1){
          fprintf(stderr, "%i %i %i %i\n", handles[core_id], handles2[core_id], handles3[core_id], handles4[core_id]);
          fprintf(stderr,"error opening %i %i\n", core_id,errno);
          exit(EXIT_FAILURE);
        }
      }
    #pragma omp barrier
    }
    if( ioctl(handles[core_id],PERF_EVENT_IOC_RESET,0) == -1){
      exit(EXIT_FAILURE);
    }
    if( ioctl(handles2[core_id],PERF_EVENT_IOC_RESET,0) == -1){
      exit(EXIT_FAILURE);
    }
    if( ioctl(handles3[core_id],PERF_EVENT_IOC_RESET,0) == -1){
      exit(EXIT_FAILURE);
    }
    if( ioctl(handles4[core_id],PERF_EVENT_IOC_RESET,0) == -1){
      exit(EXIT_FAILURE);
    }
    printf("Setup %i\n", omp_get_thread_num());
    ioctl(handles[core_id],PERF_EVENT_IOC_ENABLE,0);
    ioctl(handles2[core_id],PERF_EVENT_IOC_ENABLE,0);
    ioctl(handles3[core_id],PERF_EVENT_IOC_ENABLE,0);
    ioctl(handles4[core_id],PERF_EVENT_IOC_ENABLE,0);
    #pragma omp for simd schedule(static) aligned(array:32)
//    #pragma omp for schedule(dynamic, 32)
    for(int i = 0; i < N; i++){
      array[i] = array[i] + 1;
    }
    ioctl(handles[core_id],PERF_EVENT_IOC_DISABLE,0);
    ioctl(handles2[core_id],PERF_EVENT_IOC_DISABLE,0);
    ioctl(handles3[core_id],PERF_EVENT_IOC_DISABLE,0);
    ioctl(handles4[core_id],PERF_EVENT_IOC_DISABLE,0);
    long long count=-1, count2=-1,count3=-1, count4=-1;
    ssize_t z = read(handles[core_id], &count, sizeof(long long));
    ssize_t z2 = read(handles2[core_id], &count2, sizeof(long long));
    ssize_t z3 = read(handles3[core_id], &count3, sizeof(long long));
    ssize_t z4 = read(handles4[core_id], &count4, sizeof(long long));
    for(int i = 0; i < omp_get_num_threads(); i++){
      #pragma omp barrier
      if(i == omp_get_thread_num())
        printf("%i %i: %lli %lli %lli %lli \n",omp_get_thread_num(), core_id, count, count2, count3, count4);
    }
  }
  _mm_free(array);

}
