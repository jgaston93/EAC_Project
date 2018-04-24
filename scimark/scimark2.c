#include "scimark2.h"
#include "Random.h"
#include "kernel.h"
#include "constants.h"

/* Benchmark is executing */




void scimark2(unsigned int benchmark_num)
{
	
	double min_time = RESOLUTION_DEFAULT;

	int FFT_size = FFT_SIZE;
	int SOR_size =  SOR_SIZE;
	int Sparse_size_M = SPARSE_SIZE_M;
	int Sparse_size_nz = SPARSE_SIZE_nz;
	int LU_size = LU_SIZE;

	/* run the benchmark */
	Random R = new_Random_seed(RANDOM_SEED);

	switch(benchmark_num) {
		case 0:
			kernel_measureFFT( FFT_size, min_time, R);
			break;
		case 1:
			kernel_measureSOR( SOR_size, min_time, R); 
			break;
		case 2:  
			kernel_measureMonteCarlo(min_time, R); 
			break;
		case 3:
			kernel_measureSparseMatMult( Sparse_size_M,
				Sparse_size_nz, min_time, R); 
			break;
		case 4:    
			kernel_measureLU( LU_size, min_time, R);  
			break;
	}

	Random_delete(R);
}
