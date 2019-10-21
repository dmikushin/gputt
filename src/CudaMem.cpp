
#include "CudaMem.h"

#ifdef CUTT_HAS_UMPIRE
#include <umpire/Umpire.hpp>
#endif


// global Umpire allocator
#ifdef CUTT_HAS_UMPIRE
auto cutt_umpire_allocator = umpire::ResourceManager::getInstance().getAllocator(CUTT_USES_THIS_UMPIRE_ALLOCATOR);
#endif

//----------------------------------------------------------------------------------------
//
// Allocate gpu memory
// pp = memory pointer
// len = length of the array
//
void allocate_device_T(void **pp, const size_t len, const size_t sizeofT) {
#ifdef CUTT_HAS_UMPIRE
  *pp = cutt_umpire_allocator.allocate(sizeofT*len);
#else
  cudaCheck(cudaMalloc(pp, sizeofT*len));
#endif
}

//----------------------------------------------------------------------------------------
//
// Deallocate gpu memory
// pp = memory pointer
//
void deallocate_device_T(void **pp) {
#ifdef CUTT_HAS_UMPIRE
  cutt_umpire_allocator.deallocate((void *) (*pp) );
#else
  if (*pp != NULL) {
    cudaCheck(cudaFree((void *)(*pp)));
    *pp = NULL;
  }
#endif
}