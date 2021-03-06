#include "test_transform.h"
#include "../include/cuRPCF.h"
#include <iostream>
using namespace std;
#include "test_nonlinear.h"
#include <cassert>

void compareFlow(problem& pb);
void compare_S2P_Flow(problem& pb);
void setFlowForSpectra(problem& pb);
void compareSpectra(problem& pb);

TestResult test_transform() {
	//problem pb;
	RPCF_Paras para("parameter.txt");
	problem pb(para);
	allocDeviceMem(pb);
	initFFT(pb);
	
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pitch = pb.pitch;

	size_t size = pitch * my * mz;
	//memory allocation
	pb.hptr_u = (REAL*)malloc(size);
	assert(pb.hptr_u != nullptr);
	pb.hptr_v = (REAL*)malloc(size);
	assert(pb.hptr_v != nullptr);
	pb.hptr_w = (REAL*)malloc(size);
	assert(pb.hptr_w != nullptr);
	pb.hptr_omega_x = (REAL*)malloc(size);
	assert(pb.hptr_omega_x != nullptr);
	pb.hptr_omega_y = (REAL*)malloc(size);
	assert(pb.hptr_omega_y != nullptr);
	pb.hptr_omega_z = (REAL*)malloc(size);
	assert(pb.hptr_omega_z != nullptr);

	setFlow(pb);

	int dim[3] = { pb.mx,pb.my,pb.mz };
	int tdim[3] = { pb.mz,pb.mx,pb.my };

	//myCudaFree(pb.dptr_omega_z, XYZ_3D);
	//myCudaFree(pb.dptr_omega_y, XYZ_3D);
	//myCudaFree(pb.dptr_omega_x, XYZ_3D);
	//myCudaFree(pb.dptr_w, XYZ_3D);
	//myCudaFree(pb.dptr_v, XYZ_3D);

	Padding_mode pad = Padding;
	transform_3d_one(FORWARD, pb.dptr_omega_z, pb.dptr_tomega_z, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_omega_y, pb.dptr_tomega_y, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_omega_x, pb.dptr_tomega_x, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_w, pb.dptr_tw, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_v, pb.dptr_tv, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_u, pb.dptr_tu, dim, tdim, pad);

	transform_3d_one(BACKWARD, pb.dptr_u, pb.dptr_tu, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_v, pb.dptr_tv, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_w, pb.dptr_tw, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_x, pb.dptr_tomega_x, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_y, pb.dptr_tomega_y, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_z, pb.dptr_tomega_z, dim, tdim, pad);

	//transform_3d_one(FORWARD, pb.dptr_u, pb.dptr_tu, dim, tdim);
	//transform_3d_one(BACKWARD, pb.dptr_u, pb.dptr_tu, dim, tdim);
	//transform_3d_one(FORWARD, pb.dptr_u, pb.dptr_tu, dim, tdim);
	//transform_3d_one(BACKWARD, pb.dptr_u, pb.dptr_tu, dim, tdim);

	//-------------------------------
	compareFlow(pb);

	//problem pb2;
	//initCUDA(pb2);
	//initFFT(pb2);
	//int mx = pb2.mx;
	//int my = pb2.my;
	//int mz = pb2.mz;
	//int pitch = pb2.pitch;
	//size_t size2 = pitch * my * mz;
	////memory allocation
	//pb2.hptr_u = (REAL*)malloc(size2);
	//assert(pb2.hptr_u != nullptr);
	//int dim2[3] = { pb2.mx,pb2.my,pb2.mz };
	//int tdim2[3] = { pb2.mz,pb2.mx,pb2.my };
	//setFlowForSpectra(pb2);
	//transform_3d_one(FORWARD, pb2.dptr_u, pb2.dptr_tu, dim2, tdim2);
	//compareSpectra(pb2);

	//

	transform_3d_one(FORWARD, pb.dptr_omega_z, pb.dptr_tomega_z, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_omega_y, pb.dptr_tomega_y, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_omega_x, pb.dptr_tomega_x, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_w, pb.dptr_tw, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_v, pb.dptr_tv, dim, tdim, pad);
	transform_3d_one(FORWARD, pb.dptr_u, pb.dptr_tu, dim, tdim, pad);

	pad = No_Padding;

	transform_3d_one(BACKWARD, pb.dptr_u, pb.dptr_tu, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_v, pb.dptr_tv, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_w, pb.dptr_tw, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_x, pb.dptr_tomega_x, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_y, pb.dptr_tomega_y, dim, tdim, pad);
	transform_3d_one(BACKWARD, pb.dptr_omega_z, pb.dptr_tomega_z, dim, tdim, pad);

	compare_S2P_Flow(pb);


	return TestSuccess;
}

