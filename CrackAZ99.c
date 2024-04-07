/*
 * To compile the program:
 *  - gcc CrackAZ99.c -o crack -lpthread -lcrypt
 *  - gcc EncryptSHA512.c -o EncryptSHA512 -lcrypt
 *
 * To execute the compiled program
 *  - ./EncryptSHA512 <your_password> for e.g ./EncryptSHA512 HP93 gives output
 *  - $6$AS$Ig.vW9RG9J5gPFUvHwyV67GdVVndF.2ROH6.qZjQN1Nm5kqn0t/FKNf4.48qRHdyAWwIQOtKkCosTrwyj3SvJ.

 *  - ./crack <num_threads> for e.g ./crack 4 gives output 
 *  - #6408    HP93 $6$AS$Ig.vW9RG9J5gPFUvHwyV67GdVVndF.2ROH6.qZjQN1Nm5kqn0t/FKNf4.48qRHdyAWwIQOtKkCosTrwyj3SvJ.
 *  -  6411 solutions explored
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <crypt.h>

int count = 0;               // Counter for the number of combinations explored
pthread_mutex_t mtx;         // Mutex for thread safety
volatile int isCracked = 0;  // Flag to signal if the password is cracked
char salt_and_encrypted[92]; // Array to store the salt and encrypted password
typedef struct
{
  char start, end; // Start and end characters for the thread's character range
} ThreadData;

void substr(char *dest, char *src, int start, int length)
{
  memcpy(dest, src + start, length);
  *(dest + length) = '\0';
}

// Thread function to crack password combinations
void *crack(void *arg)
{
  ThreadData *data = (ThreadData *)arg;
  int x, y, z;
  char salt[7];
  char plain[7];
  char *enc;
  struct crypt_data _data[1] = {0};

  substr(salt, salt_and_encrypted, 0, 6);

  for (x = data->start; x < data->end; x++)
  {
    for (y = 'A'; y <= 'Z'; y++)
    {
      for (z = 0; z <= 99 && !isCracked; z++)
      {
        sprintf(plain, "%c%c%02d", x, y, z);

        // enc = (char *)crypt(plain, salt);
        enc = crypt_r(plain, salt, _data); // using crypt_r because it is thread safe version of crypt
        pthread_mutex_lock(&mtx);
        count++;
        pthread_mutex_unlock(&mtx);
        if (strcmp(salt_and_encrypted, enc) == 0)
        {
          isCracked = 1;
          printf("#%-8d%s %s\n", count, plain, enc);
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  /*
   * Verify the arguments
   */
  if (argc != 2)
  {
    printf("Usage: '%s' <num_threads>\n", argv[0]);
    return EXIT_FAILURE;
  }

  /*
   * Get the number of threads to use, limit to 26
   */
  int num_threads = (atoi(argv[1]) <= 26) ? atoi(argv[1]) : 26;
  if (num_threads <= 0)
  {
    printf("Invalid number of threads.\n");
    return EXIT_FAILURE;
  }

  /*
   * Get the encrypted password
   */
  printf("Enter the encrypted password: ");
  scanf("%s", salt_and_encrypted);
  if (strlen(salt_and_encrypted) != 92)
  {
    printf("%ld", strlen(salt_and_encrypted));
    printf("The length of encrypted password is invalid.");
    printf("It should be 92 characters long.");
    return EXIT_FAILURE;
  }

  // Create threads
  pthread_t threads[num_threads];
  ThreadData threadData[num_threads];
  pthread_mutex_init(&mtx, NULL);
  int per_thread_interval = (('Z' - 'A') + 1) / num_threads;
  int remaining = (('Z' - 'A') + 1) % num_threads;
  char start = 'A';
  for (int i = 0; i < num_threads; i++)
  {
    threadData[i].start = start;
    threadData[i].end = start + per_thread_interval + (i < remaining ? 1 : 0);
    pthread_create(&threads[i], NULL, crack, (void *)&threadData[i]);
    start = threadData[i].end;
  }

  // Wait for threads to finish
  for (int i = 0; i < num_threads; i++)
  {
    pthread_join(threads[i], NULL);
  }

  // Result
  printf("%d solutions explored\n", count);

  pthread_mutex_destroy(&mtx);
  return EXIT_SUCCESS;
}
