#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <memory>

struct android_app;

class CubemapRender {
public:
    CubemapRender(): program_object_(0) {}
    virtual ~CubemapRender() {
        glDeleteProgram(program_object_);
        program_object_ = 0;
    }

    bool Init();
    void Draw(GLsizei width, GLsizei height) const;

private:
    ///
    // Create a simple cubemap with a 1x1 face with a different color for each face
    // return texture id
    GLuint CreateSimpleTextureCubemap();

    GLuint program_object_;
    struct RenderUserData {
        // Handle to a program object
        GLuint programObject;

        // Sampler location
        GLint samplerLoc;

        // Texture handle
        GLuint textureId;

        // Vertex data
        int      numIndices;
        GLfloat *vertices;
        GLfloat *normals;
        GLuint  *indices;
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

    CubemapRender* cubemap_render_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H