#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

struct android_app;

class MRTRender {
public:
    MRTRender() {
        UserData_.programObject = 0;
    }
    virtual ~MRTRender() {
        ShutDown();
    }

    bool Init();
    void Draw(GLsizei width, GLsizei height) const;

private:
    int InitFBO();

    void DrawGeometry(GLsizei width, GLsizei height) const;
    void BlitTextures(GLsizei width, GLsizei height) const;
    void ShutDown();

    struct RenderUserData {
        // Handle to a program object
        GLuint programObject;

        // Handle to a framebuffer object
        GLuint fbo;

        // Texture handle
        GLuint colorTexId[4];

        // Texture size
        GLsizei textureWidth;
        GLsizei textureHeight;
    }UserData_;
};


class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true),
            cubemap_render_(nullptr) {
        initRenderer();
    }

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Renders all the models in the renderer
     */
    void render();

private:
    /*!
     * Performs necessary OpenGL initialization. Customize this if you want to change your EGL
     * context or application-wide settings.
     */
    void initRenderer();

    /*!
     * @brief we have to check every frame to see if the framebuffer has changed in size. If it has,
     * update the viewport accordingly
     */
    void updateRenderArea();

    android_app* app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    MRTRender* cubemap_render_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H