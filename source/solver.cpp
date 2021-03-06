#include <iostream>
#include <cassert>
#include <omp.h>
#include <iostream>
#include <time.h> 
#include <cublas_v2.h>

#include "../include/solver.h"
#include "../include/rhs.cuh"
#include "../include/matrix_op.h"
#include "../include/velocity.h"
#include "../include/coefficient.cuh"
#include "../include/operation.h"
#include "../include/transform_multi_gpu.h"
#include "../include/cuRPCF.h"

using namespace std;
//// compute multiply of matrix and vector
//void multiplyMatrix(cuRPCF::complex* mul, cuRPCF::complex* v, const int n);
//
//// compute coeffecient matrix of v
//void getCoefV(cuRPCF::complex * coefv, int n, REAL kmn, REAL alpha,
//	matrix2d<REAL>& T0, matrix2d<REAL>& T2, matrix2d<REAL>& T4,
//	REAL* U0, REAL* dU0, REAL* ddU0, const REAL dt, const REAL Re);
//
//// compute coeffecient matrix of omega
//void getCoefOmega(cuRPCF::complex * coefOmega, int n, REAL kmn, REAL alpha,
//	matrix2d<REAL>& T0, matrix2d<REAL>& T2, matrix2d<REAL>& T4,
//	REAL* U0, REAL* dU0, REAL* ddU0, const REAL dt, const REAL Re);



cublasHandle_t __cublas_handle;

int nextStep(problem& pb) {

#ifdef CURPCF_CUDA_PROFILING
	clock_t start_time, end_time;
	double cost_eq = 0.0;
	double cost_rhs_omega = 0.0;
#endif

	get_rhs_v(pb);
	//cout << "solve eq v" << endl;
	//solve equation of v from (0,0) to (nx,ny)

#ifdef CURPCF_CUDA_PROFILING
	start_time = clock();
#endif

	solveEq(pb.matrix_coeff_v, pb.rhs_v,
		pb.nz, pb.tPitch, pb.mx, pb.my); 
	//solveEqGPU(pb.matrix_coeff_v, pb.rhs_v,
	//	pb.nz, pb.tPitch, pb.mx, pb.my, 0); 

#ifdef CURPCF_CUDA_PROFILING
	end_time = clock();
	cost_eq += (double)(end_time - start_time) / CLOCKS_PER_SEC;

	start_time = clock(); 
#endif	

	//cout << "get rhs omega" << endl;
	get_rhs_omega(pb);

#ifdef CURPCF_CUDA_PROFILING
	end_time = clock();
	cost_rhs_omega += (double)(end_time - start_time) / CLOCKS_PER_SEC;
	std::cout << "rhs omega time = " << cost_rhs_omega << std::endl;
	start_time = clock();
#endif

	//cout << "solve eq omega" << endl;
	//solve equation of omega from (0,0) to (nx,ny)
	solveEq(pb.matrix_coeff_omega, pb.rhs_omega_y,
		pb.nz, pb.tPitch, pb.mx, pb.my);
	//solveEqGPU(pb.matrix_coeff_omega, pb.rhs_omega_y, 
	//	pb.nz, pb.tPitch, pb.mx, pb.my, 1);
	
#ifdef CURPCF_CUDA_PROFILING
	end_time = clock();
	cost_eq += (double)(end_time - start_time) / CLOCKS_PER_SEC;
	std::cout << "solve equation time = " << cost_eq << std::endl;
#endif

	//cout << "save 0 v,oy" << endl;

	// save results of u0 and w0 results.
	save_0_v_omega_y(pb);
	
	//cout << "get velocity" << endl;

	synchronizeGPUsolver();

	cuCheck(cudaMemcpy(pb.dptr_tw.ptr, pb.rhs_v, pb.tSize, cudaMemcpyHostToDevice),"cpy");
	cuCheck(cudaMemcpy(pb.dptr_tomega_z.ptr, pb.rhs_omega_y, pb.tSize, cudaMemcpyHostToDevice), "cpy");

	getUVW(pb);
	//pb.currenStep++;
	return 0;
}

