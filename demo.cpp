#include "csg.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "flythrough_camera.h"
#include "shader_program.hpp"
#include "defer.hpp"
#include <randomcolor.h>

using namespace csg;
using namespace glm;
using namespace std;

static RandomColor random_color;

static float signed_distance(const vec3& point, const plane_t& plane) {
    return dot(plane.normal, point) + plane.offset;
}

static vec3 project_point(const vec3& point,
                          const plane_t& plane)
{
    return point - signed_distance(point, plane) * plane.normal;
}

static plane_t make_plane(
    const vec3& point,
    const vec3& normal)
{
    return plane_t{ normal, -dot(point, normal) };
}

static plane_t transformed(
    const plane_t& plane,
    const mat4& transform)
{
    vec3 origin_transformed = vec3(
        transform * vec4(project_point(vec3(0,0,0), plane), 1.0f)
    );
    vec3 normal_transformed = glm::normalize(vec3(
        transpose(inverse(transform)) * vec4(plane.normal, 0.0f)
    ));    
    plane_t transformed_plane =  make_plane(origin_transformed, normal_transformed);
    return transformed_plane;
}

static constexpr volume_t AIR = 0;
static constexpr volume_t SOLID = 1;

enum brush_type_t {
    AIR_BRUSH,
    SOLID_BRUSH
};

struct cube_brush_userdata_t {
    void set_brush_type(brush_type_t type){ 
        this->type = type;
        switch (type) {
            case AIR_BRUSH: brush->set_volume_operation(make_fill_operation(AIR)); break;
            case SOLID_BRUSH: brush->set_volume_operation(make_fill_operation(SOLID)); break;
            default: ;
        }
    }

    brush_type_t get_brush_type() const {
        return type;
    }

    void set_transform(const mat4& transform) {
        static vector<plane_t> cube_planes = {
            make_plane(vec3(+1,0,0), vec3(+1,0,0)),
            make_plane(vec3(-1,0,0), vec3(-1,0,0)),
            make_plane(vec3(0,+1,0), vec3(0,+1,0)),
            make_plane(vec3(0,-1,0), vec3(0,-1,0)),
            make_plane(vec3(0,0,+1), vec3(0,0,+1)),
            make_plane(vec3(0,0,-1), vec3(0,0,-1))
        };

        this->transform = transform;

        vector<plane_t> transformed_planes ;
        for (const plane_t& plane: cube_planes)
            transformed_planes.push_back( transformed(plane, transform) );

        brush->set_planes(transformed_planes);
    }

    const mat4& get_transform() const {
        return transform;
    }

    void update_display_list() {
        glNewList(display_list, GL_COMPILE);

        auto faces = brush->get_faces();
        for (face_t& face: faces) {
            glBegin(GL_TRIANGLES);
            for (fragment_t& fragment: face.fragments) {
                if (fragment.back_volume == fragment.front_volume) {
                    continue;
                }

                const vector<vertex_t>& vertices = fragment.vertices;

                vec3 normal = face.plane->normal;
                bool flip_face = fragment.back_volume == AIR;
                if (flip_face) {
                    normal = -normal;
                }

                glNormal3fv(value_ptr(normal));

                vector<triangle_t> tris = triangulate(fragment);
                for (triangle_t& tri: tris) {
                    if (flip_face) {
                        glVertex3fv(value_ptr(vertices[tri.i].position));
                        glVertex3fv(value_ptr(vertices[tri.k].position));
                        glVertex3fv(value_ptr(vertices[tri.j].position));
                    } else {
                        glVertex3fv(value_ptr(vertices[tri.i].position));
                        glVertex3fv(value_ptr(vertices[tri.j].position));
                        glVertex3fv(value_ptr(vertices[tri.k].position));
                    }
                }
            }
            glEnd();
        }
        glEndList();
    }

    void render_fill() {
        glColor3ub(red, green, blue);
        glCallList(display_list);
    }

    void render_wire() {
        glCallList(display_list);
    }

    cube_brush_userdata_t(brush_t* brush): brush(brush) {
        display_list = glGenLists(1);

        int argb = random_color.generate();
        blue = (argb >> 0) & 0xff;
        green = (argb >> 8) & 0xff;
        red = (argb >> 16) & 0xff;        
    }

private:    
    mat4 transform;
    char red,green,blue;
    brush_t *brush;
    brush_type_t type;
    GLuint display_list;
};

void render_world(world_t& world, const mat4& viewproj, ShaderProgram* shader) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glColor3f(1,1,1);

        // frustum culling    
        vector<brush_t*> visible = world.query_frustum(viewproj);

        // draw the fragments
        // ----------------------------
        shader->use();
        for (brush_t* brush: visible) {
            any_cast<cube_brush_userdata_t>(&brush->userdata)->render_fill();
        }
        glUseProgram(0);

        // draw the wireframe
        // ---------------------------
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glColor3f(1,1,1);

        for (brush_t* brush: visible) {
            any_cast<cube_brush_userdata_t>(&brush->userdata)->render_wire();
        }

        glPolygonOffset(0.0f, 0.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
    
    glDisable(GL_CULL_FACE);
}

