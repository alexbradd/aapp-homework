#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "alignment.h"

// Direction used for the traceback
enum BACK_DIR {LEFT, TOP, TOP_LEFT};

// Cell of the dynamic programming matrix
typedef struct {
  unsigned int distance;
  enum BACK_DIR backdir;
} cell_t;

// Dynamic programming matrix + relevant data used in the algorithm
typedef struct {
  cell_t **mat;
  const char *x, *y;
  unsigned int rows,
               cols,
               replace_p, gap_p;
} matrix_t;

// Allocate a new matrix_t
static inline matrix_t *matrix_alloc(const char *x, const char *y, int replace_p, int gap_p) {
  matrix_t *mat = calloc(1, sizeof(matrix_t));
  if (!mat) abort();
  mat->x = x;
  mat->y = y;
  mat->rows = strlen(y) + 1;
  mat->cols = strlen(x) + 1;
  mat->replace_p = replace_p;
  mat->gap_p = gap_p;

  mat->mat = calloc(mat->rows, sizeof(cell_t*));
  for (int i = 0; i < mat->rows; i++) {
    mat->mat[i] = calloc(mat->cols, sizeof(cell_t));
    if (!mat->mat[i]) abort();
  }
  return mat;
}

// Deallocate a matrix_t
static inline void matrix_free(matrix_t *m) {
  assert(m != NULL);
  for (int i = 0; i < m->rows; i++)
    free(m->mat[i]);
  free(m);
}

// see alignment.h
void alignment_free(alignment_t **a) {
  assert(a != NULL);
  assert((*a)->x != NULL);
  assert((*a)->y != NULL);

  free((*a)->x);
  free((*a)->y);
  free(*a);
  *a = NULL;
}

// Return the min between the three numbers
static inline enum BACK_DIR min(unsigned int left, unsigned int top, unsigned int topleft) {
  if (left < top) {
    if (left < topleft) // a < b | c
      return LEFT;
    else                // c < a < b
     return TOP_LEFT;
  } else {
    if (top < topleft)  // b < a | c
      return TOP;
    else
      return TOP_LEFT;  // c < a | b
  }
}

// Calculate and assign the cost of cell [i;j] of the matrix, i and j greater
// than 1
//
// Assumes that the first col and first row have been already takne care of.
static inline void calculate_cost(matrix_t *mat, int i, int j) {
  assert(mat != NULL);
  assert(i >= 1 && j >= 1);

  unsigned int left = mat->mat[i][j-1].distance + mat->gap_p,
               top  = mat->mat[i-1][j].distance + mat->gap_p,
               topleft = mat->mat[i-1][j-1].distance +
                 (mat->y[i - 1] == mat->x[j - 1] ? 0 : mat->replace_p);
  enum BACK_DIR wm = min(left, top, topleft);
  switch (wm) {
    case LEFT:
      mat->mat[i][j].distance = left;
      mat->mat[i][j].backdir = LEFT;
      break;
    case TOP:
      mat->mat[i][j].distance = top;
      mat->mat[i][j].backdir = TOP;
      break;
    case TOP_LEFT:
      mat->mat[i][j].distance = topleft;
      mat->mat[i][j].backdir = TOP_LEFT;
      break;
  }
}

// Prepare the first row and column of the matrix with increasing gap costs and
// null backpointers
static inline void prepare_blank_row_and_col(matrix_t *mat) {
  for (int i = 0; i < mat->cols; i++) {
    mat->mat[0][i].distance = i * mat->gap_p;
    mat->mat[0][i].backdir = LEFT;
  }
  for (int i = 0; i < mat->rows; i++) {
    mat->mat[i][0].distance = i * mat->gap_p;
    mat->mat[i][0].backdir = TOP;
  }
}

// Trace back on the matrix while writing the strings in the alignment_t strings
static inline void traceback(matrix_t *m, alignment_t *r) {
  int i = m->rows - 1;
  int j = m->cols - 1;
  int z = 0;
  r->cost = m->mat[i][j].distance;
  while (i > 0 || j > 0) {
    switch(m->mat[i][j].backdir) {
      case TOP_LEFT:
        if (m->x[j - 1] == m->y[i - 1]) {
          r->x[z] = m->x[j - 1];
          r->y[z] = m->y[i - 1];
        } else {
          r->x[z] = '*';
          r->y[z] = '*';
        }
        i--;
        j--;
        break;
      case TOP:
        r->x[z] = '_';
        r->y[z] = m->y[i - 1];
        i--;
        break;
      case LEFT:
        r->x[z] = m->x[j - 1];
        r->y[z] = '_';
        j--;
        break;
    }
    z++;
  }
  r->x[z] = '\0';
  r->y[z] = '\0';
}

// Reverse the given string
static inline void strrev(char *a) {
  char tmp;
  int len = strlen(a),
      halflen = len >> 1;
  for (int i = 0; i < halflen; i++) {
    tmp = a[i];
    a[i] = a[len - i - 1];
    a[len - i - 1] = tmp;
  }
}

// see alignment.h
alignment_t *align(const char *x,
                   const char *y,
                   unsigned int replace_p, unsigned int gap_p)
{
  assert(x != NULL && y != NULL);

  // Allocate structures
  matrix_t *mat = matrix_alloc(x, y, replace_p, gap_p);
  alignment_t *result = calloc(1, sizeof(alignment_t));
  // Maximum size alignment size is m + n
  // Proof in the colab
  result->x = calloc(1, sizeof(char[mat->rows + mat->cols + 1])); // +1 for \0
  result->y = calloc(1, sizeof(char[mat->rows + mat->cols + 1]));

  // Populate the matrix with distances and backpointers
  prepare_blank_row_and_col(mat);
  for (int i = 1; i < mat->rows; i++)
    for (int j = 1; j< mat->cols; j++)
      calculate_cost(mat, i, j);

  // Minimum cost is always the bottom-cell cell. Now we traceback following the
  // backpointers
  traceback(mat, result);

  // due to the traceback strings are in reverse.
  strrev(result->x);
  strrev(result->y);

  matrix_free(mat);
  return result;
}

#ifdef MAIN
#define REPLACE_PENALTY 10
#define GAP_PENALTY     2

void test(const char *x, const char *y) {
  alignment_t *a;

  printf(">>> Aligning '%s' and '%s'\n", x, y);
  a = align(x, y, REPLACE_PENALTY, GAP_PENALTY);
  printf("x: %s; y: %s; cost = %d\n", a->x, a->y, a->cost);
  alignment_free(&a);
}

int main(void) {
  printf("Alignment test\n");

  // empty string
  test("", "");

  // classroom examples
  test("CG", "CA");
  test("AGGGCT", "AGGCA");

  // substring
  test("AGGGCT", "GCT");
  test("GCT", "ACTGGCT");

  // equality
  test("ACGT", "ACGT");

  // Test to show that the length of the aligned string is at worst m+n
  test("GGGGG", "CCCCC");
}
#endif
