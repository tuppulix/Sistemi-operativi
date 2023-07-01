#include "lib-misc.h"
#include <bits/pthreadtypes.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 26 //buffer size for english alphabet

typedef enum { AL, MZ, P } pthread_name; //name of thread

//struct shared
typedef struct {
  char c;
  unsigned long stats[BUFFER_SIZE];
  bool done;
  sem_t sem[3];
} shared;

//struct for single thread
typedef struct {
  pthread_t tid;
  char thread_n;
  shared *share;
} pthread_strutt;

//function to inizialize shared
shared *shared_init() {
  int err;
  shared *sh = malloc(sizeof(shared));

  sh->done = 0;

  memset(sh->stats, 0, BUFFER_SIZE);

  if ((err = sem_init(&sh->sem[P], 0, 1)) != 0)
    exit_with_err("sem_p", err);

  if ((err = sem_init(&sh->sem[AL], 0, 0)) != 0)
    exit_with_err("sem_al", err);

  if ((err = sem_init(&sh->sem[MZ], 0, 0)) != 0)
    exit_with_err("sem_mz", err);

  return sh;
}
//destory semaphore
void shared_destroy(shared *sh) {
  for (int i = 0; i < 3; i++) {
    sem_destroy(&sh->sem[3]);
  }
  free(sh);
}
//function for thread
void stats(void *arg) {
  int err;
  pthread_strutt *td = (pthread_strutt *)arg;

  while (1) {
    if ((err = sem_wait(&td->share->sem[td->thread_n])) != 0)
      exit_with_err("sem_wait", err);

    if (td->share->done == 1)
      break;

    td->share->stats[td->share->c - 'a']++;

    if ((err = sem_post(&td->share->sem[P])) != 0)
      exit_with_err("sem_post", err);
  }
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("use %s, <file_name.txt>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int err, fd;
  pthread_strutt td[2];
  shared *sh = shared_init();
  char *map;
  struct stat s_file;

  for (int i = 0; i < 2; i++) {
    td[i].share = sh;
    td[i].thread_n = i;
  }

  if ((err = pthread_create(&td[AL].tid, NULL, (void *)stats,
                            (void *)&td[AL])) != 0)
    exit_with_err("thread_create", err);

  if ((err = pthread_create(&td[MZ].tid, NULL, (void *)stats,
                            (void *)&td[MZ])) != 0)
    exit_with_err("thread_create", err);

  if ((fd = open(argv[1], O_RDONLY)) == -1)
    exit_with_err("open", fd);

  if ((err = fstat(fd, &s_file)) == -1)
    exit_with_err("fstat", err);

  if ((map = mmap(NULL, s_file.st_size, PROT_READ, MAP_SHARED, fd, 0)) ==
      MAP_FAILED)
    exit_with_err("mmap", err);

  if ((err = close(fd)) == -1)
    exit_with_err("close", err);

  int i = 0;
  while (i < s_file.st_size) {
    if ((map[i] >= 'a' && map[i] <= 'z') || (map[i] >= 'A' && map[i] <= 'Z')) {
      if ((err = sem_wait(&sh->sem[P])) != 0)
        exit_with_err("sem_wait", err);

      sh->c = tolower(map[i]);

      if (map[i] <= 'l') {
        if ((err = sem_post(&sh->sem[AL])) != 0)
          exit_with_err("post", err);
      } else {
        if ((err = sem_post(&sh->sem[AL])) != 0)
          exit_with_err("post", err);
      }
    }
    i++;
  }

  if ((err = sem_post(&sh->sem[P])) != 0)
    exit_with_err("post", err);

  printf("Statistiche sul file %s\n", argv[1]);
  printf("\n");

  for (int i = 0; i < BUFFER_SIZE; i++) {
    printf("%c: %lu\t", i + 'a', sh->stats[i]);
  }
  printf("\n");

  sh->done = 1;
  if ((err = sem_post(&sh->sem[AL])) != 0)
    exit_with_err("post", err);

  if ((err = sem_post(&sh->sem[MZ])) != 0)
    exit_with_err("post", err);

  for (int i = 0; i < 2; i++) {
    if ((pthread_join(td[i].tid, NULL)) != 0)
      exit_with_err("join", err);
  }

  shared_destroy(sh);

  if ((err = munmap(map, s_file.st_size)) == -1)
    exit_with_err("munmap", err);

  exit(EXIT_SUCCESS);
}