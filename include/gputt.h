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
#ifndef CUTT_H
#define CUTT_H

#include "gpu_runtime.h" // gpuStream_t

#ifdef _WIN32
#ifdef gputt_EXPORTS
#define CUTT_API __declspec(dllexport)
#else
#define CUTT_API __declspec(dllimport)
#endif
#else // _WIN32
#define CUTT_API
#endif // _WIN32

// Handle type that is used to store and access gputt plans
typedef unsigned int gputtHandle;

// Return value
typedef enum CUTT_API gputtResult_t {
  CUTT_SUCCESS,            // Success
  CUTT_INVALID_PLAN,       // Invalid plan handle
  CUTT_INVALID_PARAMETER,  // Invalid input parameter
  CUTT_INVALID_DEVICE,     // Execution tried on device different than where plan was created
  CUTT_INTERNAL_ERROR,     // Internal error
  CUTT_UNDEFINED_ERROR,    // Undefined error
} gputtResult;

//
// Create plan
//
// Parameters
// handle            = Returned handle to gpuTT plan
// rank              = Rank of the tensor
// dim[rank]         = Dimensions of the tensor
// permutation[rank] = Transpose permutation
// sizeofType        = Size of the elements of the tensor in bytes (=4 or 8)
// stream            = CUDA stream (0 if no stream is used)
//
// Returns
// Success/unsuccess code
// 
gputtResult CUTT_API gputtPlan(gputtHandle* handle, int rank, const int* dim, const int* permutation, size_t sizeofType,
  gpuStream_t stream);

//
// Create plan and choose implementation by measuring performance
//
// Parameters
// handle            = Returned handle to gpuTT plan
// rank              = Rank of the tensor
// dim[rank]         = Dimensions of the tensor
// permutation[rank] = Transpose permutation
// sizeofType        = Size of the elements of the tensor in bytes (=4 or 8)
// stream            = CUDA stream (0 if no stream is used)
// idata             = Input data size product(dim)
// odata             = Output data size product(dim)
//
// Returns
// Success/unsuccess code
// 
gputtResult CUTT_API gputtPlanMeasure(gputtHandle* handle, int rank, const int* dim, const int* permutation, size_t sizeofType,
  gpuStream_t stream, const void* idata, void* odata, const void* alpha = NULL, const void* beta = NULL);

//
// Destroy plan
//
// Parameters
// handle            = Handle to the gpuTT plan
// 
// Returns
// Success/unsuccess code
//
gputtResult CUTT_API gputtDestroy(gputtHandle handle);

//
// Execute plan out-of-place; performs a tensor transposition of the form \f[ \mathcal{B}_{\pi(i_0,i_1,...,i_{d-1})} \gets \alpha * \mathcal{A}_{i_0,i_1,...,i_{d-1}} + \beta * \mathcal{B}_{\pi(i_0,i_1,...,i_{d-1})}, \f]
//
// Parameters
// handle            = Returned handle to gpuTT plan
// idata             = Input data size product(dim)
// odata             = Output data size product(dim)
// alpha             = scalar for input
// beta              = scalar for output
// 
// Returns
// Success/unsuccess code
//
gputtResult CUTT_API gputtExecute(gputtHandle handle, const void* idata, void* odata, const void* alpha = NULL, const void* beta = NULL);

#endif // CUTT_H