void compareFlow(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = (mz / 2 + 1);
	size_t pitch = pb.pitch;
	REAL lx = pb.lx;
	REAL ly = pb.ly;
	REAL* u = pb.hptr_u;
	REAL* oy = pb.hptr_omega_y;
	REAL* oz = pb.hptr_omega_z;

	size_t size = pitch * my * mz;
	cuCheck(cudaMemcpy(pb.hptr_u, pb.dptr_u.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_y, pb.dptr_omega_y.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_z, pb.dptr_omega_z.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaDeviceSynchronize(),"Sync");
	REAL PRECISION = 1e-8;

	REAL PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				REAL x = lx * i / mx;
				REAL y = ly * j / my;
				REAL z = cos(REAL(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(REAL) + i;
				REAL ex_u = (1 - z*z)*sin(y)*cos(x);
				REAL ex_oy = -2 * z * sin(y)*cos(x);
				REAL ex_oz = -(1 - z*z)*cos(y)*cos(x);

				assert(isEqual(ex_u, u[inc], PRECISION));
				assert(isEqual(ex_oy, oy[inc], PRECISION));
				assert(isEqual(ex_oz, oz[inc], PRECISION));
			}
}


void compareSpectra(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pitch = pb.tPitch;
	REAL lx = pb.lx;
	REAL ly = pb.ly;
	cuRPCF::complex* u;
	size_t size = pitch * (mx / 2 + 1)*my;
	u =(cuRPCF::complex*)malloc(size);
	assert(u != nullptr);
	cuCheck(cudaMemcpy(u, pb.dptr_tu.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	REAL PRECISION = 1e-8;

	REAL PI = 4.0*atan(1.0);
	for (int j = 0; j < my; j++)
		for (int i = 0; i < mx/2+1; i++)
			for (int k=0; k < mz/2+1; k++)
			{
				size_t inc = pitch*((mx / 2 + 1)*j + i) / sizeof(cuRPCF::complex) + k;
				if (i == 0 && j == 0) {
					if (k == 0) {
						assert(isEqual(u[inc].re, 0.5, PRECISION));
					}
					else if (k == 2) {
						assert(isEqual(u[inc].re, -0.5, PRECISION));
					}
					else {
						assert(isEqual(u[inc].re, 0.0, PRECISION));
					}				
				}
				else {
					assert(isEqual(u[inc].re, 0.0, PRECISION));
				}
				assert(isEqual(u[inc].im, 0.0, PRECISION));
			}
}


void setFlowForSpectra(problem& pb) {
	int px = pb.mx;
	int py = pb.my;
	int pz = pb.mz/2+1;
	int pitch = pb.pitch;
	REAL lx = pb.lx;
	REAL ly = pb.ly;
	REAL* u = pb.hptr_u;
	size_t size = pitch * py * pz;

	REAL PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < py; j++)
			for (int i = 0; i < px; i++)
			{
				REAL x = lx * i / px;
				REAL y = ly * j / py;
				REAL z = cos(REAL(k) / (pz - 1)*PI);
				size_t inc = (pitch * py * k + pitch *j) / sizeof(REAL) + i;
				u[inc] = 1 - z*z;
			}

	cuCheck(cudaMemcpy(pb.dptr_u.ptr, pb.hptr_u, size, cudaMemcpyHostToDevice), "memcpy");
}

void compare_S2P_Flow(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = (mz / 2 + 1);
	int nz = (mz / 4 + 1);
	size_t pitch = pb.pitch;
	REAL lx = pb.lx;
	REAL ly = pb.ly;
	REAL* u = pb.hptr_u;
	REAL* oy = pb.hptr_omega_y;
	REAL* oz = pb.hptr_omega_z;

	size_t size = pitch * my * mz;
	cuCheck(cudaMemcpy(pb.hptr_u, pb.dptr_u.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_y, pb.dptr_omega_y.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_z, pb.dptr_omega_z.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaDeviceSynchronize(), "Sync");
	REAL PRECISION = 1e-8;

	REAL PI = 4.0*atan(1.0);
	for (int k = 0; k < nz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				REAL x = lx * i / mx;
				REAL y = ly * j / my;
				REAL z = cos(REAL(k) / (nz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(REAL) + i;
				REAL ex_u = (1 - z*z)*sin(y)*cos(x);
				REAL ex_oy = -2 * z * sin(y)*cos(x);
				REAL ex_oz = -(1 - z*z)*cos(y)*cos(x);

				assert(isEqual(ex_u, u[inc], PRECISION));
				assert(isEqual(ex_oy, oy[inc], PRECISION));
				assert(isEqual(ex_oz, oz[inc], PRECISION));
			}
}