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
#ifndef GPUTTPLAN_H
#define GPUTTPLAN_H

#include "gputt.h"
#include "gputt_internal.h"

#include <list>
#include <vector>

// Size of the shared memory tile used in some algorithms.
// This parameter is associated with the warp (wavefront) size,
// and is therefore device-specific.
#if defined(__CUDACC__)
// Needs to be a literal constant for CUDA version, due to the use
// in the __launch_bounds__ attribute. HIP probably ignores __launch_bounds__,
// and therefore has no problem with it.
#define TILEDIM 32
#elif defined(__HIPCC__)
// AMD has released GPUs with different wavefront sizes: 64 and 32.
// So it's important to have the warpSize here as a parameter.
#define TILEDIM warpSize
#endif

const int TILEROWS = 8;

#define sizeofType(dtype) (static_cast<size_t>((dtype) & 0xff))

// Tells how tensor is split into Mm and Mk and what method is used
// NOTE: sizeMm and sizeMk fully define the split
class TensorSplit {
public:
  // Transposing method
  gputtTransposeMethod method;

  // Input volume
  int sizeMm;
  int volMm;

  // Output volume
  int sizeMk;
  int volMk;

  // {Input} U {Output}
  int sizeMmk;
  int volMmk;

  // {Input} CUT {Output} = Mk which is not in Mm
  int sizeMkBar;
  int volMkBar;

  // Remaining volume
  int sizeMbar;
  int volMbar;

  // For Packed and PackedSplit methods:
  // Amount of contigious volume
  int volMmkInCont;
  int volMmkOutCont;

  // For PackedSplit method:
  // Number of splits
  int numSplit;

  // Rank that is split
  int splitRank;
  int splitDim;

  // volMmk that is left unsplit
  int volMmkUnsplit;

  TensorSplit();

  void print();

  void update(const int sizeMm_in, const int sizeMk_in, const int rank,
              const int *dim, const int *permutation);

  // Number of elements in shared memory space
  size_t shmem() const;

  // Number of elements in Mmk that are used effectively
  size_t volMmkUsed() const;

  // Bytes the shared memory space that needs to be allocated
  // (can be larger than volShmem() due to padding)
  size_t shmemAlloc(gputtDataType dtype) const;
};

class LaunchConfig {
public:
  // Kernel launch configuration
  dim3 numthread;
  dim3 numblock;
  size_t shmemsize;

  // For the Packed method, number of registers to use for storage
  int numRegStorage;

  void print();
};

// Class that stores the plan data
class gputtPlan_t {
public:
  // Device for which this plan was made
  int deviceID;

  // CUDA stream associated with the plan
  gpuStream stream;

  // Kernel launch configuration
  LaunchConfig launchConfig;

  // Rank of the tensor
  int rank;

  // Type of the tensor elements
  gputtDataType dtype;

  TensorSplit tensorSplit;

  // Number of active thread blocks
  int numActiveBlock;

  int cuDimMk;
  int cuDimMm;

  int2 tiledVol;

  // Number of iterations of the kernel
  int num_iter;
  // Average memory level parallelism = average unroll count
  float mlp;
  int gld_req, gst_req, gld_tran, gst_tran;
  int cl_full_l2, cl_part_l2;
  int cl_full_l1, cl_part_l1;
  int sld_req, sst_req, sld_tran, sst_tran;
  double cycles;

  //--------------
  // Host buffers
  //--------------
  std::vector<TensorConvInOut> hostMbar;
  std::vector<TensorConvInOut> hostMmk;
  std::vector<TensorConv> hostMsh;

  //----------------
  // Device buffers
  //----------------
  // sizeMbar
  TensorConvInOut *Mbar;

  // sizeMmk
  TensorConvInOut *Mmk;

  // sizeMmk
  TensorConv *Msh;

  // For TiledSingleInRank
  TensorConv *Mk;

  // For TiledSingleOutRank
  TensorConv *Mm;

  gputtPlan_t();
  ~gputtPlan_t();
  void print();
  void setStream(gpuStream stream_in);
  bool countCycles(gpuDeviceProp_t &prop, const int numPosMbarSample = 0);
  void activate();
  void nullDevicePointers();

  static bool createPlans(const int rank, const int *dim,
                          const int *permutation, const int redRank,
                          const int *redDim, const int *redPermutation,
                          const gputtDataType dtype, const int deviceID,
                          const gpuDeviceProp_t &prop,
                          std::list<gputtPlan_t> &plans);

  bool operator>(const gputtPlan_t &rhs) const;
  bool operator<(const gputtPlan_t &rhs) const;

private:
  static bool createTrivialPlans(const int rank, const int *dim,
                                 const int *permutation,
                                 const gputtDataType dtype, const int deviceID,
                                 const gpuDeviceProp_t &prop,
                                 std::list<gputtPlan_t> &plans);

  static bool createTiledPlans(const int rank, const int *dim,
                               const int *permutation, const gputtDataType dtype,
                               const int deviceID, const gpuDeviceProp_t &prop,
                               std::list<gputtPlan_t> &plans);

  static bool createTiledCopyPlans(const int rank, const int *dim,
                                   const int *permutation,
                                   const gputtDataType dtype, const int deviceID,
                                   const gpuDeviceProp_t &prop,
                                   std::list<gputtPlan_t> &plans);

  static bool createPackedPlans(const int rank, const int *dim,
                                const int *permutation, const gputtDataType dtype,
                                const int deviceID, const gpuDeviceProp_t &prop,
                                std::list<gputtPlan_t> &plans);

  static bool createPackedSplitPlans(const int rank, const int *dim,
                                     const int *permutation,
                                     const gputtDataType dtype,
                                     const int deviceID,
                                     const gpuDeviceProp_t &prop,
                                     std::list<gputtPlan_t> &plans);

  bool setup(const int rank_in, const int *dim, const int *permutation,
             const gputtDataType dtype_in, const TensorSplit &tensorSplit_in,
             const LaunchConfig &launchConfig_in, const int numActiveBlock_in);
};

void reduceRanks(const int rank, const int *dim, const int *permutation,
                 std::vector<int> &redDim, std::vector<int> &redPermutation);

#endif // GPUTTPLAN_H
