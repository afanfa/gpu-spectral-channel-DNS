#include "../include/operation.h"
#include "../include/data.h"
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include "../include/parameters.h"
#include "../include/transform_multi_gpu.h"

using namespace std;

//int RPCF::read_parameter(Flow& fl,std::string& s) {
//	parameter& para = fl.para;
//	para.dt = 0.005;
//	para.Re = 1300;
//	para.Ro = 0.01;
//	fl.nnx = 256;
//	fl.nny = 256;
//	fl.nnz = 70;
//	return 0;
//}
//
//Flow::Flow(int i, int j, int k) :
//	u(i,j,k),v(i,j,k),w(i,j,k),
//	omega_x(i,j,k),omega_y(i,j,k),omega_z(i,j,k),
//	u_k(i,j,k),v_k(i,j,k),w_k(i,j,k),
//	omega_x_k(i,j,k),omega_y_k(i, j, k), omega_z_k(i, j, k)
//{
//};
//
//int Flow::initialize() {
//	return 0;
//}

//void problem::memcpy_device_to_host() {
//	size_t isize = dptr_u.pitch*ny*nz;
//	cudaError_t cuerr;
//	//isize = 1 * sizeof(REAL);
//	if (hptr_omega_z==nullptr) {		
//		std::cout << "allocated " << endl;
//		hptr_u = (REAL*)malloc(isize);
//		hptr_v = (REAL*)malloc(isize);
//		hptr_w = (REAL*)malloc(isize);
//		hptr_omega_x = (REAL*)malloc(isize);
//		hptr_omega_y = (REAL*)malloc(isize);
//		hptr_omega_z = (REAL*)malloc(isize);
//	}
//	cuerr = cudaSuccess;
//	cuerr = cudaMemcpy(hptr_u, dptr_u.ptr, isize, cudaMemcpyDeviceToHost);
//	cuerr = cudaMemcpy(hptr_v, dptr_v.ptr, isize, cudaMemcpyDeviceToHost);
//	cuerr = cudaMemcpy(hptr_w, dptr_w.ptr, isize, cudaMemcpyDeviceToHost);
//
//	cuerr = cudaMemcpy(hptr_omega_x, dptr_omega_x.ptr, isize, cudaMemcpyDeviceToHost);
//	cuerr = cudaMemcpy(hptr_omega_y, dptr_omega_y.ptr, isize, cudaMemcpyDeviceToHost);
//	//cuerr = cudaMemcpy(hptr_omega_z, dptr_omega_z.ptr, isize, cudaMemcpyDeviceToHost);
//
//	if (cuerr != cudaSuccess) {
//		cout << cuerr << endl;
//	}
//	cudaDeviceSynchronize();
//}


int RPCF::write_3d_to_file(char* filename,REAL* pu, int pitch, int nx, int ny, int nz) {
	ofstream outfile(filename,fstream::out);
	// skip this part
	//return 0;
	ASSERT(outfile.is_open());
	for (int k = 0; k < nz; k++) {
		size_t slice = pitch*ny*k;
		for (int j = 0; j < ny; j++) {
			REAL* row = (REAL*)((char*)pu + slice + j*pitch);
			for (int i = 0; i < nx; i++) {
				outfile << row[i] << "\t";
			}
			outfile << endl;
		}
		outfile << endl;
	}
	outfile.close();
	return 0;
}


void cuCheck(cudaError_t ret, string s) {
	if (ret == cudaSuccess) {
		return;
	}
	else {
		printf("cudaError at %s\n", s.c_str());
		system("pause");
		assert(false);
		exit(ret);
	}
}

bool isEqual(REAL a, REAL b, REAL precision ){
	if (abs(a - b) <= precision) {
		return true;
	}
	else
	{
		if (abs(a/b-1.0)<1e-4) {
			return true;
		}
		else {
			return false;
		}
	}
}



void RPCF_Paras::read_para(std::string filename) {
	ifstream infile;
	infile.open(filename.c_str(), ios::in);
	if (!infile.is_open()) {
		cerr << "Error in opening file: " << filename << endl;
		//exit(-1);
		return;
	}
	RPCF_Numerical_Para& np = this->numPara;
	
	infile >> np.mx >> np.my >> np.mz;
	infile >> np.n_pi_x >> np.n_pi_y;
	infile >> np.Re >> np.Ro >> np.dt;

	RPCF_Step_Para& sp = this->stepPara;
	infile >> sp.start_type >> sp.start_step 
		>> sp.end_step >> sp.save_internal >> sp.save_recovery_internal;

	RPCF_IO_Para& iop = this->ioPara;
	infile >> iop.output_file_prefix;
	infile >> iop.recovery_file_prefix;

	infile.close();
}


// Deal with cuda memory allocation and deallocations.
cudaPitchedPtr __myPtr[NUM_GPU];
size_t __myPPitch;
size_t __myTPitch;
int __my_pMem_allocated[NUM_GPU];
int __my_tMem_allocated[NUM_GPU];
int __my_pSize;
int __my_tSize;
size_t __myMaxMemorySize[NUM_GPU];
#define my_MAX(a,b) a>b?a:b

