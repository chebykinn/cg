#pragma once

#include <vector>
#include <cstring>
#include <cstdint>

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>

#define FACE_POLYGON    1
#define FACE_PATCH      2

#define TYPE_RAY        0 // This is the type for tracing a RAY
#define TYPE_SPHERE     1 // This is the type for tracing a SPHERE
#define TYPE_BOX        2 // This is the type for tracing a AABB (BOX)

#define MAX_TEXTURES 1000

#define M_EPS 0.03125f

// BSP header structure
struct BSPHeader {
    char str_id[4];    // This should always be 'IBSP'
    int version;      // This should be 0x2e for Quake 3 files
};


// BSP lump structure
struct BSPLump {
    int offset;       // The offset into the file for the start of this lump
    int length;       // The length in bytes for this lump
};


// BSP vertex structure
struct BSPVertex {
    glm::vec3 position;       // (x, y, z) position.
    glm::vec2 texture_coord;   // (u, v) texture coordinate
    glm::vec2 lightmap_coord;  // (u, v) lightmap coordinate
    glm::vec3 normal;         // (x, y, z) normal vector
    uint8_t color[4];         // RGBA color for the vertex
};


// BSP face structure
struct BSPFace {
    int texture_id;            // The index into the texture array
    int effect;               // The index for the effects (or -1 = n/a)
    int type;                 // 1=polygon, 2=patch, 3=mesh, 4=billboard
    int start_vert_index;       // The starting index into this face's first vertex
    int verts_num;           // The number of vertices for this face
    int start_index;           // The starting index into the indices array for this face
    int indices_num;         // The number of indices for this face
    int lightmap_id;           // The texture index for the lightmap
    int lightmap_corner[2];        // The face's lightmap corner in the image
    int lightmap_size[2];          // The size of the lightmap section
    glm::vec3 lightmap_pos;         // The 3D origin of lightmap.
    glm::vec3 lightmap_vecs[2];     // The 3D space for s and t unit vectors.
    glm::vec3 normal;         // The face normal.
    int size[2];              // The bezier patch dimensions.
};


// BSP texture structure
struct BSPTexture {
    char name[64];         // The name of the texture w/o the extension
    int flags;                // The surface flags (unknown)
    int texture_type;          // The type of texture (solid, water, slime, etc..) (type & 1) = 1 (solid)
};

// BSP lightmap structure which stores the 128x128 RGB values
struct BSPLightmap {
    uint8_t imageBits[128][128][3];   // The RGB data in a 128x128 image
};

// node in the BSP tree
struct BSPNode {
    int plane;                // The index into the planes array
    int front;                // The child index for the front node
    int back;                 // The child index for the back node
    glm::tvec3<int32_t> min;            // The bounding box min position.
    glm::tvec3<int32_t> max;            // The bounding box max position.
};

// leaf (end node) in the BSP tree
struct BSPLeaf {
    int cluster;              // The visibility cluster
    int area;                 // The area portal
    glm::tvec3<int32_t> min;            // The bounding box min position
    glm::tvec3<int32_t> max;            // The bounding box max position
    int leafface;             // The first index into the face array
    int leaf_faces_num;       // The number of faces for this leaf
    int leaf_brush;            // The first index for into the brushes
    int leaf_brushes_num;     // The number of brushes for this leaf
};

// splitter plane in the BSP tree
struct BSPPlane {
    glm::vec3 normal;         // Plane normal.
    float d;                  // The plane distance from origin
};

// cluster data for the PVS's
struct BSPVisData {
    int clusters_num;        // The number of clusters
    int bytes_per_cluster;      // The amount of bytes (8 bits) in the cluster's bitset
    std::vector<uint8_t> bitsets; // The array of bytes that holds the cluster bitsets
};

// brush data
struct BSPBrush {
    int brush_side;            // The starting brush side for the brush
    int brush_sides_num;      // Number of brush sides for the brush
    int texture_id;            // The texture index for the brush
};

// brush side data, which stores indices for the normal and texture _id
struct BSPBrushSide {
    int plane;                // The plane index
    int texture_id;            // The texture index
};

// lumps enumeration
enum eLumps {
    LUMP_ENTITIES = 0,            // Stores player/object positions, etc...
    LUMP_TEXTURES,                // Stores texture information
    LUMP_PLANES,                  // Stores the splitting planes
    LUMP_NODES,                   // Stores the BSP nodes
    LUMP_LEAFS,                   // Stores the leafs of the nodes
    LUMP_LEAF_FACES,               // Stores the leaf's indices into the faces
    LUMP_LEAF_BRUSHES,             // Stores the leaf's indices into the brushes
    LUMP_MODELS,                  // Stores the info of world models
    LUMP_BRUSHES,                 // Stores the brushes info (for collision)
    LUMP_BRUSH_SIDES,              // Stores the brush surfaces info
    LUMP_VERTICES,                // Stores the level vertices
    LUMP_INDICES,                 // Stores the level indices
    LUMP_SHADERS,                 // Stores the shader files (blending, anims..)
    LUMP_FACES,                   // Stores the faces for the level
    LUMP_LIGHTMAPS,               // Stores the lightmaps for the level
    LUMP_LIGH_VOLUMES,            // Stores extra world lighting information
    LUMP_VIS_DATA,                 // Stores PVS and cluster info (visibility)
    LUMP_MAX_LUMPS                 // A constant to store the number of lumps
};

