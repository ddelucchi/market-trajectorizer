# CUDA detection helper for optional research/parity path.
# Sets MT_CUDA_AVAILABLE and prints status; does not hard-fail CPU baseline builds.

include(CheckLanguage)

set(MT_CUDA_AVAILABLE OFF)

check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
  find_package(CUDAToolkit QUIET)
  if(CUDAToolkit_FOUND)
    set(MT_CUDA_AVAILABLE ON)
    message(STATUS "CUDA Toolkit version: ${CUDAToolkit_VERSION}")
  else()
    message(STATUS "CUDA compiler detected but CUDAToolkit package not found; CUDA path disabled.")
  endif()
else()
  message(STATUS "CUDA compiler not found; building deterministic CPU theorem-scaffold mode (not production-authoritative).")
endif()
