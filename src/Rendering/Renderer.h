#ifndef H_WD_RENDERER
#define H_WD_RENDERER

class Scene; // Forward declaration 

namespace Wado::Rendering {

class Renderer {
    public:
        virtual void init() = 0;
        virtual void render(Scene scene) = 0;
    private:
}; 

};

#endif