int startLoop(problem& pb) {
	return 0;
}

int solveEq(cuRPCF::complex* inv_coef, cuRPCF::complex* rhs, int N,
			size_t pitch, int mx, int my) {
	int nx = mx / 3 * 2;
	int ny = my / 3 * 2;
	#pragma omp parallel for firstprivate(nx,ny)
	for (int i = 0; i < nx/2+1; i++) {
		//cout << "solver omp id:" << omp_get_thread_num() << " i=" << i<<endl;		
		for (int j = 0; j < ny; j++) {			
			size_t inc_m = N*N*((nx/2+1)*j + i);
			size_t inc_rhs = pitch / sizeof(cuRPCF::complex) * ((nx/2+1)*j + i);
			cuRPCF::complex* _inv_coef_ = inv_coef + inc_m;
			cuRPCF::complex* _rhs_ = rhs + inc_rhs;
			m_multi_v(_inv_coef_, _rhs_ , N);
		}
	}
	return 0;
}


//void multiplyMatrix(cuRPCF::complex * mul, cuRPCF::complex * v, const int n)
//{
//	cuRPCF::complex* temp = (cuRPCF::complex*)malloc(n * sizeof(cuRPCF::complex));
//	for (int i = 0; i < n; i++) {
//		auto* ai = mul + n*i;
//		temp[i].re = 0.0;
//		temp[i].im = 0.0;
//		for (int j = 0; j < n; j++) {
//			temp[i].re = temp[i].re + ai[j].re * v[j].re - ai[j].im * v[j].im;
//			temp[i].im = temp[i].im + ai[j].im * v[j].re + ai[j].re * v[j].im;
//		}
//	}
//	for (int i = 0; i < n; i++) {
//		v[i] = temp[i];
//	}
//	free(temp);
//}
//
//void getCoefV(cuRPCF::complex * coefv, int n, REAL kmn, REAL alpha, 
//	matrix2d<REAL>& T0, matrix2d<REAL>& T2, matrix2d<REAL>& T4, 
//	REAL* U0, REAL* dU0, REAL* ddU0 ,const REAL dt,const REAL Re)
//{
//	for (int i = 4; i < n; i++) {
//		for (int j = 0; j < n; j++) {
//			cuRPCF::complex* coefIJ = coefv + (i*n + j);
//			coefIJ->re = -T4(i - 2, j)*dt*0.5 / Re 
//				+ (1 + kmn*dt / Re)*T2(i - 2, j)
//				- kmn*(1 + kmn*dt*0.5 / Re)*T0(i - 2, j);
//			coefIJ->im = alpha*dt*U0[i - 2] * 0.5 / Re*T2(i - 2, j)
//				- (kmn*alpha*dt*0.5*U0[i - 2]
//					- alpha*dt*0.5*ddU0[i - 2])*T0(i - 2, j);
//		}
//	}
//
//	
//	for (int j = 0; j < n; j++) {
//		cuRPCF::complex* a0j = coefv + j;
//		cuRPCF::complex* a1j = coefv + n + j;
//		cuRPCF::complex* a2j = coefv + 2 * n + j;
//		cuRPCF::complex* a3j = coefv + 3 * n + j;
//		*a0j = cuRPCF::complex(0, 0);
//		*a1j = cuRPCF::complex((j % 2 == 0) ? 1 : -1, 0);
//		*a2j = cuRPCF::complex(j*j, 0);
//		*a3j = cuRPCF::complex(((j % 2 == 0) ? -1 : 1)*j*j, 0);
//	}
//}
//
//void getCoefOmega(cuRPCF::complex * coefOmega, int n, REAL kmn, REAL alpha,
//	matrix2d<REAL>& T0, matrix2d<REAL>& T2, matrix2d<REAL>& T4,
//	REAL* U0, REAL* dU0, REAL* ddU0, const REAL dt, const REAL Re)
//{
//	for (int i = 2; i < n; i++) {
//		for (int j = 0; j < n; j++) {
//			cuRPCF::complex* coefIJ = coefOmega + (i*n + j);
//			coefIJ->re = -T2(i - 1, j)*dt*0.5 / Re
//				+ (1 + kmn*dt*0.5 / Re)*T0(i - 1, j);
//			coefIJ->im = alpha*dt*0.5*U0[i - 1] * T0(i - 1, j);
//		}
//	}
//	
//	for (int j = 0; j < n; j++) {
//		cuRPCF::complex* a0j = coefOmega + j;
//		cuRPCF::complex* a1j = coefOmega + n + j;
//		*a0j = cuRPCF::complex(1, 0);
//		*a1j = cuRPCF::complex((j % 2 == 0) ? 1 : -1, 0);
//	}
//}

