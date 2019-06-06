/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:   
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and 
 * international Copyright laws.  Users and possessors of this source code 
 * are hereby granted a nonexclusive, royalty-free license to use this code 
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOURCE CODE.  
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

#if !defined(__CUDA_GL_INTEROP_H__)
#define __CUDA_GL_INTEROP_H__

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "builtin_types.h"
#include "host_defines.h"

#if defined(__APPLE__)

#include <OpenGL/gl.h>

#else /* __APPLE__ */

#include <GL/gl.h>

#endif /* __APPLE__ */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern __host__ cudaError_t CUDARTAPI cudaGLSetGLDevice(int device);
extern __host__ cudaError_t CUDARTAPI cudaGraphicsGLRegisterImage(struct cudaGraphicsResource **resource, GLuint image, GLenum target, unsigned int Flags);
extern __host__ cudaError_t CUDARTAPI cudaGraphicsGLRegisterBuffer(struct cudaGraphicsResource **resource, GLuint buffer, unsigned int Flags);

#ifdef _WIN32
#ifndef WGL_NV_gpu_affinity
typedef void* HGPUNV;
#endif
extern __host__ cudaError_t CUDARTAPI cudaWGLGetDevice(int *device, HGPUNV hGpu);
#endif

/**
 * CUDA GL Map Flags
 */
enum cudaGLMapFlags
{
  cudaGLMapFlagsNone         = 0,  ///< Default; Assume resource can be read/written
  cudaGLMapFlagsReadOnly     = 1,  ///< CUDA kernels will not write to this resource
  cudaGLMapFlagsWriteDiscard = 2,  ///< CUDA kernels will only write to and will not read from this resource
};

extern __host__ cudaError_t CUDARTAPI cudaGLRegisterBufferObject(GLuint bufObj);
extern __host__ cudaError_t CUDARTAPI cudaGLMapBufferObject(void **devPtr, GLuint bufObj);
extern __host__ cudaError_t CUDARTAPI cudaGLUnmapBufferObject(GLuint bufObj);
extern __host__ cudaError_t CUDARTAPI cudaGLUnregisterBufferObject(GLuint bufObj);

extern __host__ cudaError_t CUDARTAPI cudaGLSetBufferObjectMapFlags(GLuint bufObj, unsigned int flags); 
extern __host__ cudaError_t CUDARTAPI cudaGLMapBufferObjectAsync(void **devPtr, GLuint bufObj, cudaStream_t stream);
extern __host__ cudaError_t CUDARTAPI cudaGLUnmapBufferObjectAsync(GLuint bufObj, cudaStream_t stream);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* __CUDA_GL_INTEROP_H__ */
