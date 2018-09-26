#include "test_nonlinear.h"
#include "cuRPCF.h"
#include <iostream>
#include <assert.h>
using namespace std;
#include "nonlinear.cuh"
#include "main_test.h"


TestResult test_nonlinear() {
	//problem pb;
	//initCUDA(pb);
	//initFFT(pb);

	//int mx = pb.mx;
	//int my = pb.my;
	//int mz = pb.mz;
	//int pitch = pb.pitch;

	//size_t size = pitch * my * mz;

	////memory allocation
	//pb.hptr_u = (real*)malloc(size);
	//assert(pb.hptr_u != nullptr);
	//pb.hptr_v = (real*)malloc(size);
	//assert(pb.hptr_v != nullptr);
	//pb.hptr_w = (real*)malloc(size);
	//assert(pb.hptr_w != nullptr);
	//pb.hptr_omega_x = (real*)malloc(size);
	//assert(pb.hptr_omega_x != nullptr);
	//pb.hptr_omega_y = (real*)malloc(size);
	//assert(pb.hptr_omega_y != nullptr);
	//pb.hptr_omega_z = (real*)malloc(size);
	//assert(pb.hptr_omega_z != nullptr);

	//
	//set_lamb_nonlinear_basic(pb);
	//transform(FORWARD, pb);
	////compute rhs nonlinear part;
	//rhsNonlinear(pb);
	////validate output
	//assert(check_nonlinear_basic(pb) == TestSuccess);
	RPCF_Paras para("parameter.txt");
	problem pb2(para);
	initCUDA(pb2);
	initFFT(pb2);
	int mx = pb2.mx;
	int my = pb2.my;
	int mz = pb2.mz;
	int pitch = pb2.pitch;
	size_t size = pitch * my * mz;
	//memory allocation
	pb2.hptr_u = (real*)malloc(size);
	assert(pb2.hptr_u != nullptr);
	pb2.hptr_v = (real*)malloc(size);
	assert(pb2.hptr_v != nullptr);
	pb2.hptr_w = (real*)malloc(size);
	assert(pb2.hptr_w != nullptr);
	pb2.hptr_omega_x = (real*)malloc(size);
	assert(pb2.hptr_omega_x != nullptr);
	pb2.hptr_omega_y = (real*)malloc(size);
	assert(pb2.hptr_omega_y != nullptr);
	pb2.hptr_omega_z = (real*)malloc(size);
	assert(pb2.hptr_omega_z != nullptr);


	//set_lamb_nonlinear_basic2(pb2);
	//transform(FORWARD, pb2);
	////compute rhs nonlinear part;
	//rhsNonlinear(pb2);
	////validate output
	//assert(check_nonlinear_basic2(pb2) == TestSuccess);

	////impose input field
	//setFlow(pb2);	
	////compute lamb vector
	//computeLambVector(pb2);
	////validate lamb vector
	//assert(check_lamb(pb2)==TestSuccess);
	//transform(FORWARD, pb2);
	////compute rhs nonlinear part;
	//rhsNonlinear(pb2);
	////validate output
	//assert(check_nonlinear_complex(pb2) == TestSuccess);

	//impose input field
	setFlow_basic3(pb2);
	//compute lamb vector
	computeLambVector(pb2);
	//validate lamb vector
	assert(check_lamb_basic3(pb2) == TestSuccess);
	transform(FORWARD, pb2);
	//compute rhs nonlinear part;
	rhsNonlinear(pb2);
	//validate output
	assert(check_nonlinear_basic3(pb2) == TestSuccess);

	return TestSuccess;
}

void setFlow(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.pz;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* u = pb.hptr_u;
	real* v = pb.hptr_v;
	real* w = pb.hptr_w;
	real* ox = pb.hptr_omega_x;
	real* oy = pb.hptr_omega_y;
	real* oz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				u[inc] = (1 - z*z)*sin(y)*cos(x);
				v[inc] = 0.0;
				w[inc] = 0.0;
				ox[inc] = 0.0;
				oy[inc] = 2 * z * sin(y)*cos(x);
				oz[inc] = (1-z*z)*cos(y)*cos(x);
			}

	cuCheck(cudaMemcpy(pb.dptr_u.ptr, pb.hptr_u, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_v.ptr, pb.hptr_v, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_w.ptr, pb.hptr_w, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_x.ptr, pb.hptr_omega_x, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_y.ptr, pb.hptr_omega_y, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_z.ptr, pb.hptr_omega_z, size, cudaMemcpyHostToDevice), "memcpy");

}