cuRPCF::complex* __dev_coef_v = NULL;
cuRPCF::complex* __dev_coef_omega = NULL;
cuRPCF::complex* __dev_rhs = NULL;
int initGPUSolver(problem& pb) {
	cublasStatus_t stat;
	stat = cublasCreate(&__cublas_handle);
	if (stat != CUBLAS_STATUS_SUCCESS) {
		printf("CUBLAS initialization failed\n");
		return EXIT_FAILURE;
	}
	size_t mSize = pb.nz * pb.nz * (pb.nx / 2 + 1)*pb.ny * sizeof(cuRPCF::complex);
	size_t tSize = pb.tSize;
	cuCheck(cudaSetDevice(dev_id[1]));
	cuCheck(cudaMalloc(&__dev_coef_v, mSize*2 + tSize),"alloc solver data");
	__dev_coef_omega = __dev_coef_v + mSize / sizeof(cuRPCF::complex);
	__dev_rhs = __dev_coef_omega + mSize / sizeof(cuRPCF::complex);
	cuCheck(cudaMemcpy(__dev_coef_v, pb.matrix_coeff_v, mSize, cudaMemcpyHostToDevice),"memcpy");
	cuCheck(cudaMemcpy(__dev_coef_omega, pb.matrix_coeff_omega, mSize, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaSetDevice(dev_id[0]));
	return 0;
}

int allocHostMemory(problem& pb)
{
	//omp_set_num_threads(12);
	size_t& tSize = pb.tSize;
	size_t mSize = pb.nz * pb.nz * (pb.nx / 2 + 1)*pb.ny * sizeof(cuRPCF::complex);

	cout << "malloc matrix memory" << endl;
	pb.matrix_coeff_v = (cuRPCF::complex*)malloc(mSize);
	pb.matrix_coeff_omega = (cuRPCF::complex*)malloc(mSize);

	cout << "malloc nonlinear memory" << endl;
	cudaMallocHost(&pb.nonlinear_v, tSize);
	cudaMallocHost(&pb.nonlinear_v_p, tSize);
	cudaMallocHost(&pb.nonlinear_omega_y, tSize);
	cudaMallocHost(&pb.nonlinear_omega_y_p, tSize);

	cout << "malloc rhs memory" << endl;
	cudaMallocHost(&pb.rhs_v, tSize); 
	cudaMallocHost(&pb.rhs_v_p, tSize);
	cudaMallocHost(&pb.rhs_omega_y, tSize);

	cout << "malloc zerowave number memory" << endl;
	cudaMallocHost(&pb.lambx0, sizeof(cuRPCF::complex)*pb.nz);
	cudaMallocHost(&pb.lambx0_p, sizeof(cuRPCF::complex)*pb.nz);
	cudaMallocHost(&pb.lambz0, sizeof(cuRPCF::complex)*pb.nz);
	cudaMallocHost(&pb.lambz0_p, sizeof(cuRPCF::complex)*pb.nz);

	pb.tv0 = (cuRPCF::complex*)malloc(sizeof(cuRPCF::complex)*pb.nz);
	pb.tomega_y_0 = (cuRPCF::complex*)malloc(sizeof(cuRPCF::complex)*pb.nz);

	cout << "malloc U memory" << endl;
	pb._U0 = (REAL*)malloc(sizeof(REAL)*pb.nz);
	pb._dU0 = (REAL*)malloc(sizeof(REAL)*pb.nz);
	pb._ddU0 = (REAL*)malloc(sizeof(REAL)*pb.nz);

	cout << "malloc T memory" << endl;
	pb.T0 = (REAL*)malloc(sizeof(REAL)*pb.nz*pb.nz);
	pb.T2 = (REAL*)malloc(sizeof(REAL)*pb.nz*pb.nz);
	pb.T4 = (REAL*)malloc(sizeof(REAL)*pb.nz*pb.nz);

	return 0;
}

// initialize T matrixes, U vector and Aij^-1, Bij^-1
int initAuxMatrix(problem& pb, bool inversed) {
	const int nz = pb.nz;
	const int nx = pb.nx;
	const int ny = pb.ny;

	//init T matrix
	get_T_matrix(pb.nz - 1, pb.T0, pb.T2, pb.T4);

	//init mean velocity vector
	get_U(pb.nz, pb._U0, pb._dU0, pb._ddU0);

	//init coef matrix
	const int hnx = pb.nx / 2 + 1;
	int ky = 0;

	#pragma omp parallel for private(ky)
	for (int kx = 0; kx < hnx; kx++) {
		//cout << "omp id:" << omp_get_thread_num() << endl;
		for (ky = 0; ky < ny; ky++) {
			if (kx == 0 && ky == 0) {
				_get_coef_u0(pb.matrix_coeff_v, pb.nz - 1, pb.T0, pb.T2, pb.Re, pb.dt);
				_get_coef_w0(pb.matrix_coeff_omega, pb.nz - 1, pb.T0, pb.T2, pb.Re, pb.dt);
				if (inversed) {
					int ret;
					ret = inverse(pb.matrix_coeff_omega, pb.nz);
					ASSERT(ret == 0);
					ret = inverse(pb.matrix_coeff_v, pb.nz);
					ASSERT(ret == 0);
				}
				continue;
			}

			REAL ialpha, ibeta;
			get_ialpha_ibeta(kx, ky, ny, pb.aphi, pb.beta, ialpha, ibeta);

			REAL kmn = ialpha * ialpha + ibeta * ibeta;

			size_t inc = pb.nz * pb.nz * (hnx * ky + kx);
			cuRPCF::complex* coe_v = pb.matrix_coeff_v + inc;
			cuRPCF::complex* coe_o = pb.matrix_coeff_omega + inc;

			_get_coefficient_v(coe_v, pb.nz - 1, pb._U0, pb._ddU0,
				pb.T0, pb.T2, pb.T4, pb.Re, pb.dt, kmn, ialpha);
			_get_coefficient_omega(coe_o, pb.nz - 1, pb._U0,
				pb.T0, pb.T2, pb.T4, pb.Re, pb.dt, kmn, ialpha);
			if (inversed) {
				int ret;
				ret = inverse(coe_v, pb.nz);
				ASSERT(ret == 0);
				ret = inverse(coe_o, pb.nz);
				ASSERT(ret == 0);
			}
		}
	}

	pb.currenStep = pb.para.stepPara.start_step;

	//int ret = initGPUSolver(pb);
	//assert(ret == 0);
	return 0;
}

int destroySolver(problem& pb) {
	free(pb.matrix_coeff_omega);
	free(pb.matrix_coeff_v);
	cudaFreeHost(pb.nonlinear_v);
	cudaFreeHost(pb.nonlinear_omega_y);
	cudaFreeHost(pb.nonlinear_v_p);
	cudaFreeHost(pb.nonlinear_omega_y_p);
	return 0;
}

void save_0_v_omega_y(problem& pb) {
	const int nz = pb.nz;
	for (int i = 0; i < pb.nz; i++) {
		pb.tv0[i] = pb.rhs_v[i];
		pb.tomega_y_0[i] = pb.rhs_omega_y[i];
	}
}


//extern size_t __myMaxMemorySize[NUM_GPU];
#ifdef REAL_DOUBLE
#define CUBLAS_CGEMV cublasZgemv
#define CUBLAS_COMPLEX cuDoubleComplex
#elif
#define CUBLAS_CGEMV cublasCgemv
#define CUBLAS_COMPLEX cuComplex
#endif

extern size_t __myMaxMemorySize[NUM_GPU];

int solveEqGPU(cuRPCF::complex* inv_coef, cuRPCF::complex* rhs, int N,
	size_t pitch, int mx, int my, int num_equation) {
	int nx = mx / 3 * 2;
	int ny = my / 3 * 2;

	//cuRPCF::complex* dev_ptr = (cuRPCF::complex*)get_fft_buffer_ptr();
	
	size_t free_memory = __myMaxMemorySize[0];

	size_t total_matrix_and_data_size = ((N)*(N) * sizeof(cuRPCF::complex) + pitch)*(nx / 2 + 1)*ny;
	size_t n_parts = 1;
	//size_t n_parts = total_matrix_and_data_size / free_memory + 1;
	size_t index_per_part = (nx / 2 + 1)*ny / n_parts;
	assert((nx / 2 + 1)*ny % n_parts == 0);
	size_t size_matrix_per_part = (N)*(N)*index_per_part * sizeof(cuRPCF::complex);
	size_t size_rhs_per_part = pitch*index_per_part;

	for (int iPart = 0; iPart < n_parts; iPart++) {
		size_t start_index = index_per_part * iPart;
		size_t inc_m = (N)*(N)*start_index;
		size_t inc_rhs = pitch / sizeof(cuRPCF::complex) * start_index;
		cuRPCF::complex* pMatrix = inv_coef + inc_m;
		cuRPCF::complex* pRHS = rhs + inc_rhs;
		//cuRPCF::complex* dev_matrix = dev_ptr;
		cuRPCF::complex* dev_matrix = (num_equation==0 ? __dev_coef_v : __dev_coef_omega);
		//cuRPCF::complex* dev_rhs = dev_ptr + size_matrix_per_part / sizeof(cuRPCF::complex);
		cuRPCF::complex* dev_rhs = __dev_rhs;

		cudaError_t err;
		//assert(pMatrix + size_matrix_per_part/sizeof(cuRPCF::complex) <= inv_coef + (N)*(N)*(nx/2+1)*ny);
		//err = cudaMemcpy(dev_matrix, pMatrix, size_matrix_per_part, cudaMemcpyHostToDevice);
		//assert(err == cudaSuccess);
		//assert(pRHS + size_rhs_per_part/sizeof(cuRPCF::complex) <= rhs + pitch/sizeof(cuRPCF::complex)*(nx/2+1)*ny);
		err = cudaMemcpy(dev_rhs, pRHS, size_rhs_per_part, cudaMemcpyHostToDevice);
		assert(err == cudaSuccess);

		CUBLAS_COMPLEX _cublas_alpha, _cublas_beta;
		cublasStatus_t cuStat;
		_cublas_alpha.x = 1.0;
		_cublas_alpha.y = 0.0;
		_cublas_beta.x = 0.0;
		_cublas_beta.y = 0.0;
		//for (int i = 0; i < index_per_part; i++) {
		for (int i = 0; i < 1; i++) {
			CUBLAS_COMPLEX* p_dev_matrix = (CUBLAS_COMPLEX*)(dev_matrix + (N)*(N)*i);
			CUBLAS_COMPLEX* p_dev_rhs = (CUBLAS_COMPLEX*)(dev_rhs + pitch / sizeof(cuRPCF::complex)*i);
			//cuStat = CUBLAS_CGEMV(__cublas_handle, CUBLAS_OP_T, N, N, &_cublas_alpha, (CUBLAS_COMPLEX*)dev_matrix,
			//	N, (CUBLAS_COMPLEX*)dev_rhs, 1, &_cublas_beta, (CUBLAS_COMPLEX*)dev_rhs, 1);
			//assert(cuStat == CUBLAS_STATUS_SUCCESS);
			err = m_multi_v_gpu((cuRPCF::complex*)p_dev_matrix, (cuRPCF::complex*)p_dev_rhs, N, pitch, index_per_part);
			assert(err == cudaSuccess);
		}

		cudaMemcpy(pRHS, dev_rhs, size_rhs_per_part, cudaMemcpyDeviceToHost);
	}
	return 0;
}