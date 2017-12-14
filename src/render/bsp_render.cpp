#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <game/config.h>
#include <game/render/bsp_render.h>
#include <game/sys/glsl.h>
#include <game/sys/quake3_bsp.h>

using namespace game::render;

static Quake3Bsp qbsp;

// This will identify our vertex buffer
static GLuint vertexbuffer;
static GLuint elementbuffer;
static GLuint uvbuffer;
static GLuint normalbuffer;

static GLuint matrix_id;
static GLuint texture_id;

bool BspRender::is_collided() {
    return qbsp.collided();
}

glm::vec3 BspRender::trace_box(glm::vec3 start, glm::vec3 end) {
    return qbsp.trace_box(start, end, glm::vec3(-15, -40, -15), glm::vec3(15, 40, 15));
}

bool BspRender::is_on_ground() {
    return qbsp.is_on_ground();
}

void BspRender::prepare() {
    auto str = Config::data_path() + std::string("maps/") + _map_name + ".bsp";
    auto done = qbsp.load_bsp(str);
    _logger->debug("done {}", done);


    //GLuint VertexArrayID;
    //glGenVertexArrays(1, &VertexArrayID);
    //glBindVertexArray(VertexArrayID);
    /*
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
    */
    //glGenBuffers(1, &elementbuffer);
    //glGenBuffers(1, &vertexbuffer);
    //glGenBuffers(1, &uvbuffer);
    //glGenBuffers(1, &normalbuffer);

    //matrix_id = glGetUniformLocation(sys::GLSL::program_id(), "MVP");
    //texture_id = glGetUniformLocation(sys::GLSL::program_id(), "texture_sampler");
}


BspRender::BspRender(std::string map_name) {
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
    _map_name = map_name;
    prepare();
}

void BspRender::render(Viewport &view) {
    // Model matrix : an identity matrix (model will be at the origin)
    //qbsp.render(view.camera()->view(), view.camera()->position());
    qbsp.render(view.camera()->position());

    // Our ModelViewProjection : multiplication of our 3 matrices
    //glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

    // Get a handle for our "MVP" uniform
    // Only during the initialisation

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // This is done in the main loop since each model will have a different MVP matrix
    // (At least for the M part)
    //glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

    /*

    //auto pos = view.camera()->position();
    auto pos = glm::vec3(80.000000,320.000000,55.000000);
    int leafIndex = qbsp.FindLeaf(pos);
    //_logger->debug("index: {}", leafIndex);

    // Grab the cluster that is assigned to the leaf
    int cluster = qbsp.leaf(leafIndex)->cluster;

    // Initialize our counter variables (start at the last leaf and work down)
    int i = qbsp.leaf_count();

    // Go through all the leafs and check their visibility
    while(i--) {
        // Get the current leaf that is to be tested for visibility from our camera's leaf
        tBSPLeaf *pLeaf = qbsp.leaf(i);

        // If the current leaf can't be seen from our cluster, go to the next leaf
        if(!qbsp.IsClusterVisible(cluster, pLeaf->cluster))
            continue;

        // If the current leaf is not in the camera's frustum, go to the next leaf
        //if(!g_Frustum.BoxInFrustum((float)pLeaf->min.x, (float)pLeaf->min.y, (float)pLeaf->min.z,
                                      //(float)pLeaf->max.x, (float)pLeaf->max.y, (float)pLeaf->max.z))
            //continue;

        // If we get here, the leaf we are testing must be visible in our camera's view.
        // Get the number of faces that this leaf is in charge of.
        int faceCount = pLeaf->numOfLeafFaces;

        // Loop through and render all of the faces in this leaf
        while(faceCount--) {
            // Grab the current face index from our leaf faces array
            int faceIndex = qbsp.leaf_face_index(pLeaf, faceCount);

            tBSPFace *pFace = qbsp.face(faceIndex);

            // Before drawing this face, make sure it's a normal polygon
            if(pFace->type != FACE_POLYGON) continue;

            // Since many faces are duplicated in other leafs, we need to
            // make sure this face already hasn't been drawn.
            if(!qbsp.is_face_drawn(faceIndex)) {
                qbsp.face_drawn(faceIndex);

                qbsp.RenderFace(faceIndex, elementbuffer, vertexbuffer, uvbuffer, normalbuffer);
            }
        }
    }
    */
}