void setFlow_basic3(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.pz;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* u = pb.hptr_u;
	real* v = pb.hptr_v;
	real* w = pb.hptr_w;
	real* ox = pb.hptr_omega_x;
	real* oy = pb.hptr_omega_y;
	real* oz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				u[inc] = (1 - z*z)*sin(y);
				v[inc] = 0.0;
				w[inc] = 0.0;
				ox[inc] = 0.0;
				oy[inc] = 2 * z * sin(y);
				oz[inc] = (1 - z*z)*cos(y);
			}

	cuCheck(cudaMemcpy(pb.dptr_u.ptr, pb.hptr_u, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_v.ptr, pb.hptr_v, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_w.ptr, pb.hptr_w, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_x.ptr, pb.hptr_omega_x, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_y.ptr, pb.hptr_omega_y, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_omega_z.ptr, pb.hptr_omega_z, size, cudaMemcpyHostToDevice), "memcpy");

}


TestResult check_lamb(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* lambx = pb.hptr_omega_x;
	real* lamby = pb.hptr_omega_y;
	real* lambz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	// cpy data from gpu memory to cpu memory
	cuCheck(cudaMemcpy(pb.hptr_omega_x, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_y, pb.dptr_lamb_y.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_z, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost), "memcpy");

	real PRECISION = 1e-8;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				real ex_lambx = 0.0;
				real ex_lamby = (1 - z*z)*(1 - z*z)*cos(y)*sin(y)*cos(x)*cos(x);
				real ex_lambz = -2.0*z*(1 - z*z)*sin(y)*sin(y)*cos(x)*cos(x);
				assert(isEqual(lambx[inc], ex_lambx, PRECISION));
				assert(isEqual(lamby[inc], ex_lamby, PRECISION));
				assert(isEqual(lambz[inc], ex_lambz, PRECISION));
			}
	return TestSuccess;
}


TestResult check_lamb_basic3(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* lambx = pb.hptr_omega_x;
	real* lamby = pb.hptr_omega_y;
	real* lambz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	// cpy data from gpu memory to cpu memory
	cuCheck(cudaMemcpy(pb.hptr_omega_x, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_y, pb.dptr_lamb_y.ptr, size, cudaMemcpyDeviceToHost), "memcpy");
	cuCheck(cudaMemcpy(pb.hptr_omega_z, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost), "memcpy");

	real PRECISION = 1e-8;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				real ex_lambx = 0.0;
				real ex_lamby = (1 - z*z)*(1 - z*z)*cos(y)*sin(y);
				real ex_lambz = -2.0*z*(1 - z*z)*sin(y)*sin(y);
				assert(isEqual(lambx[inc], ex_lambx, PRECISION));
				assert(isEqual(lamby[inc], ex_lamby, PRECISION));
				assert(isEqual(lambz[inc], ex_lambz, PRECISION));
			}
	return TestSuccess;
}

void set_lamb_nonlinear_basic(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* lambx = pb.hptr_omega_x;
	real* lamby = pb.hptr_omega_y;
	real* lambz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				real ex_lambx = (1 - z*z)*sin(x);
				real ex_lamby = 0.0;
				real ex_lambz = 0.0;
				lambx[inc] = ex_lambx;
				lamby[inc] = ex_lamby;
				lambz[inc] = ex_lambz;
			}
	// cpy data from gpu memory to cpu memory
	cuCheck(cudaMemcpy(pb.dptr_lamb_x.ptr, pb.hptr_omega_x, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_lamb_y.ptr, pb.hptr_omega_y, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_lamb_z.ptr, pb.hptr_omega_z, size, cudaMemcpyHostToDevice), "memcpy");
}