__host__ void initMyCudaMalloc(dim3 dims) {
	// get memory allignment factor
	cudaPitchedPtr testPtr;
	
	int mx = dims.x;
	int my = dims.y;
	int mz = dims.z;
	int nx = mx/ 3 * 2;
	int ny = my/ 3 * 2;
	int pz = mz / 2 + 1;

	cudaExtent ext = make_cudaExtent(
		sizeof(cuRPCF::complex)*(mx/2+1),my,pz
	);
	cuCheck(cudaMalloc3D(&testPtr, ext), "mem test");
	__myPPitch = testPtr.pitch;
	cudaFree(testPtr.ptr);

	ext = make_cudaExtent(
		sizeof(cuRPCF::complex)*mz, nx / 2 + 1, ny
	);
	cuCheck(cudaMalloc3D(&testPtr, ext), "mem test");
	__myTPitch = testPtr.pitch;
	cudaFree(testPtr.ptr);

	__my_pSize = __myPPitch * my * pz;
	__my_tSize = __myTPitch * (nx / 2 + 1) * ny;
	size_t maxSize = __my_pSize>__my_tSize ? __my_pSize : __my_tSize;

	__myMaxMemorySize[0] = my_MAX(maxSize * 8, 1024*1024*1024);
	if(NUM_GPU>1) __myMaxMemorySize[1] = maxSize * 3;

	// allocate the whole memory at one time to save time.
	ext = make_cudaExtent(__myMaxMemorySize[0], 1, 1);
	cuCheck(cudaSetDevice(dev_id[0]), "set device");
	cuCheck(cudaMalloc3D(&__myPtr[0], ext), "my cuda malloc");
	//cuCheck(cudaMemset(__myPtr[0].ptr, -1, __myMaxMemorySize[0]), "memset");

	// allocate the peer device memory
	if (NUM_GPU > 1) {
		ext = make_cudaExtent(__myMaxMemorySize[1], 1, 1);
		cuCheck(cudaSetDevice(dev_id[1]), "set device");
		cuCheck(cudaMalloc3D(&__myPtr[1], ext), "my cuda malloc");
	}
	//cuCheck(cudaMemset(__myPtr[1].ptr, -1, __myMaxMemorySize[1]), "memset");

	__my_pMem_allocated[0] = 0;
	__my_tMem_allocated[0] = 0;
	
	if (NUM_GPU > 1) {
		__my_pMem_allocated[1] = 0;
		__my_tMem_allocated[1] = 0;
	}

	cuCheck(cudaSetDevice(dev_id[0]), "set device");
}

__host__ void* get_fft_buffer_ptr(int dev_id) {
	assert(dev_id < NUM_GPU);
	return __myPtr[dev_id].ptr;
}

__host__ cudaError_t myCudaMalloc(cudaPitchedPtr& Ptr, myCudaMemType type, int dev_id) {
	assert(dev_id < NUM_GPU);
	if (__my_pMem_allocated[dev_id] + __my_tMem_allocated[dev_id] >= 7)return cudaErrorMemoryAllocation;
	if (type == XYZ_3D) {
		// check if memory is already used up.
		if (__my_pMem_allocated[dev_id] >= 6)return cudaErrorMemoryAllocation;
		Ptr.pitch = __myPPitch;
		Ptr.ptr = (char*)__myPtr[dev_id].ptr + (__my_pMem_allocated[dev_id]+1)*__my_pSize;
		__my_pMem_allocated[dev_id]++;
		return cudaSuccess;
	}
	else if (type == ZXY_3D) {
		// check if memory is already used up.
		if (__my_tMem_allocated[dev_id] >= 6)return cudaErrorMemoryAllocation;
		Ptr.pitch = __myTPitch;
		Ptr.ptr = (char*)__myPtr[dev_id].ptr + __myMaxMemorySize[dev_id]
			- (__my_tMem_allocated[dev_id]+1)*__my_tSize;
		__my_tMem_allocated[dev_id]++;
		return cudaSuccess;
	}
	else {
		return cudaErrorInvalidValue;	//WRONG TYPE	
	}
}

__host__ cudaError_t myCudaFree(cudaPitchedPtr& Ptr, myCudaMemType type, int dev_id){
	assert(dev_id < NUM_GPU);
	if (type == XYZ_3D) {
		if (__my_pMem_allocated[dev_id] <= 0) return cudaErrorMemoryAllocation;
		int i = ((char*)Ptr.ptr - (char*)__myPtr[dev_id].ptr) / __my_pSize;
		assert(((char*)Ptr.ptr - (char*)__myPtr[dev_id].ptr) % __my_pSize == 0);
		// the next memory to free must be the last memory of allocated block.
		if (__my_pMem_allocated[dev_id] != i) return cudaErrorInvalidValue;
		Ptr.ptr = NULL;
		__my_pMem_allocated[dev_id]--;
		return cudaSuccess;
	}
	else if (type == ZXY_3D) {
		if (__my_tMem_allocated[dev_id] <= 0) return cudaErrorMemoryAllocation;
		int i = ((char*)__myPtr[dev_id].ptr + __myMaxMemorySize[dev_id] - (char*)Ptr.ptr) / __my_tSize;
		assert(((char*)__myPtr[dev_id].ptr + __myMaxMemorySize[dev_id] - (char*)Ptr.ptr) % __my_tSize == 0);
		if (__my_tMem_allocated[dev_id] != i) return cudaErrorInvalidValue;
		Ptr.ptr = NULL;
		__my_tMem_allocated[dev_id]--;
		return cudaSuccess;
	}
	else {
		return cudaErrorInvalidValue;
	}
}

__host__ void destroyMyCudaMalloc(int _dev_id) {
	assert(_dev_id < NUM_GPU);
	for (int i = 0; i < NUM_GPU; i++) {
		cuCheck(cudaFree(__myPtr[dev_id[i]].ptr), "destroy allocator");
	}
}

string get_file_name(string prefix, int num, string suffix) {
	string filename;
	ostringstream s_num;
	s_num << num;
	filename = prefix + s_num.str() + string(".") + suffix;
	return filename;
}