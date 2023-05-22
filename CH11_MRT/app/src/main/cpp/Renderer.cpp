#include "Renderer.h"


#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <android/imagedecoder.h>


#include <cassert>
#include <memory>
#include <vector>

#include "AndroidOut.h"
#include "LearnES3Util.h"

//! executes glGetString and outputs the result to logcat
#define PRINT_GL_STRING(s) { aout << #s": " << glGetString(s) << std::endl; }

/*!
 * @brief if glGetString returns a space separated list of elements, prints each one on a new line
 *
 * This works by creating an istringstream of the input c-style string. Then that is used to create
 * a vector -- each element of the vector is a new element in the input string. Finally a foreach
 * loop consumes this and outputs it to logcat using @a aout
 */
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

//! Color for cornflower blue. Can be sent directly to glClearColor
#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1

///
// Initialize the framebuffer object and MRTs
//
int MRTRender::InitFBO() {
    RenderUserData* userData = &UserData_;
    int i;
    GLint defaultFramebuffer = 0;
    const GLenum attachments[4] =
            {
                    GL_COLOR_ATTACHMENT0,
                    GL_COLOR_ATTACHMENT1,
                    GL_COLOR_ATTACHMENT2,
                    GL_COLOR_ATTACHMENT3
            };

    glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );

    // Setup fbo
    glGenFramebuffers ( 1, &userData->fbo );
    glBindFramebuffer ( GL_FRAMEBUFFER, userData->fbo );

    // Setup four output buffers and attach to fbo
    userData->textureHeight = userData->textureWidth = 400;
    glGenTextures ( 4, &userData->colorTexId[0] );
    for (i = 0; i < 4; ++i)
    {
        glBindTexture ( GL_TEXTURE_2D, userData->colorTexId[i] );

        glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
                       userData->textureWidth, userData->textureHeight,
                       0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

        // Set the filtering mode
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

        glFramebufferTexture2D ( GL_DRAW_FRAMEBUFFER, attachments[i],
                                 GL_TEXTURE_2D, userData->colorTexId[i], 0 );
    }

    glDrawBuffers ( 4, attachments );

    if ( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus ( GL_FRAMEBUFFER ) )
    {
        return FALSE;
    }

    // Restore the original framebuffer
    glBindFramebuffer ( GL_FRAMEBUFFER, defaultFramebuffer );

    return TRUE;
}

// Initialize the shader and program object
bool MRTRender::Init() {
    RenderUserData* userData = &UserData_;
    char vShaderStr[] =
            "#version 300 es                            \n"
            "layout(location = 0) in vec4 a_position;   \n"
            "void main()                                \n"
            "{                                          \n"
            "   gl_Position = a_position;               \n"
            "}                                          \n";

    char fShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "layout(location = 0) out vec4 fragData0;            \n"
            "layout(location = 1) out vec4 fragData1;            \n"
            "layout(location = 2) out vec4 fragData2;            \n"
            "layout(location = 3) out vec4 fragData3;            \n"
            "void main()                                         \n"
            "{                                                   \n"
            "  // first buffer will contain red color            \n"
            "  fragData0 = vec4 ( 1, 0, 0, 1 );                  \n"
            "                                                    \n"
            "  // second buffer will contain green color         \n"
            "  fragData1 = vec4 ( 0, 1, 0, 1 );                  \n"
            "                                                    \n"
            "  // third buffer will contain blue color           \n"
            "  fragData2 = vec4 ( 0, 0, 1, 1 );                  \n"
            "                                                    \n"
            "  // fourth buffer will contain gray color          \n"
            "  fragData3 = vec4 ( 0.5, 0.5, 0.5, 1 );            \n"
            "}                                                   \n";

    // Load the shaders and get a linked program object
    userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

    InitFBO();

    glClearColor ( 1.0f, 1.0f, 1.0f, 0.0f );
    return TRUE;
};

void MRTRender::DrawGeometry(GLsizei width, GLsizei height) const {
    const RenderUserData *userData = &UserData_;
    GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,
                            -1.0f, -1.0f, 0.0f,
                            1.0f, -1.0f, 0.0f,
                            1.0f,  1.0f, 0.0f,
    };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    // Set the viewport
    glViewport ( 0, 0, width, height);

    // Clear the color buffer
    glClear ( GL_COLOR_BUFFER_BIT );

    // Use the program object
    glUseProgram ( userData->programObject );

    // Load the vertex position
    glVertexAttribPointer ( 0, 3, GL_FLOAT,
                            GL_FALSE, 3 * sizeof ( GLfloat ), vVertices );
    glEnableVertexAttribArray ( 0 );

    // Draw a quad
    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
}

