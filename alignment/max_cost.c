#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alignment.h"

// A pair of strings
typedef struct {
  char *x, *y;
} string_pair_t;

// Allocate a new string pair
static inline string_pair_t *string_pair_alloc(unsigned int m, unsigned int n) {
  string_pair_t *result = calloc(1, sizeof(string_pair_t));
  result->x = calloc(1, sizeof(char[m + 1]));
  result->y = calloc(1, sizeof(char[n + 1]));
  result->x[m] = '\0';
  result->y[n] = '\0';
  return result;
}

// Free a string pair
static inline void string_pair_free(string_pair_t **s) {
  assert(s != NULL);
  assert(*s != NULL);
  free((*s)->x);
  free((*s)->y);
  free(*s);
  *s = NULL;
}

// Copy pattern into x of size m, and, if necessary:
//
// - truncate patterns, or
// - fill with the last character in pattern
void insert_pattern(char *x, size_t m, const char *pattern) {
  if (m < 4) {
    strncpy(x, pattern, m);
  } else {
    strncpy(x, pattern, 4);
    for (int i = 4; i < m; i++)
      x[i] = pattern[3];
  }
}

// Generating a new pair of strings using the "ATCG+" and "GCTA+" patterns
//
// The pair and the strings inside it are all heap allocated and need to be freed
// using string_pair_free()
string_pair_t *max_cost_strings(unsigned int m, unsigned int n)
{
  static const char *PATTERN_1 = "ACGT",
                    *PATTERN_2 = "TGCA";
  string_pair_t *s = string_pair_alloc(m, n);

  insert_pattern(s->x, m, PATTERN_1);
  insert_pattern(s->y, n, PATTERN_2);

  return s;
}

// Align the string pair with the given penalties
void try_alignment(string_pair_t *s, unsigned int gap_p, unsigned int replace_p) {
  printf("Aligning %s and %s\n", s->x, s->y);
  alignment_t *a = align(s->x, s->y, replace_p, gap_p);
  printf("x: %s\ny: %s\ncost = %d\n", a->x, a->y, a->cost);
  alignment_free(&a);
}

int main(int argc, char **argv) {
  unsigned int m, n, g, r;

  if (argc < 5) {
    srand((unsigned int) time(NULL));
    m = rand() % 128;
    n = rand() % 128;
    g = rand() % 10;
    r = rand() % 10;
    printf(">> Not enough parameters, generating with:\n");
  } else {
    m = abs(atoi(argv[1]));
    n = abs(atoi(argv[2]));
    g = abs(atoi(argv[3]));
    r = abs(atoi(argv[4]));
    printf(">> Received parameters, generating with:\n");
  }
  printf(">> \tString 1 length: %d\n", m);
  printf(">> \tString 2 length: %d\n", n);
  printf(">> \tGap penalty: %d\n", g);
  printf(">> \tReplacement penalty: %d\n", r);

  string_pair_t *s = max_cost_strings(m, n);
  try_alignment(s, g, r);
  string_pair_free(&s);
  putchar('\n');
}
