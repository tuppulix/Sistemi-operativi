#include "lib-misc.h"
#include <complex.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define buffer_size 4096

typedef enum { R, P, W } thread_name;

typedef struct {
  char buffer[buffer_size];
  bool done;
  sem_t sem[3];
} shared;

typedef struct {
  char *filename;
  pthread_t tid;
  shared *share;
} thread_strutt;

bool reverse_chars(char*s) {
  for (int i = 0; i < strlen(s); i++)
        if (s[i] != s[strlen(s) - 1 - i])
            return 0;

    return 1;
}

void shared_init(shared*sh) {

  int err;
  
  sh->done = 0;

  if ((err = sem_init(&sh->sem[R], 0, 1))!=0)
    exit_with_err("init", err);

  if ((err = sem_init(&sh->sem[P], 0, 0))!=0)
    exit_with_err("init", err);

  if ((err = sem_init(&sh->sem[W], 0, 0))!=0)
    exit_with_err("init", err);

}

void semaphore_destory(shared *sh) {
  int err;
  for (int i = 0; i < 3; i++) {
    if ((err = sem_destroy(&sh->sem[i]) != 0))
      exit_with_err("destroy", err);
  }
  free(sh);
}

void erre(void *arg) {
  int err;
  thread_strutt *td = (thread_strutt *)arg;
  FILE*fd;
  char buffer[buffer_size];

  if ((fd = fopen(td->filename, "r")) == NULL)
    exit_with_err("fopen", err);

  while (fgets(buffer, buffer_size, fd)) {
    if ((err = sem_wait(&td->share->sem[R])))
      exit_with_err("wait", err);

    if(buffer[strlen(buffer)-1]=='\n')
       buffer[strlen(buffer)-1]='\0';

    strncpy(td->share->buffer, buffer, buffer_size);

    if ((err = sem_post(&td->share->sem[P])) != 0)
      exit_with_err("sem_post", err);

  }
   if ((err = sem_wait(&td->share->sem[R])) != 0)
        exit_with_err("sem_wait", err);

    td->share->done = 1;

    if ((err = sem_post(&td->share->sem[P])) != 0)
        exit_with_err("sem_post", err);

    if ((err = sem_post(&td->share->sem[W])) != 0)
        exit_with_err("sem_post", err);

    fclose(fd);
}

void pi(void *arg) {
  int err;
  thread_strutt *td = (thread_strutt *)arg;

  while (1) {
    if ((err = sem_wait(&td->share->sem[P])) != 0)
      exit_with_err("sem_wait", err);

    if (td->share->done)
      break;

    if (reverse_chars(td->share->buffer)){
      if ((err = sem_post(&td->share->sem[W]))!=0)
        exit_with_err("sem_post_W", err);
    } else {
      if ((err = sem_post(&td->share->sem[R]))!=0)
        exit_with_err("sem_post_R", err);
    }
  }
}

void doppiaw(void *arg) {
  int err;
  thread_strutt *td = (thread_strutt *)arg;

  while (1) {
    if ((err = sem_wait(&td->share->sem[W])) != 0)
      exit_with_err("sem_wait_W", err);

    if (td->share->done)
      break;

    printf("%s\n", td->share->buffer);

    if ((err = sem_post(&td->share->sem[R])) != 0)
      exit_with_err("sem_post", err);
  }
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("errore: inserire %s e <file.txt>", argv[0]);
    exit(EXIT_FAILURE);
  }
  int err;
  thread_strutt td[3];
  shared *sh = malloc(sizeof(shared));
  shared_init(sh);

  for (int i = 0; i < 3; i++) 
    td[i].share = sh;

  td[R].filename=argv[1];

  

  if ((err = pthread_create(&td[R].tid, NULL, (void *)erre, (void *)&td[R])) != 0)
    exit_with_err("create", err);
  if ((err = pthread_create(&td[P].tid, NULL, (void *)pi, (void *)&td[P])) != 0)
    exit_with_err("create", err);
  if ((err = pthread_create(&td[W].tid, NULL, (void *)doppiaw, (void *)&td[W])) != 0)
    exit_with_err("create", err);


  for (int i = 0; i < 3; i++) {
    if ((err = pthread_join(td[i].tid, NULL)) != 0)
      exit_with_err("join", err);
  }

  semaphore_destory(sh);

  exit(EXIT_SUCCESS);
}