TestResult check_nonlinear_basic(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* nonlinear_v = pb.hptr_omega_x;
	real* nonlinear_omega_y = pb.hptr_omega_y;

	size_t size = pitch * my * mz;
	real alpha = pb.aphi;
	real beta = pb.beta;

	real PRECISION = 1e-4;

	int indim[3];
	int outdim[3];

	indim[0] = pb.mx;
	indim[1] = pb.my;
	indim[2] = pb.mz;

	outdim[0] = pb.mz;
	outdim[1] = pb.mx;
	outdim[2] = pb.my;

	transform_3d_one(BACKWARD, pb.dptr_lamb_x, pb.dptr_tLamb_x, indim, outdim);
	transform_3d_one(BACKWARD, pb.dptr_lamb_z, pb.dptr_tLamb_z, indim, outdim);

	cudaError_t err;
	err = cudaMemcpy(nonlinear_v, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);
	err = cudaMemcpy(nonlinear_omega_y, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				/*real ex_rhsn_v = -2.0*(z - z*z*z)*(cos(2 * x)*cos(2 * y)
				+ cos(2 * y) - sin(2 * x)*sin(2 * y))
				+ (1 - z*z)*(1 - z*z)*sin(2 * x)*(2 * cos(2 * x) + 1);
				real ex_rhsn_o = 4*z*(1-z*z)*sin(y)*cos(x)*(
				sin(y)*sin(x) + cos(y)*cos(x));*/
				real ex_rhsn_v = cos(x)*(-2 * z);
				real ex_rhsn_o = 0.0;
				assert(isEqual(ex_rhsn_v, nonlinear_v[inc], PRECISION));
				assert(isEqual(ex_rhsn_o, nonlinear_omega_y[inc], PRECISION));
			}

	return TestSuccess;
}


TestResult check_nonlinear_basic3(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* nonlinear_v = pb.hptr_omega_x;
	real* nonlinear_omega_y = pb.hptr_omega_y;

	size_t size = pitch * my * mz;
	real alpha = pb.aphi;
	real beta = pb.beta;

	real PRECISION = 1e-4;

	int indim[3];
	int outdim[3];

	indim[0] = pb.mx;
	indim[1] = pb.my;
	indim[2] = pb.mz;

	outdim[0] = pb.mz;
	outdim[1] = pb.mx;
	outdim[2] = pb.my;

	transform_3d_one(BACKWARD, pb.dptr_lamb_x, pb.dptr_tLamb_x, indim, outdim);
	transform_3d_one(BACKWARD, pb.dptr_lamb_z, pb.dptr_tLamb_z, indim, outdim);

	cudaError_t err;
	err = cudaMemcpy(nonlinear_v, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);
	err = cudaMemcpy(nonlinear_omega_y, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				/*real ex_rhsn_v = -2.0*(z - z*z*z)*(cos(2 * x)*cos(2 * y)
				+ cos(2 * y) - sin(2 * x)*sin(2 * y))
				+ (1 - z*z)*(1 - z*z)*sin(2 * x)*(2 * cos(2 * x) + 1);
				real ex_rhsn_o = 4*z*(1-z*z)*sin(y)*cos(x)*(
				sin(y)*sin(x) + cos(y)*cos(x));*/
				real ex_rhsn_v = 0.0; 4 * z*(1 - z*z)*cos(2 * y);
				real ex_rhsn_o = 0.0;
				assert(isEqual(ex_rhsn_v, nonlinear_v[inc], PRECISION));
				assert(isEqual(ex_rhsn_o, nonlinear_omega_y[inc], PRECISION));
			}

	return TestSuccess;
}
void set_lamb_nonlinear_basic2(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = pb.mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* lambx = pb.hptr_omega_x;
	real* lamby = pb.hptr_omega_y;
	real* lambz = pb.hptr_omega_z;
	size_t size = pitch * my * mz;

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				real ex_lambx = 0.0;
				real ex_lamby = 0.0;
				real ex_lambz = sin(23*x)*z;
				lambx[inc] = ex_lambx;
				lamby[inc] = ex_lamby;
				lambz[inc] = ex_lambz;
			}
	// cpy data from gpu memory to cpu memory
	cuCheck(cudaMemcpy(pb.dptr_lamb_x.ptr, pb.hptr_omega_x, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_lamb_y.ptr, pb.hptr_omega_y, size, cudaMemcpyHostToDevice), "memcpy");
	cuCheck(cudaMemcpy(pb.dptr_lamb_z.ptr, pb.hptr_omega_z, size, cudaMemcpyHostToDevice), "memcpy");
}



