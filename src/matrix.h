#ifndef MATRIX_H_
#define MATRIX_H_

/**
 * Prints the given integer matrix.
 */
void print_matrix_i(int**, int, char*); 

/**
 * Prints the given double matrix.
 */
void print_matrix_d(double**, int); 

/**
 * Given a 2D array, frees it from the memory.
 */
void free_matrix_d(double**, int);

/**
 * Given a 2D array, frees it from the memory.
 */
void free_matrix_i(int**, int);

#endif // MATRIX_H_
