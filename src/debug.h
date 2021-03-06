/* Student Name: Yiğit Özkavcı
 * Student Number: 2013400111
 * Compile Status: Compiling
 * Program Status: Working
 *
 * Notes:
 * You can compile the code on any unix/linux platform with typing:
 * $ make
 *
 * Then run with:
 * $ mpiexec -n <n_of_processors> <program> <in_file> <out_file> <threshold>
 */

#ifndef DEBUG_H_
#define DEBUG_H_

/**********************************************************************
 * Debug without any parameter.
 **********************************************************************/
void debug_1(char *message, int *rank);

/**********************************************************************
 * Debug with 1 integer parameter. Mesage should contain one "%d"
 * inside, just like printf.
 **********************************************************************/
void debug_2(char *message, int *arg1, int *rank);

/**********************************************************************
 * Debug with 2 integer parameters.
 **********************************************************************/
void debug_3(char *message, int *arg1, int *arg2, int *rank);

/**********************************************************************
 * Debug with 3 integer parameters.
 **********************************************************************/
void debug_4(char *message, int *arg1, int *arg2, int *arg3, int *rank);

#endif // DEBUG_H_
