#ifndef __ALIGNMENT_H__
#define __ALIGNMENT_H__

// Struct used as result to align(). It contains the two aligned strings and the
// cost of the alignment
typedef struct {
  char *x, *y;
  unsigned int cost;
} alignment_t;

// Given two strings x and y and the penalties, output the minimum cost
// alignment
//
// The alignment is heap allocated, so it needs to be freed using
// alignment_free()
extern alignment_t *align(const char *x,
                          const char *y,
                          unsigned int replace_p, unsigned int gap_p);
// Destroy the `alignment_t` returned by align()
extern void alignment_free(alignment_t **a);
#endif
