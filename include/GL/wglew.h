/*
** The OpenGL Extension Wrangler Library
** Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
** Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
** Copyright (C) 2002, Lev Povalahev
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice, 
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution.
** * The name of the author may be used to endorse or promote products 
**   derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** Copyright (c) 2007 The Khronos Group Inc.
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
** 
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef __wglew_h__
#define __wglew_h__
#define __WGLEW_H__

#ifdef __wglext_h_
#error wglext.h included before wglew.h
#endif

#define __wglext_h_

#if !defined(APIENTRY) && !defined(__CYGWIN__)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#include <windows.h>
#endif

/*
 * GLEW_STATIC needs to be set when using the static version.
 * GLEW_BUILD is set when building the DLL version.
 */
#ifdef GLEW_STATIC
#  define GLEWAPI extern
#else
#  ifdef GLEW_BUILD
#    define GLEWAPI extern __declspec(dllexport)
#  else
#    define GLEWAPI extern __declspec(dllimport)
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------- WGL_ARB_extensions_string ----------------------- */

#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1

typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);

#define wglGetExtensionsStringARB WGLEW_GET_FUN(__wglewGetExtensionsStringARB)

#define WGLEW_ARB_extensions_string WGLEW_GET_VAR(__WGLEW_ARB_extensions_string)

#endif /* WGL_ARB_extensions_string */

/* ----------------------- WGL_EXT_extensions_string ----------------------- */

#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string 1

typedef const char* (WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);

#define wglGetExtensionsStringEXT WGLEW_GET_FUN(__wglewGetExtensionsStringEXT)

#define WGLEW_EXT_extensions_string WGLEW_GET_VAR(__WGLEW_EXT_extensions_string)

#endif /* WGL_EXT_extensions_string */

/* ------------------------------------------------------------------------- */

#ifdef GLEW_MX
#define WGLEW_EXPORT
#else
#define WGLEW_EXPORT GLEWAPI
#endif /* GLEW_MX */

#ifdef GLEW_MX
struct WGLEWContextStruct
{
#endif /* GLEW_MX */

WGLEW_EXPORT PFNWGLGETEXTENSIONSSTRINGARBPROC __wglewGetExtensionsStringARB;

WGLEW_EXPORT PFNWGLGETEXTENSIONSSTRINGEXTPROC __wglewGetExtensionsStringEXT;
WGLEW_EXPORT GLboolean __WGLEW_ARB_extensions_string;
WGLEW_EXPORT GLboolean __WGLEW_EXT_extensions_string;

#ifdef GLEW_MX
}; /* WGLEWContextStruct */
#endif /* GLEW_MX */

/* ------------------------------------------------------------------------- */

#ifdef GLEW_MX

typedef struct WGLEWContextStruct WGLEWContext;
GLEWAPI GLenum wglewContextInit (WGLEWContext* ctx);
GLEWAPI GLboolean wglewContextIsSupported (WGLEWContext* ctx, const char* name);

#define wglewInit() wglewContextInit(wglewGetContext())
#define wglewIsSupported(x) wglewContextIsSupported(wglewGetContext(), x)

#define WGLEW_GET_VAR(x) (*(const GLboolean*)&(wglewGetContext()->x))
#define WGLEW_GET_FUN(x) wglewGetContext()->x

#else /* GLEW_MX */

#define WGLEW_GET_VAR(x) (*(const GLboolean*)&x)
#define WGLEW_GET_FUN(x) x

GLEWAPI GLboolean wglewIsSupported (const char* name);

#endif /* GLEW_MX */

GLEWAPI GLboolean wglewGetExtension (const char* name);

#ifdef __cplusplus
}
#endif

#undef GLEWAPI

#endif /* __wglew_h__ */
