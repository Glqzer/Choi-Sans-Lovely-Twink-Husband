#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Merge the elements in the sorted ranges [begin, mid) and [mid, end),
// copying the result into temparr.
void merge(int64_t *arr, size_t begin, size_t mid, size_t end, int64_t *temparr) {
  int64_t *endl = arr + mid;
  int64_t *endr = arr + end;
  int64_t *left = arr + begin, *right = arr + mid, *dst = temparr;

  for (;;) {
    int at_end_l = left >= endl;
    int at_end_r = right >= endr;

    if (at_end_l && at_end_r) break;

    if (at_end_l)
      *dst++ = *right++;
    else if (at_end_r)
      *dst++ = *left++;
    else {
      int cmp = compare_i64(left, right);
      if (cmp <= 0)
        *dst++ = *left++;
      else
        *dst++ = *right++;
    }
  }
}

// Compare two values, returns negative value if lhs < rhs, 0 if equal, and positive value
// if lhs> rhs.
int compare_i64(const void *p1, const void *p2) {
  int64_t left = *(const int64_t *) p1;
  int64_t right = *(const int64_t *) p2;

  if (left < right) {
    return -1;
  }
  if (left > right) {
    return 1;
  }
  return 0;
}

// Recursively merge sorts half of given sequence. Function should be called when in child process.
int do_child_work(int64_t* arr, size_t begin, size_t end, size_t threshold) {
  int check = merge_sort(arr, begin, end, threshold);
  return check;
}

// Sorts elements in given range either sequentially or in parallel depending on threshold parameter.
// Returns 0 if no errors.
int merge_sort(int64_t *arr, size_t begin, size_t end, size_t threshold) {
  // If (number of elements <= threshold), sort elements sequentially
  size_t length = end - begin;
  size_t mid = begin + (length) / 2;
  if (length <= threshold) {
    // Call qsort() with newly defined comparison function compare_i64()
    qsort(arr + begin, length, sizeof(int64_t), compare_i64);
  }
  else {
    if (begin >= end) {
      return 1;
    } 
    // Recursively sort the left half of the sequence
    //merge_sort(arr, begin, mid, threshold);
    // Recursively sort the right half of the sequence
    //merge_sort(arr, mid, end, threshold); 
    //-->commented this block out after checking that sequential works!

    int wstatus1;
    int wstatus2;

    pid_t pid1 = fork(); // Creates first child process
    
    if (pid1 == -1) {
      // Fork failed to start a new process
      // Handle error and exit
      fprintf(stderr, "%s", "Error: fork() failed to start a new process.\n");
      return -1;
    } else if (pid1 == 0) {
      // This is now in the child ONE process
      // Sort left half of sequence
      int retcode = do_child_work(arr, begin, mid, threshold);
      exit(retcode);
      // Everything past here is now unreachable in the child  
    }
    pid_t pid2 = fork(); // Creates second child process
    if (pid2 == -1) {
      // Fork failed to start a new process
      // Handle the error and exit
      fprintf(stderr, "%s", "Error: fork() failed to start a new process.\n");
      return -1;
    } else if (pid2 == 0) {
      // This is now in the child TWO process
      // Sort right half of sequence
      int retcode = do_child_work(arr, mid, end, threshold);
      exit(retcode);
      // Everything past here is now unreachable in the child  
    }
    
    // If pids are not 0, we are in the parent process
    pid_t actual_pid1 = waitpid(pid1, &wstatus1, 0);
    pid_t actual_pid2 = waitpid(pid2, &wstatus2, 0);
    if (actual_pid1 == -1) {
      // Handle waitpid1 failure
      fprintf(stderr, "%s", "Error: Waitpid failure for child 1 process.\n");
      return 1;
    }
    if (actual_pid2 == -1) {
      // Handle waitpid2 failure
      fprintf(stderr, "%s", "Error: Waitpid failure for child 2 process.\n");
      return 1;
    }
    // WARNING!!!!!! if the child process path can get here, things will quickly break very badly
    if (!WIFEXITED(wstatus1) || !WIFEXITED(wstatus2)) {
    // Subprocess crashed, was interrupted, or did not exit normally
    // Handle as error
      fprintf(stderr, "%s", "Error: Subprocess crashed, was interrupted, or didn't exit normally.\n");
      return 1;
    }
    if ((WEXITSTATUS(wstatus1) != 0) || WEXITSTATUS(wstatus2) != 0) {
      // Subprocess returned a non-zero exit code
      // If following standard UNIX conventions, this is also an error
      fprintf(stderr, "%s", "Error: Subprocess returned a non-zero exit code.\n");
      return 1;
    }
  
  }

  // Merge the sorted sequences into a temp array (dynamic allocate on heap, use malloc and free)
  int64_t* temparr = (int64_t*) malloc(length * sizeof(int64_t));
  merge(arr, begin, mid, end, temparr); //add temparray

  // Copy the contents of the temp array back to the original array
  // Loop through temp array to copy, pay attention to indexing!
  for (int i = 0; i < length; i++) {
    arr[i + begin] = temparr[i];
  }
  // Then free temp array
  free(temparr);
  return 0;
}

int main(int argc, char **argv) {
  // Check for correct number of command line arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <filename> <sequential threshold>\n", argv[0]);
    return 1;
  }

  // Process command line arguments
  const char *filename = argv[1];
  char *end;
  size_t threshold = (size_t) strtoul(argv[2], &end, 10);
  if (end != argv[2] + strlen(argv[2])) {
    fprintf(stderr, "%s", "Error: Threshold value is invalid.\n");
    return -1;
  }

  // Open the file
  int fd = open(filename, O_RDWR);
  if (fd < 0) {
    // File couldn't be opened: handle error and exit
    fprintf(stderr, "%s", "Error: File couldn't be opened.\n");
    return -1;
  }

  // Use fstat to determine the size of the file
  struct stat statbuf;
  int rc = fstat(fd, &statbuf);
  if (rc != 0) {
    // Handle fstat error and exit
    fprintf(stderr, "%s", "Error: Call to fstat() returns a non-zero exit code.\n");
    return -1;
  }
  size_t file_size_in_bytes = statbuf.st_size;

  // Map the file into memory using mmap
  int64_t *data = mmap(NULL, file_size_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  // Immediately close file descriptor here since mmap maintains a separate
  // reference to the file and all open fds will gets duplicated to the children, which will
  // cause fd in-use-at-exit leaks.

  close(fd); // Closes file descriptor

  if (data == MAP_FAILED) {
    // Handle mmap error and exit
    fprintf(stderr, "%s", "Error: Failure to mmap file.\n");
    return -1;
  }

  // DON'T FORGET to call munmap and close BEFORE returning from the topmost process 
  // in your program to prevent leaking resources!!
  
  // Sort the data!
  int check = merge_sort(data, 0, (file_size_in_bytes / 8), threshold);

  // Unmap and close the file
  munmap(data, file_size_in_bytes);
  close(fd);

  // Exit with a 0 exit code if sort was successful
  if (check == 0) {
    return 0;
  }
  return -1;
}