// simple shader with headlight
// ---------------------------------------
const char *solid_vert = R"(
    #version 120

    varying vec3 v_vertex_viewspace;
    varying vec3 v_color;
    varying vec3 v_normal_viewspace;

    void main() {
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        v_vertex_viewspace = (gl_ModelViewMatrix * gl_Vertex).xyz;
        v_color = gl_Color.rgb;
        v_normal_viewspace = (gl_ModelViewMatrix * vec4(gl_Normal,0)).xyz;
    }
)";
const char *solid_frag = R"(
    #version 120

    varying vec3 v_vertex_viewspace;
    varying vec3 v_color;
    varying vec3 v_normal_viewspace;

    void main() {
        vec3 to_camera = -normalize(v_vertex_viewspace);
        float distance = length(v_vertex_viewspace);
        float density = 0.1;
        float lambert = dot(normalize(v_normal_viewspace), to_camera);

        gl_FragColor.rgb = v_color * lambert * (1/exp(distance*density));
        gl_FragColor.a = 1;
    }
)";
// ---------------------------------------

int main(int, char**) {
    SDL_Init(SDL_INIT_EVERYTHING);
    DEFER( SDL_Quit() );

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    SDL_Window *window = SDL_CreateWindow(
        "tiny_csg demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );
    DEFER( SDL_DestroyWindow(window) );

    SDL_SetRelativeMouseMode(SDL_TRUE);

    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    DEFER( SDL_GL_DeleteContext(context) );

    // vsync
    if (SDL_GL_SetSwapInterval(-1) != 0 )
        SDL_GL_SetSwapInterval(1);

    glewInit();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // init the shader
    ShaderProgram *solid_shader = 0;
    {
        Shader *vert = new Shader(solid_vert, strlen(solid_vert), GL_VERTEX_SHADER);
        DEFER( delete vert );
        Shader *frag = new Shader(solid_frag, strlen(solid_frag), GL_FRAGMENT_SHADER);
        DEFER( delete frag );
        vert->compile();
        frag->compile();
        Shader *shaders[] = {vert, frag};
        solid_shader = new ShaderProgram(shaders, 2);
    }
    DEFER( delete solid_shader );

    /* 
        init the camera
    */
    vec3 eye(0,0,0);
    vec3 look(0,0,-1);
    vec3 up(0,1,0);
    mat4 view(1);

    /*
        build the world
    */
    world_t world;
    world.set_void_volume(SOLID);
    
    brush_t *room = world.add(); 
    {
        room->userdata = cube_brush_userdata_t(room);
        cube_brush_userdata_t *userdata = any_cast<cube_brush_userdata_t>(&room->userdata);
        userdata->set_brush_type(AIR_BRUSH);
        userdata->set_transform(mat4(1));
    }

    brush_t *pillar = world.add();
    {
        pillar->userdata = cube_brush_userdata_t(pillar);
        cube_brush_userdata_t *userdata = any_cast<cube_brush_userdata_t>(&pillar->userdata);
        userdata->set_brush_type(SOLID_BRUSH);
        userdata->set_transform(
            scale(mat4(1), vec3(0.25f, 2.0f, 0.25f))
        );
    }

    brush_t *tunnel = world.add();
    {
        tunnel->userdata = cube_brush_userdata_t(tunnel);
        cube_brush_userdata_t *userdata = any_cast<cube_brush_userdata_t>(&tunnel->userdata);
        userdata->set_brush_type(AIR_BRUSH);
        userdata->set_transform(
            scale(mat4(1), vec3(2.0f, 0.25f, 0.25f))
        );    
    }

    /*
        main loop
    */
    bool done = false;
    int ticks_now = SDL_GetTicks();
    int ticks_last = 0;
    float delta_time = 0;
    while (!done) {
        ticks_last = ticks_now;
        ticks_now = SDL_GetTicks();
        delta_time = (ticks_now - ticks_last) / 1000.0f;

        ivec2 mouse_delta(0,0);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    done = true;
                    break;
                case SDL_MOUSEMOTION:
                    mouse_delta = ivec2(ev.motion.xrel, ev.motion.yrel);
                    break;
            }
        }

        // rotate the tunnel brush
        // ---------------------------------------
        static cube_brush_userdata_t*
            tunnel_userdata = any_cast<cube_brush_userdata_t>(&tunnel->userdata);

        static const mat4
            tunnel_original_transform = tunnel_userdata->get_transform();        

        float seconds_now = ticks_now / 1000.0f;

        tunnel_userdata->set_transform(
            glm::rotate(mat4(1), radians(seconds_now*45.0f), vec3(0,1,0)) *
            tunnel_original_transform 
        );
        // ---------------------------------------

        // rebuild the world and update display list for any rebuilt brush
        // ---------------------------------------
        auto rebuilt = world.rebuild();   
        for (brush_t* b: rebuilt)
            any_cast<cube_brush_userdata_t>(&b->userdata)->update_display_list();        
        // ---------------------------------------

        // update camera
        // ---------------------------------------
        auto keys_down = SDL_GetKeyboardState(NULL);
        flythrough_camera_update(
            value_ptr(eye), value_ptr(look), value_ptr(up), value_ptr(view),
            delta_time, 4.0f, 0.4f, 80.0f, 
            mouse_delta.x, mouse_delta.y,
            keys_down[SDL_SCANCODE_W],
            keys_down[SDL_SCANCODE_A],
            keys_down[SDL_SCANCODE_S],
            keys_down[SDL_SCANCODE_D],
            keys_down[SDL_SCANCODE_SPACE],
            keys_down[SDL_SCANCODE_C],
            0
        ) ;

        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        mat4 proj = perspective(
            radians(60.0f),
            float(w)/h,
            0.01f,
            20.0f
        );
        // ---------------------------------------

        // drawing
        // ---------------------------------------
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(value_ptr(view));
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(value_ptr(proj));

        mat4 viewproj = proj * view;

        glViewport(0,0,w,h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        render_world(world, viewproj, solid_shader);
        // ---------------------------------------

        // swap buffers
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
