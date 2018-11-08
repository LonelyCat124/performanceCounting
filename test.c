#include <papi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 16777216

unsigned long wrap_gettid( void ){
  return (unsigned long) omp_get_thread_num();
}


int main(){

  int retval;
  unsigned long int tid;
  int *array;

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  array = malloc(sizeof(int)*N);
  #pragma omp parallel default(none) shared(array)
{
  /*if(omp_get_thread_num() == 21){*/
    #pragma omp for schedule(static)
    for(int i = 0; i < N; i++){
      array[i] = i;
    }
//  }
}
 

  #pragma omp parallel default(none) private(retval)
  {
    #pragma omp master
    {
      retval = PAPI_thread_init(pthread_self);
    }

  }

 /* int code;
  retval = PAPI_event_name_to_code("OFFCORE_REQUESTS_OUTSTANDING", &code) ;
  printf("Error %s\n", PAPI_strerror(retval));*/
  int events[] = {PAPI_SR_INS, PAPI_MEM_WCY};
// int events[] = {PAPI_L3_TCW, code};
  int size_events = 2;
  #pragma omp parallel default(none) shared(array, events, size_events)
  {
    long long values[size_events];
    values[0] = 0; values[1] = 0;
    int ret = PAPI_start_counters(events, size_events);
    printf("%s\n", PAPI_strerror(ret));
    //#pragma omp for schedule(static)
    #pragma omp for schedule(dynamic, 32)
    for(int i = 0; i < N; i++){
      array[i] = array[i] + 1;
    }
    ret = PAPI_stop_counters(values, size_events);
    if( ret != PAPI_OK) printf("%s\n", PAPI_strerror(ret));
    for(int i = 0; i < omp_get_num_threads(); i++){
      #pragma omp barrier
      if(i == omp_get_thread_num())
        printf("Thread %i counters: [ %lli %lli ]\n", omp_get_thread_num(), values[0], values[1]);
    }
  }


}
