// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Book:      OpenGL(R) ES 3.0 Programming Guide, 2nd Edition
// Authors:   Dan Ginsburg, Budirijanto Purnomo, Dave Shreiner, Aaftab Munshi
// ISBN-10:   0-321-93388-5
// ISBN-13:   978-0-321-93388-1
// Publisher: Addison-Wesley Professional
// URLs:      http://www.opengles-book.com
//            http://my.safaribooksonline.com/book/animation-and-3d/9780133440133
//
// Hello_Triangle.c
//
//    This is a simple example that draws a single triangle with
//    a minimal vertex/fragment shader.  The purpose of this
//    example is to demonstrate the basic concepts of
//    OpenGL ES 3.0 rendering.

// #include "esUtil.h"

///
// Includes
//
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
// #include <android_native_app_glue.h>
#include <time.h>

///
//  Macros
//
#ifdef WIN32
#define ESUTIL_API  __cdecl
#define ESCALLBACK  __cdecl
#else
#define ESUTIL_API
#define ESCALLBACK
#endif

///
// Types
//
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

///
// Defines
//
#define ES_PI  (3.14159265f)

//
/// \brief Log a message to the debug output for the platform
/// \param formatStr Format string for error log.
//
void ESUTIL_API esLogMessage(const char* formatStr, ...) {
   va_list params;
   char buf[BUFSIZ] = {0};

   va_start ( params, formatStr );
   vsprintf ( buf, formatStr, params );

#ifdef ANDROID
   __android_log_print ( ANDROID_LOG_INFO, "esUtil" , "%s", buf );
#else
   printf ( "%s", buf );
#endif

   va_end ( params );
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint esLoadShader(GLenum type, const char *shaderSrc) {
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader(type);

   if (shader == 0) {
      return 0;
   }

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );

   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if (!compiled) {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
      if (infoLen > 1) {
         char* infoLog = new char[sizeof(char) * infoLen];

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         esLogMessage ( "Error compiling shader:\n%s\n", infoLog );

         delete []infoLog;
      }

      glDeleteShader(shader);
      return 0;
   }

   return shader;
}

//
///
/// \brief Load a vertex and fragment shader, create a program object, link program.
//         Errors output to log.
/// \param vertShaderSrc Vertex shader source code
/// \param fragShaderSrc Fragment shader source code
/// \return A new program object linked with the vertex/fragment shader pair, 0 on failure
//
GLuint ESUTIL_API esLoadProgram ( const char *vertShaderSrc, const char *fragShaderSrc )
{
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = esLoadShader ( GL_VERTEX_SHADER, vertShaderSrc );

   if ( vertexShader == 0 )
   {
      return 0;
   }

   fragmentShader = esLoadShader ( GL_FRAGMENT_SHADER, fragShaderSrc );

   if ( fragmentShader == 0 )
   {
      glDeleteShader ( vertexShader );
      return 0;
   }

   // Create the program object
   programObject = glCreateProgram ( );

   if ( programObject == 0 )
   {
      return 0;
   }

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = (char*)malloc(sizeof(char) * infoLen);

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         esLogMessage ( "Error linking program:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return 0;
   }

   // Free up no longer needed shader resources
   glDeleteShader ( vertexShader );
   glDeleteShader ( fragmentShader );

   return programObject;
}

//
/// \brief Generates geometry for a sphere.  Allocates memory for the vertex data and stores
///        the results in the arrays.  Generate index list for a TRIANGLE_STRIP
/// \param numSlices The number of slices in the sphere
/// \param vertices If not NULL, will contain array of float3 positions
/// \param normals If not NULL, will contain array of float3 normals
/// \param texCoords If not NULL, will contain array of float2 texCoords
/// \param indices If not NULL, will contain the array of indices for the triangle strip
/// \return The number of indices required for rendering the buffers (the number of indices stored in the indices array
///         if it is not NULL ) as a GL_TRIANGLE_STRIP
//
int ESUTIL_API esGenSphere(int numSlices, float radius, GLfloat **vertices, GLfloat **normals,
                           GLfloat **texCoords, GLuint **indices)
{
   int i;
   int j;
   int numParallels = numSlices / 2;
   int numVertices = ( numParallels + 1 ) * ( numSlices + 1 );
   int numIndices = numParallels * numSlices * 6;
   float angleStep = ( 2.0f * ES_PI ) / ( ( float ) numSlices );

   // Allocate memory for buffers
   if (vertices != NULL) {
      *vertices = (GLfloat*)malloc(sizeof(GLfloat) * 3 * numVertices);
   }

   if (normals != NULL) {
      *normals = (GLfloat*)malloc(sizeof(GLfloat) * 3 * numVertices);
   }

   if (texCoords != NULL) {
      *texCoords = (GLfloat*)malloc(sizeof(GLfloat) * 2 * numVertices);
   }

   if (indices != NULL) {
      *indices = (GLuint*)malloc(sizeof(GLuint) * numIndices);
   }

   for ( i = 0; i < numParallels + 1; i++ )
   {
      for ( j = 0; j < numSlices + 1; j++ )
      {
         int vertex = ( i * ( numSlices + 1 ) + j ) * 3;

         if ( vertices )
         {
            ( *vertices ) [vertex + 0] = radius * sinf ( angleStep * ( float ) i ) *
                                         sinf ( angleStep * ( float ) j );
            ( *vertices ) [vertex + 1] = radius * cosf ( angleStep * ( float ) i );
            ( *vertices ) [vertex + 2] = radius * sinf ( angleStep * ( float ) i ) *
                                         cosf ( angleStep * ( float ) j );
         }

         if ( normals )
         {
            ( *normals ) [vertex + 0] = ( *vertices ) [vertex + 0] / radius;
            ( *normals ) [vertex + 1] = ( *vertices ) [vertex + 1] / radius;
            ( *normals ) [vertex + 2] = ( *vertices ) [vertex + 2] / radius;
         }

         if ( texCoords )
         {
            int texIndex = ( i * ( numSlices + 1 ) + j ) * 2;
            ( *texCoords ) [texIndex + 0] = ( float ) j / ( float ) numSlices;
            ( *texCoords ) [texIndex + 1] = ( 1.0f - ( float ) i ) / ( float ) ( numParallels - 1 );
         }
      }
   }

   // Generate the indices
   if ( indices != NULL )
   {
      GLuint *indexBuf = ( *indices );

      for ( i = 0; i < numParallels ; i++ )
      {
         for ( j = 0; j < numSlices; j++ )
         {
            *indexBuf++  = i * ( numSlices + 1 ) + j;
            *indexBuf++ = ( i + 1 ) * ( numSlices + 1 ) + j;
            *indexBuf++ = ( i + 1 ) * ( numSlices + 1 ) + ( j + 1 );

            *indexBuf++ = i * ( numSlices + 1 ) + j;
            *indexBuf++ = ( i + 1 ) * ( numSlices + 1 ) + ( j + 1 );
            *indexBuf++ = i * ( numSlices + 1 ) + ( j + 1 );
         }
      }
   }

   return numIndices;
}