void MRTRender::BlitTextures(GLsizei width, GLsizei height) const {
    const RenderUserData* userData = &UserData_;

    // set the fbo for reading
    glBindFramebuffer ( GL_READ_FRAMEBUFFER, userData->fbo );

    // Copy the output red buffer to lower left quadrant
    glReadBuffer ( GL_COLOR_ATTACHMENT0 );
    glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                        0, 0, width/2, height/2,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR );

    // Copy the output green buffer to lower right quadrant
    glReadBuffer ( GL_COLOR_ATTACHMENT1 );
    glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                        width/2, 0, width, height/2,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR );

    // Copy the output blue buffer to upper left quadrant
    glReadBuffer ( GL_COLOR_ATTACHMENT2 );
    glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                        0, height/2, width/2, height,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR );

    // Copy the output gray buffer to upper right quadrant
    glReadBuffer ( GL_COLOR_ATTACHMENT3 );
    glBlitFramebuffer ( 0, 0, userData->textureWidth, userData->textureHeight,
                        width/2, height/2, width, height,
                        GL_COLOR_BUFFER_BIT, GL_LINEAR );
}

void MRTRender::ShutDown() {
    RenderUserData* userData = &UserData_;

    // Delete texture objects
    glDeleteTextures ( 4, userData->colorTexId );

    // Delete fbo
    glDeleteFramebuffers ( 1, &userData->fbo);

    // Delete program object
    glDeleteProgram ( userData->programObject );
}

///
// Draw a triangle using the shader pair created in Init()
//
void MRTRender::Draw(GLsizei width, GLsizei height) const {
    const RenderUserData* userData = &UserData_;
    GLint defaultFramebuffer = 0;
    const GLenum attachments[4] =
            {
                    GL_COLOR_ATTACHMENT0,
                    GL_COLOR_ATTACHMENT1,
                    GL_COLOR_ATTACHMENT2,
                    GL_COLOR_ATTACHMENT3
            };

    // 不论是直接渲染到屏幕还是进行离屏渲染，都需要创建震缓冲区对象即FBO，
    // 只不过直接渲染到屏幕的FBO的GL_FRAMEBUFFER_BINDING为0。渲染到其他存储空间的frambuffer的id大于0.
    glGetIntegerv ( GL_FRAMEBUFFER_BINDING, &defaultFramebuffer );

    // FIRST: use MRTs to output four colors to four buffers
    glBindFramebuffer ( GL_FRAMEBUFFER, userData->fbo );
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDrawBuffers ( 4, attachments );
    DrawGeometry(width, height);

    // SECOND: copy the four output buffers into four window quadrants
    // with framebuffer blits

    // Restore the default framebuffer, prepare to blit to default frame buffer
    glBindFramebuffer ( GL_DRAW_FRAMEBUFFER, defaultFramebuffer );
    BlitTextures(width, height);
}

// ====================================================================================================================

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }

    delete cubemap_render_;
    cubemap_render_ = nullptr;
}

void Renderer::render() {
    // Check to see if the surface has changed size. This is _necessary_ to do every frame when
    // using immersive mode as you'll get no other notification that your renderable area has
    // changed.
    updateRenderArea();

    // When the renderable area changes, the projection matrix has to also be updated. This is true
    // even if you change from the sample orthographic projection matrix as your aspect ratio has
    // likely changed.
    if (shaderNeedsNewProjectionMatrix_) {
        shaderNeedsNewProjectionMatrix_ = false;
    }

    // clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Render all the models. There's no depth testing in this sample so they're accepted in the
    // order provided. But the sample EGL setup requests a 24 bit depth buffer so you could
    // configure it at the end of initRenderer
    cubemap_render_->Draw(width_, height_);

    // Present the rendered image. This is an implicit glFlush.
    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    // Choose your render attributes
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs = 0;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;
    aout << "Chose " << config << std::endl;

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    // Create a GLES 3 context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    // make width and height invalid so it gets updated the first frame in @a updateRenderArea()
    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    // setup any other gl related global states
    glClearColor(CORNFLOWER_BLUE);

    // enable alpha globally for now, you probably don't want to do this in a game
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cubemap_render_ = new MRTRender();
    cubemap_render_->Init();
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);

        // make sure that we lazily recreate the projection matrix before we render
        shaderNeedsNewProjectionMatrix_ = true;
    }
}

void Renderer::handleInput() {
    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        // no inputs yet.
        return;
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        aout << "Pointer(s): ";

        // get the x and y position of this event if it is not ACTION_MOVE.
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Down";
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
                // treat the CANCEL as an UP event: doing nothing in the app, except
                // removing the pointer from the cache if pointers are locally saved.
                // code pass through on purpose.
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Up";
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    aout << "(" << pointer.id << ", " << x << ", " << y << ")";

                    if (index != (motionEvent.pointerCount - 1)) aout << ",";
                    aout << " ";
                }
                aout << "Pointer Move";
                break;
            default:
                aout << "Unknown MotionEvent Action: " << action;
        }
        aout << std::endl;
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        aout << "Key: " << keyEvent.keyCode <<" ";
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                aout << "Key Down";
                break;
            case AKEY_EVENT_ACTION_UP:
                aout << "Key Up";
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                // Deprecated since Android API level 29.
                aout << "Multiple Key Actions";
                break;
            default:
                aout << "Unknown KeyEvent Action: " << keyEvent.action;
        }
        aout << std::endl;
    }
    // clear the key input count too.
    android_app_clear_key_events(inputBuffer);
}