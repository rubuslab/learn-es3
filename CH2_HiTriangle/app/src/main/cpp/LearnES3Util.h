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

/// esCreateWindow flag - RGB color buffer
#define ES_WINDOW_RGB           0
/// esCreateWindow flag - ALPHA color buffer
#define ES_WINDOW_ALPHA         1
/// esCreateWindow flag - depth buffer
#define ES_WINDOW_DEPTH         2
/// esCreateWindow flag - stencil buffer
#define ES_WINDOW_STENCIL       4
/// esCreateWindow flat - multi-sample buffer
#define ES_WINDOW_MULTISAMPLE   8

typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

typedef struct
{
    GLfloat   m[4][4];
} ESMatrix;

typedef struct ESContext ESContext;

struct ESContext
{
    /// Put platform specific data here
    void       *platformData;

    /// Put your user data here...
    void       *userData;

    /// Window width
    GLint       width;

    /// Window height
    GLint       height;

#ifndef __APPLE__
    /// Display handle
    EGLNativeDisplayType eglNativeDisplay;

    /// Window handle
    EGLNativeWindowType  eglNativeWindow;

    /// EGL display
    EGLDisplay  eglDisplay;

    /// EGL context
    EGLContext  eglContext;

    /// EGL surface
    EGLSurface  eglSurface;
#endif

    /// Callbacks
    void ( ESCALLBACK *drawFunc ) ( ESContext * );
    void ( ESCALLBACK *shutdownFunc ) ( ESContext * );
    void ( ESCALLBACK *keyFunc ) ( ESContext *, unsigned char, int, int );
    void ( ESCALLBACK *updateFunc ) ( ESContext *, float deltaTime );
};

//
/// \brief Log a message to the debug output for the platform
/// \param formatStr Format string for error log.
//
void ESUTIL_API esLogMessage ( const char *formatStr, ... ) {
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
GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
   {
      return 0;
   }

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );

   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
         char* infoLog = new char[sizeof(char) * infoLen];

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         esLogMessage ( "Error compiling shader:\n%s\n", infoLog );

         delete []infoLog;
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}

///
// Initialize the shader and program object
//
/*
int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   char vShaderStr[] =
      "#version 300 es                          \n"
      "layout(location = 0) in vec4 vPosition;  \n"
      "void main()                              \n"
      "{                                        \n"
      "   gl_Position = vPosition;              \n"
      "}                                        \n";

   char fShaderStr[] =
      "#version 300 es                              \n"
      "precision mediump float;                     \n"
      "out vec4 fragColor;                          \n"
      "void main()                                  \n"
      "{                                            \n"
      "   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
      "}                                            \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

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
         char *infoLog = malloc ( sizeof ( char ) * infoLen );

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         esLogMessage ( "Error linking program:\n%s\n", infoLog );

         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return FALSE;
   }

   // Store the program object
   userData->programObject = programObject;

   glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
   return TRUE;
}
*/

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(GLuint program_object, GLsizei width, GLsizei height)
{
   GLfloat vVertices[] = {  0.0f,  0.5f, 0.0f,
                            -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f, 0.0f
                         };

   // Set the viewport
   glViewport ( 0, 0, width, height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram(program_object);

   // Load the vertex data
   glVertexAttribPointer ( 0, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
   glEnableVertexAttribArray ( 0 );

   glDrawArrays ( GL_TRIANGLES, 0, 3 );
}

void Shutdown(GLuint program_object) {
   glDeleteProgram(program_object);
}

/*
int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "Hello Triangle", 320, 240, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc ( esContext, Shutdown );
   esRegisterDrawFunc ( esContext, Draw );

   return GL_TRUE;
}
*/