// Quake3 BSP class
class Quake3Bsp {
private:
    std::shared_ptr<spdlog::logger> _logger;

public:

    Quake3Bsp();
    ~Quake3Bsp();

    // This loads a .bsp file by it's file name (Returns true if successful)
    bool load_bsp(const std::string &filename);

    // This renders the level to the screen, currently the camera pos isn't being used
    void render(const glm::vec3 &pos);

    // This traces a single ray and checks collision with brushes
    glm::vec3 trace_ray(glm::vec3 start, glm::vec3 end);

    // This traces a sphere along a ray to check for collision with the brushes
    glm::vec3 trace_sphere(glm::vec3 start, glm::vec3 end, float radius);

    // This traces a axis-aligned bounding box (AABB) along a ray to check for collision
    glm::vec3 trace_box(glm::vec3 start, glm::vec3 end, glm::vec3 min, glm::vec3 max);

    // This function tells us whether or not we are on the ground or still falling
    bool is_on_ground() { return _is_grounded; }

    // This tells us if we have just collided
    bool collided()      { return _is_collided; }

    // This destroys the level data
    void destroy();

    // This finds a leaf in the BSP tree according to the position passed in
    int find_leaf(const glm::vec3 &pos);

    // This tells us if a cluster is visible or not
    int is_cluster_visible(int current, int test);
    void render_face(int faceIndex);

private:
    bool create_texture(uint32_t &texture, const std::string &filename);

    // This manually changes the gamma levels of an image
    void change_gamma(uint8_t *pImage, int size, float factor);

    // This creates a texture map from the lightmap image bits
    void create_lightmap_texture(uint32_t &texture, uint8_t *pImageBits, int width, int height);

    // This checks to see if we can step up over a collision (like a step)
    glm::vec3 try_step(glm::vec3 start, glm::vec3 end);

    // This traverses the BSP tree to check our movement vector with the brushes
    glm::vec3 trace(glm::vec3 start, glm::vec3 end);

    // This recursively checks all the nodes until we find leafs that store the brushes
    void check_node(int nodeIndex, float starRatio, float endRatio, glm::vec3 start, glm::vec3 end);

    // This checks our movement vector against the brush and it's sides
    void check_brush(BSPBrush *pBrush, glm::vec3 start, glm::vec3 end);

    // This attaches the correct extension to the file name, if found
    void find_texture(char *filename);
    void tesselate(int control_offset, int control_width, int vert_offset, int index_offset);

    // This renders a single face to the screen

    int _textures_num = 0;      // The number of texture maps
    int _lightmaps_num = 0;     // The number of light maps
    int _leafs_num = 0;         // The number of leafs

    int _traceType = 0;          // This stores if we are checking a ray, sphere or a box
    float _traceRatio = 0;       // This stores the ratio from our start pos to the intersection pt.
    float _traceRadius = 0;      // This stores the sphere's radius for a collision offset

    bool _is_collided = false;     // This tells if we just collided or not

    bool _is_grounded = false;     // This stores whether or not we are on the ground or falling
    bool _is_try_step = false;      // This tells us whether or not we should try to step over something

    glm::vec3 _traceMins = {0, 0, 0};      // This stores the minimum values of the AABB (bottom corner)
    glm::vec3 _traceMaxs = {0, 0, 0};      // This stores the maximum values of the AABB (top corner)
    glm::vec3 _extents = {0, 0, 0};        // This stores the largest length of the box
    glm::vec3 _collisionNormal = {0, 0, 0};// This stores the normal of the plane we collided with

    std::vector<int> _indices;
    std::vector<BSPVertex> _verts;
    std::vector<BSPNode> _nodes;
    std::vector<BSPFace> _faces;
    std::vector<BSPLeaf> _leafs;
    std::vector<BSPPlane> _planes;
    std::vector<int> _leaf_faces;
    std::vector<BSPTexture> _textures;
    std::vector<BSPBrush> _brushes;
    std::vector<BSPBrushSide> _brush_sides;
    std::vector<int> _leaf_brushes;
    BSPVisData   _clusters = {};

    uint32_t _textures_list[MAX_TEXTURES];        // The texture array for the world
    uint32_t _lightmaps_list[MAX_TEXTURES];       // The lightmap texture array

    std::vector<bool> _faces_drawn;           // The bitset for the faces that have/haven't been drawn
};
