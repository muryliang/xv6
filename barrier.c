#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

// #define SOL

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

/*
 * one thread come in:
 *  lock mutex,
 *  while(1) {
 *      if decing = 1; continue wait
 *      else wake up and out
 *  }
 *  nthread++
 *  cond_wait
 *
 *  when last thread came in :
 *      lock mutext
 *      nthread++, barrier.nthread  = global.nthread, then round++, decing = 1
 *      broadcast,(if one thread fast, next round, will get wrong round number, so round++shoud happen before broadcast)
 *
 *      method:
 *          when signaled, lock again, nthread--
 */
static void 
barrier()
{
    static int state = 0; // 0 inc, 1 dec

    // first step: if decreasing, wait until state become 0 and nthread  become 0 too
    pthread_mutex_lock(&bstate.barrier_mutex);
    while (state == 1) {
        pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    }
    /*
    pthread_mutex_unlock(&bstate.barrier_mutex);

    pthread_mutex_lock(&bstate.barrier_mutex);
    */
    if (++bstate.nthread < nthread) {
        pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    } else {
        state = 1; // forbidden waked up thread again increase nthread
        bstate.round++;
        pthread_cond_broadcast(&bstate.barrier_cond);
    }
    // decreasing step
    if (!--bstate.nthread) {
        state = 0;
        pthread_cond_broadcast(&bstate.barrier_cond);
    }
    pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
