/*
EECE6540 - Heterogeneous Computing
Lab 2 - Kernel File
J.M. Gallagher
25-Jun-2021

Adapted from material made available by Dr. Luo at 
https://github.com/ACANETS/eece-6540-labs/tree/master/Labs/lab2/matrix_multi/device 
file matrix_multi.cl
*/

/* widthA=heightB for valid matrix multiplication */
__kernel void simpleMultiply(
    __global float *outputC,
    int widthA,
    int heightA,
    int widthB,
    int heightB,
    __global float *inputA,
    __global float *inputB,
    __global float *inputC)
{
    /* get global position in Y direction */
    int row = get_global_id (1);
    /* get global position in X direction */
    int col = get_global_id (0);

    float sum = 0.0f;

    /* calculate result of one element of Matrix D */
    for (int i = 0; i < widthA; i++) {
        sum += inputA[row*widthA + i] * inputB[i*widthB + col];
    }

    outputD[row*widthB + col] = sum + inputC[row*widthB + col];
}

