#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#define TABSIZE 10
#define MUTEX_KEY 1
#define MUTEX_AC_KEY 2
#define MUTEX_B_KEY 3
#define EMPTY_KEY 4
#define FULL_KEY 5
#define ELEM_PTR 6

static struct sembuf buf; // for semop
int progDuration;

struct Elements
{
  int elems[TABSIZE];
  int read[TABSIZE];
  int length, first, last;
};

void init(struct Elements *e);
void up(int semId);
void down(int semId);
void add(struct Elements *e, int val);
int get(struct Elements *e);
void print(struct Elements *e);
void producer();
void consumer(int id);

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    progDuration = atoi(argv[1]);
  }
  else
  {
    progDuration = 30;
  }

  int buffId = shmget(ELEM_PTR, sizeof(struct Elements), IPC_CREAT | 0600);
  struct Elements *elemPtr = (struct Elements *)shmat(buffId, NULL, 0);

  init(elemPtr); // initialize buffer

  int mutex = semget(MUTEX_KEY, 1, IPC_CREAT | 0600);
  if (mutex == -1)
  {
    perror("error mutex");
    exit(1);
  }
  semctl(mutex, 0, SETVAL, (int)1);

  int mutexAC = semget(MUTEX_AC_KEY, 1, IPC_CREAT | 0600);
  if (mutexAC == -1)
  {
    perror("error mutexAC");
    exit(1);
  }
  semctl(mutexAC, 0, SETVAL, (int)1);

  int mutexB = semget(MUTEX_B_KEY, 1, IPC_CREAT | 0600);
  if (mutexB == -1)
  {
    perror("error mutexB");
    exit(1);
  }
  semctl(mutexB, 0, SETVAL, (int)1);

  int empty = semget(EMPTY_KEY, 1, IPC_CREAT | 0600);
  if (empty == -1)
  {
    perror("error empty");
    exit(1);
  }
  semctl(empty, 0, SETVAL, (int)TABSIZE);

  int full = semget(FULL_KEY, 1, IPC_CREAT | 0600);
  if (full == -1)
  {
    perror("error full");
    exit(1);
  }
  semctl(full, 0, SETVAL, (int)0);

  pid_t pid[4];

  pid[0] = fork();
  if (pid[0] == 0)
  {
    producer();
  }

  for (int i = 1; i < 4; ++i)
  {
    pid[i] = fork();

    if (pid[i] == 0)
    {
      consumer(i);
    }
  }

  sleep(progDuration);

  for (int i = 0; i < 4; ++i)
  {
    kill(pid[i], SIGKILL);
  }

  shmdt(elemPtr);

  return 0;
}

/* ____________________ FUNCTIONS ____________________ */

void init(struct Elements *e)
{
  e->length = 0;
  e->first = 0;
  e->last = 0;
}

void up(int semId)
{
  buf.sem_num = 0;
  buf.sem_op = 1;
  buf.sem_flg = 0;

  if (semop(semId, &buf, 1) == -1)
  {
    perror("error up");
    exit(1);
  }
}

void down(int semId)
{
  buf.sem_num = 0;
  buf.sem_op = -1;
  buf.sem_flg = 0;

  if (semop(semId, &buf, 1) == -1)
  {
    perror("error down");
    exit(1);
  }
}

void add(struct Elements *e, int val)
{
  printf("P   adds %8d", val);

  e->elems[e->last] = val;
  e->read[e->last] = 0;
  e->last = (e->last + 1) % TABSIZE;
  e->length++;
}

int get(struct Elements *e)
{
  if (e->read[e->first] == 1)
  {
    printf(" eats %8d", e->elems[e->first]);

    int tmp = e->elems[e->first];
    e->first = (e->first + 1) % TABSIZE;
    e->length--;
    return tmp;
  }

  printf(" tries %7d", e->elems[e->first]);

  e->read[e->first] = 1;
  return -1;
}

void print(struct Elements *e)
{
  int tmp = e->first;

  printf("%12d element(s) : ", e->length);

  for (int i = 0; i < e->length; ++i, ++tmp)
  {
    printf("%d ", e->elems[tmp % TABSIZE]);
  }

  printf("\n");
}

void producer()
{
  int buffId = shmget(ELEM_PTR, sizeof(struct Elements), 0600);
  struct Elements *elemPtr = (struct Elements *)shmat(buffId, NULL, 0);
  int mutex = semget(MUTEX_KEY, 1, 0600);
  int empty = semget(EMPTY_KEY, 1, 0600);
  int full = semget(FULL_KEY, 1, 0600);
  int prodId = 0;

  while (1)
  {
    sleep(rand() % 2);
    down(empty);
    down(mutex);
    add(elemPtr, prodId);
    print(elemPtr);
    up(mutex);
    up(full);
    ++prodId; // new id of element
  }
}

void consumer(int id)
{
  int buffId = shmget(ELEM_PTR, sizeof(struct Elements), 0600);
  struct Elements *elemPtr = (struct Elements *)shmat(buffId, NULL, 0);
  int mutex = semget(MUTEX_KEY, 1, 0600);
  int empty = semget(EMPTY_KEY, 1, 0600);
  int full = semget(FULL_KEY, 1, 0600);
  int mutexB = semget(MUTEX_B_KEY, 1, 0600);
  int mutexAC = semget(MUTEX_AC_KEY, 1, 0600);

  if (id == 1 || id == 3)
  { // consumer 1 and 3
    while (1)
    {
      sleep(rand() % 4);
      down(mutexAC);
      down(full);
      down(mutex);

      printf("C%d ", id);

      if (get(elemPtr) != -1)
      {
        print(elemPtr);
        up(mutexAC);
        up(mutexB);
        up(empty);
      }
      else
      {
        print(elemPtr);
        up(full);
      }
      up(mutex);
    }
  }

  if (id == 2)
  { // consumer 2
    while (1)
    {
      sleep(rand() % 3);
      down(mutexB);
      down(full);
      down(mutex);

      printf("C%d ", id);

      if (get(elemPtr) != -1)
      {
        print(elemPtr);
        up(mutexAC);
        up(mutexB);
        up(empty);
      }
      else
      {
        print(elemPtr);
        up(full);
      }
      up(mutex);
    }
  }
}