TestResult check_nonlinear_basic2(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* nonlinear_v = pb.hptr_omega_x;
	real* nonlinear_omega_y = pb.hptr_omega_y;

	size_t size = pitch * my * mz;
	real alpha = pb.aphi;
	real beta = pb.beta;

	real PRECISION = 1e-8;

	int indim[3];
	int outdim[3];

	indim[0] = pb.mx;
	indim[1] = pb.my;
	indim[2] = pb.mz;

	outdim[0] = pb.mz;
	outdim[1] = pb.mx;
	outdim[2] = pb.my;

	transform_3d_one(BACKWARD, pb.dptr_lamb_x, pb.dptr_tLamb_x, indim, outdim);
	transform_3d_one(BACKWARD, pb.dptr_lamb_z, pb.dptr_tLamb_z, indim, outdim);

	cudaError_t err;
	err = cudaMemcpy(nonlinear_v, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);
	err = cudaMemcpy(nonlinear_omega_y, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				/*real ex_rhsn_v = -2.0*(z - z*z*z)*(cos(2 * x)*cos(2 * y)
				+ cos(2 * y) - sin(2 * x)*sin(2 * y))
				+ (1 - z*z)*(1 - z*z)*sin(2 * x)*(2 * cos(2 * x) + 1);
				real ex_rhsn_o = 4*z*(1-z*z)*sin(y)*cos(x)*(
				sin(y)*sin(x) + cos(y)*cos(x));*/
				real ex_rhsn_v = +23*23*sin(23*x)*z;
				real ex_rhsn_o = 0.0;
				assert(isEqual(ex_rhsn_v, nonlinear_v[inc], PRECISION));
				assert(isEqual(ex_rhsn_o, nonlinear_omega_y[inc], PRECISION));
			}

	return TestSuccess;
}


TestResult check_nonlinear_complex(problem& pb) {
	int mx = pb.mx;
	int my = pb.my;
	int mz = pb.mz;
	int pz = mz / 2 + 1;
	int pitch = pb.pitch;
	real lx = pb.lx;
	real ly = pb.ly;
	real* nonlinear_v = pb.hptr_omega_x;
	real* nonlinear_omega_y = pb.hptr_omega_y;

	size_t size = pitch * my * mz;
	real alpha = pb.aphi;
	real beta = pb.beta;

	real PRECISION = 1e-8;

	int indim[3];
	int outdim[3];

	indim[0] = pb.mx;
	indim[1] = pb.my;
	indim[2] = pb.mz;

	outdim[0] = pb.mz;
	outdim[1] = pb.mx;
	outdim[2] = pb.my;

	transform_3d_one(BACKWARD, pb.dptr_lamb_x, pb.dptr_tLamb_x, indim, outdim);
	transform_3d_one(BACKWARD, pb.dptr_lamb_z, pb.dptr_tLamb_z, indim, outdim);

	cudaError_t err;
	err = cudaMemcpy(nonlinear_v, pb.dptr_lamb_x.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);
	err = cudaMemcpy(nonlinear_omega_y, pb.dptr_lamb_z.ptr, size, cudaMemcpyDeviceToHost);
	ASSERT(err == cudaSuccess);

	real PI = 4.0*atan(1.0);
	for (int k = 0; k < pz; k++)
		for (int j = 0; j < my; j++)
			for (int i = 0; i < mx; i++)
			{
				real x = lx * i / mx;
				real y = ly * j / my;
				real z = cos(real(k) / (pz - 1)*PI);
				size_t inc = (pitch * my * k + pitch *j) / sizeof(real) + i;
				real ex_rhsn_v = -(1 - z*z)*(2*z)*(cos(2 * x) - cos(2 * y) - 2 * cos(2 * y)*cos(2 * x))
					+ 2*(1-z*z)*(-2*z)*cos(2*y)*cos(x)*cos(x);
					//-2.0*(z - z*z*z)*(cos(2 * x)*cos(2 * y)
				//+ cos(2 * y) - sin(2 * x)*sin(2 * y))
				//+ (1 - z*z)*(1 - z*z)*sin(2 * x)*(2 * cos(2 * x) + 1);
				real ex_rhsn_o = -(1 - z*z)*(1 - z*z)*cos(y)*sin(y)*2*cos(x)*sin(x);
					//4*z*(1-z*z)*sin(y)*cos(x)*(
				//sin(y)*sin(x) + cos(y)*cos(x));
				//real ex_rhsn_v = cos(x)*(-2 * z);
				//real ex_rhsn_o = 0.0;
				assert(isEqual(ex_rhsn_v, nonlinear_v[inc], PRECISION));
				assert(isEqual(ex_rhsn_o, nonlinear_omega_y[inc], PRECISION));
			}

	return TestSuccess;
}
