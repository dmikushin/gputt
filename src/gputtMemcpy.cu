/******************************************************************************
MIT License

Copyright (c) 2016 Antti-Pekka Hynninen
Copyright (c) 2016 Oak Ridge National Laboratory (UT-Batelle)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include "gputtMemcpy.h"
#include "gputtUtils.h"

const int numthread = 64;

// -----------------------------------------------------------------------------------
//
// Copy using scalar loads and stores
//
template <typename T>
__global__ void scalarCopyKernel(const int n, const T *data_in, T *data_out) {

  for (int i = threadIdx.x + blockIdx.x * blockDim.x; i < n;
       i += blockDim.x * gridDim.x) {
    data_out[i] = data_in[i];
  }
}
template <typename T>
void scalarCopy(const int n, const T *data_in, T *data_out,
                gpuStream stream) {

  int numblock = (n - 1) / numthread + 1;
  // numblock = min(65535, numblock);
  // numblock = min(256, numblock);

  scalarCopyKernel<T><<<numblock, numthread, 0, stream>>>(n, data_in, data_out);

  gpuCheck(gpuGetLastError());
}
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
//
// Copy using vectorized loads and stores
//
template <typename T>
__global__ void vectorCopyKernel(const int n, T *data_in, T *data_out) {

  // Maximum vector load is 128 bits = 16 bytes
  const int vectorLength = 16 / sizeof(T);

  int idx = threadIdx.x + blockIdx.x * blockDim.x;

  // Vector elements
  for (int i = idx; i < n / vectorLength; i += blockDim.x * gridDim.x) {
    reinterpret_cast<int4 *>(data_out)[i] =
        reinterpret_cast<int4 *>(data_in)[i];
  }

  // Remaining elements
  for (int i = idx + (n / vectorLength) * vectorLength; i < n;
       i += blockDim.x * gridDim.x + threadIdx.x) {
    data_out[i] = data_in[i];
  }
}

template <typename T>
void vectorCopy(const int n, T *data_in, T *data_out, gpuStream stream) {

  const int vectorLength = 16 / sizeof(T);

  int numblock = (n / vectorLength - 1) / numthread + 1;
  // numblock = min(65535, numblock);
  int shmemsize = 0;

  vectorCopyKernel<T>
      <<<numblock, numthread, shmemsize, stream>>>(n, data_in, data_out);

  gpuCheck(gpuGetLastError());
}
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
//
// Copy using vectorized loads and stores
//
template <int numElem>
__global__ void memcpyFloatKernel(const int n, float4 *data_in,
                                  float4 *data_out) {
  int index = threadIdx.x + numElem * blockIdx.x * blockDim.x;
  float4 a[numElem];
#pragma unroll
  for (int i = 0; i < numElem; i++) {
    if (index + i * blockDim.x < n)
      a[i] = data_in[index + i * blockDim.x];
  }
#pragma unroll
  for (int i = 0; i < numElem; i++) {
    if (index + i * blockDim.x < n)
      data_out[index + i * blockDim.x] = a[i];
  }
}

template <int numElem>
__global__ void memcpyFloatLoopKernel(const int n, float4 *data_in,
                                      float4 *data_out) {
  for (int index = threadIdx.x + blockIdx.x * numElem * blockDim.x; index < n;
       index += numElem * gridDim.x * blockDim.x) {
    float4 a[numElem];
#pragma unroll
    for (int i = 0; i < numElem; i++) {
      if (index + i * blockDim.x < n)
        a[i] = data_in[index + i * blockDim.x];
    }
#pragma unroll
    for (int i = 0; i < numElem; i++) {
      if (index + i * blockDim.x < n)
        data_out[index + i * blockDim.x] = a[i];
    }
  }
}

#define NUM_ELEM 2
void memcpyFloat(const int n, float *data_in, float *data_out,
                 gpuStream stream) {

  int numblock = (n / (4 * NUM_ELEM) - 1) / numthread + 1;
  int shmemsize = 0;
  memcpyFloatKernel<NUM_ELEM><<<numblock, numthread, shmemsize, stream>>>(
      n / 4, (float4 *)data_in, (float4 *)data_out);

  // int numblock = 64;
  // int shmemsize = 0;
  // memcpyFloatLoopKernel<NUM_ELEM> <<< numblock, numthread, shmemsize, stream
  // >>> (n/4, (float4 *)data_in, (float4 *)data_out);

  gpuCheck(gpuGetLastError());
}
// -----------------------------------------------------------------------------------

// Explicit instances
template void scalarCopy<int>(const int n, const int *data_in, int *data_out,
                              gpuStream stream);
template void scalarCopy<int64_t>(const int n,
                                        const int64_t *data_in,
                                        int64_t *data_out,
                                        gpuStream stream);
template void vectorCopy<int>(const int n, int *data_in, int *data_out,
                              gpuStream stream);
template void vectorCopy<int64_t>(const int n, int64_t *data_in,
                                        int64_t *data_out,
                                        gpuStream stream);
void memcpyFloat(const int n, float *data_in, float *data_out,
                 gpuStream stream);
