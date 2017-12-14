#include <SDL_image.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

#include <thread>
#include <chrono>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <game/sys/quake3_bsp.h>
#include <game/config.h>
#define MAX_PATH 255

#define BEZIER_LEVEL 3

using namespace game;

using namespace std::chrono_literals;

Quake3Bsp::Quake3Bsp() {
    _logger = spdlog::stdout_color_mt("bsp");
}

bool Quake3Bsp::create_texture(uint32_t &texture, const std::string &filename) {
    _logger->info("Loading texture: {}", filename);
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> img =
        decltype(img)(IMG_Load((Config::data_path() + filename).c_str()),
                      SDL_FreeSurface);
    if(!img) {
        _logger->error("Failed to load image, SDL_error: {}", IMG_GetError());
        return false;
    }
    // Generate a texture with the associative texture _id stored in the array
    glGenTextures(1, &texture);

    // This sets the alignment requirements for the start of each pixel row in memory.
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    // Bind the texture to the texture arrays index and init the texture
    glBindTexture(GL_TEXTURE_2D, texture);

    int texture_type = GL_RGB;
    if(img->format->BytesPerPixel == 4) texture_type = GL_RGBA;

    // Build Mipmaps (builds different versions of the picture for distances - looks better)
    gluBuild2DMipmaps(GL_TEXTURE_2D, img->format->BytesPerPixel, img->w,
                      img->h, texture_type, GL_UNSIGNED_BYTE, img->pixels);

    //Assign the mip map levels and texture info
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    return true;
}


// This is our maximum height that the user can climb over
const float MAX_STEP_HEIGHT = 10.0f;

// This will store how many faces are drawn and are seen by the camera
static int g_VisibleFaces = 0;

// This tells us if we want to render the lightmaps
static bool g_bLightmaps = true;

// This holds the gamma value that was stored in the config file
static float g_Gamma = 3;

// This tells us if we want to render the textures
static bool g_bTextures = true;

void Quake3Bsp::change_gamma(uint8_t *pImage, int size, float factor) {
    // Go through every pixel in the lightmap
    for(int i = 0; i < size / 3; i++, pImage += 3) {
        float scale = 1.0f, temp = 0.0f;
        float r = 0, g = 0, b = 0;

        // extract the current RGB values
        r = (float)pImage[0];
        g = (float)pImage[1];
        b = (float)pImage[2];

        // Multiply the factor by the RGB values, while keeping it to a 255 ratio
        r = r * factor / 255.0f;
        g = g * factor / 255.0f;
        b = b * factor / 255.0f;

        // Check if the the values went past the highest value
        if(r > 1.0f && (temp = (1.0f/r)) < scale) scale=temp;
        if(g > 1.0f && (temp = (1.0f/g)) < scale) scale=temp;
        if(b > 1.0f && (temp = (1.0f/b)) < scale) scale=temp;

        // Get the scale for this pixel and multiply it by our pixel values
        scale*=255.0f;
        r*=scale;   g*=scale;   b*=scale;

        // Assign the new gamma'nized RGB values to our image
        pImage[0] = (uint8_t)r;
        pImage[1] = (uint8_t)g;
        pImage[2] = (uint8_t)b;
    }
}


void Quake3Bsp::create_lightmap_texture(uint32_t &texture, uint8_t *pImageBits, int width, int height) {
    // Generate a texture with the associative texture _id stored in the array
    glGenTextures(1, &texture);

    // This sets the alignment requirements for the start of each pixel row in memory.
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    // Bind the texture to the texture arrays index and init the texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // Change the lightmap gamma values by our desired gamma
    change_gamma(pImageBits, width*height*3, g_Gamma);

    // Build Mipmaps (builds different versions of the picture for distances - looks better)
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, pImageBits);

    //Assign the mip map levels
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static bool file_exists(const std::string& str) {
   std::ifstream fs(str);
   return fs.is_open();
}

void Quake3Bsp::find_texture(char *filename) {
    std::string jpg_path = filename + std::string(".jpg");
    std::string tga_path = filename + std::string(".tga");

    if(file_exists(Config::data_path() + jpg_path)) {
        std::copy(jpg_path.begin(), jpg_path.end(), filename);
        return;
    }

    if(file_exists(Config::data_path() + tga_path)) {
        std::copy(tga_path.begin(), tga_path.end(), filename);
        return;
    }
}

BSPVertex operator+(const BSPVertex& v1, const BSPVertex& v2) {
    BSPVertex temp;
    temp.position = v1.position + v2.position;
    temp.texture_coord = v1.texture_coord + v2.texture_coord;
    temp.lightmap_coord = v1.lightmap_coord + v2.lightmap_coord;
    temp.normal = v1.normal + v2.normal;
    return temp;
}

BSPVertex operator*(const BSPVertex& v1, const float& d) {
    BSPVertex temp;
    temp.position = v1.position * d;
    temp.texture_coord = v1.texture_coord * d;
    temp.lightmap_coord = v1.lightmap_coord * d;
    temp.normal = v1.normal * d;
    return temp;
}

void Quake3Bsp::tesselate(int control_offset, int control_width, int vert_offset, int index_offset) {
    BSPVertex controls[9];
    int cIndex = 0;
    for (int c = 0; c < 3; c++) {
        int pos = c * control_width;
        controls[cIndex++] = _verts[control_offset + pos];
        controls[cIndex++] = _verts[control_offset + pos + 1];
        controls[cIndex++] = _verts[control_offset + pos + 2];
    }

    int L1 = BEZIER_LEVEL + 1;

    for (int j = 0; j <= BEZIER_LEVEL; ++j) {
        float a = (float)j / BEZIER_LEVEL;
        float b = 1.f - a;
        _verts[vert_offset + j] = controls[0] * b * b + controls[3] * 2 * b * a + controls[6] * a * a;
    }

    for (int i = 1; i <= BEZIER_LEVEL; ++i) {
        float a = (float)i / BEZIER_LEVEL;
        float b = 1.f - a;

        BSPVertex temp[3];

        for (int j = 0; j < 3; ++j) {
            int k = 3 * j;
            temp[j] = controls[k + 0] * b * b + controls[k + 1] * 2 * b * a + controls[k + 2] * a * a;
        }

        for (int j = 0; j <= BEZIER_LEVEL; ++j) {
            float a = (float)j / BEZIER_LEVEL;
            float b = 1.f - a;

            _verts[vert_offset + i * L1 + j] = temp[0] * b * b + temp[1] * 2 * b * a + temp[2] * a * a;
        }
    }

    for (int i = 0; i <= BEZIER_LEVEL; ++i) {
        for (int j = 0; j <= BEZIER_LEVEL; ++j) {
            int offset = index_offset + (i * BEZIER_LEVEL + j) * 6;
            _indices[offset + 0] = (i    ) * L1 + (j    ) + vert_offset;
            _indices[offset + 1] = (i    ) * L1 + (j + 1) + vert_offset;
            _indices[offset + 2] = (i + 1) * L1 + (j + 1) + vert_offset;

            _indices[offset + 3] = (i + 1) * L1 + (j + 1) + vert_offset;
            _indices[offset + 4] = (i + 1) * L1 + (j    ) + vert_offset;
            _indices[offset + 5] = (i    ) * L1 + (j    ) + vert_offset;
        }
    }
}

bool Quake3Bsp::load_bsp(const std::string &filename) {
    FILE *fp = NULL;
    int i = 0;

    if((fp = fopen(filename.c_str(), "rb")) == NULL) {
        _logger->error("Couldn't find BSP file {}", filename);
        return false;
    }

    BSPHeader header = {};
    BSPLump lumps[LUMP_MAX_LUMPS] = {};

    fread(&header, 1, sizeof(BSPHeader), fp);
    fread(&lumps, LUMP_MAX_LUMPS, sizeof(BSPLump), fp);

    size_t faces_num = lumps[LUMP_FACES].length / sizeof(BSPFace);
    _faces.resize(faces_num);

    fseek(fp, lumps[LUMP_FACES].offset, SEEK_SET);

    fread(&_faces[0], faces_num, sizeof(BSPFace), fp);
    int bezier_count = 0;
	int bezier_patch_size = (BEZIER_LEVEL + 1) * (BEZIER_LEVEL + 1);
	int bezier_index_size = BEZIER_LEVEL * BEZIER_LEVEL * 6;
	for (int i = 0; i < faces_num; i++) {
		if(_faces[i].type != FACE_PATCH) continue;
		int dimx = (_faces[i].size[0] - 1) / 2;
		int dimy = (_faces[i].size[0] - 1) / 2;
		int size = dimx * dimy;
		bezier_count += size;
	}

    size_t indices_num = lumps[LUMP_INDICES].length / sizeof(int);
    _indices.resize(indices_num + bezier_index_size * bezier_count);

    size_t verts_num = lumps[LUMP_VERTICES].length / sizeof(BSPVertex);
    _verts.resize(verts_num + bezier_patch_size * bezier_count);

    _textures_num = lumps[LUMP_TEXTURES].length / sizeof(BSPTexture);
    _textures.resize(_textures_num);

    _lightmaps_num = lumps[LUMP_LIGHTMAPS].length / sizeof(BSPLightmap);
    std::vector<BSPLightmap> lightmaps(_lightmaps_num);

    fseek(fp, lumps[LUMP_VERTICES].offset, SEEK_SET);


    for(i = 0; i < verts_num; i++) {
        fread(&_verts[i], 1, sizeof(BSPVertex), fp);
        // Swap the y and z values, and negate the new z so Y is up.
        float temp = _verts[i].position.y;
        _verts[i].position.y = _verts[i].position.z;
        _verts[i].position.z = -temp;
    }

    fseek(fp, lumps[LUMP_INDICES].offset, SEEK_SET);

    fread(&_indices[0], indices_num, sizeof(int), fp);


    //for (int i = 0, vOffset = verts_num, iOffset = indices_num; i < faces_num; i++) {
        //BSPFace &face = _faces[i];
        //if (face.type == FACE_PATCH) {
            //int dimX = (face.size[0] - 1) / 2;
            //int dimY = (face.size[1] - 1) / 2;

            //face.start_index = iOffset;

            //for (int x = 0, n = 0; n < dimX; n++, x = 2 * n) {
                //for (int y = 0, m = 0; m < dimY; m++, y = 2 * m) {
                    //tesselate(face.start_vert_index + x + face.size[0] * y,
                              //face.size[0], vOffset, iOffset);
                    //vOffset += bezier_patch_size;
                    //iOffset += bezier_index_size;
                //}
            //}

            //face.indices_num = iOffset - face.start_index;
        //} else {
            //for (int i = 0; i < face.indices_num; i++) {
                //_indices[face.start_index + i] += face.start_vert_index;
            //}
        //}
    //}

    fseek(fp, lumps[LUMP_TEXTURES].offset, SEEK_SET);

    fread(&_textures[0], _textures_num, sizeof(BSPTexture), fp);

    for(i = 0; i < _textures_num; i++) {
        find_texture(_textures[i].name);
        create_texture(_textures_list[i], _textures[i].name);
    }

    fseek(fp, lumps[LUMP_LIGHTMAPS].offset, SEEK_SET);

    for(i = 0; i < _lightmaps_num ; i++) {
        fread(&lightmaps[i], 1, sizeof(BSPLightmap), fp);

        // Create a texture map for each lightmap that is read in.  The lightmaps
        // are always 128 by 128.
        create_lightmap_texture(_lightmaps_list[i],
                             (unsigned char *)lightmaps[i].imageBits, 128, 128);
    }

    size_t nodes_num = lumps[LUMP_NODES].length / sizeof(BSPNode);
    _nodes.resize(nodes_num);

    fseek(fp, lumps[LUMP_NODES].offset, SEEK_SET);
    fread(&_nodes[0], nodes_num, sizeof(BSPNode), fp);

    _leafs_num = lumps[LUMP_LEAFS].length / sizeof(BSPLeaf);
    _leafs.resize(_leafs_num);

    fseek(fp, lumps[LUMP_LEAFS].offset, SEEK_SET);
    fread(&_leafs[0], _leafs_num, sizeof(BSPLeaf), fp);

    // Now we need to go through and convert all the leaf bounding boxes
    // to the normal OpenGL Y up axis.
    for(i = 0; i < _leafs_num; i++) {
        // Swap the min y and z values, then negate the new Z
        int temp = _leafs[i].min.y;
        _leafs[i].min.y = _leafs[i].min.z;
        _leafs[i].min.z = -temp;

        // Swap the max y and z values, then negate the new Z
        temp = _leafs[i].max.y;
        _leafs[i].max.y = _leafs[i].max.z;
        _leafs[i].max.z = -temp;
    }

    size_t leaf_faces_num = lumps[LUMP_LEAF_FACES].length / sizeof(int);
    _leaf_faces.resize(leaf_faces_num);

    fseek(fp, lumps[LUMP_LEAF_FACES].offset, SEEK_SET);
    fread(&_leaf_faces[0], leaf_faces_num, sizeof(int), fp);

    size_t planes_num = lumps[LUMP_PLANES].length / sizeof(BSPPlane);
    _planes.resize(planes_num);

    fseek(fp, lumps[LUMP_PLANES].offset, SEEK_SET);
    fread(&_planes[0], planes_num, sizeof(BSPPlane), fp);

    for(i = 0; i < planes_num; i++) {
        float temp = _planes[i].normal.y;
        _planes[i].normal.y = _planes[i].normal.z;
        _planes[i].normal.z = -temp;
    }

    fseek(fp, lumps[LUMP_VIS_DATA].offset, SEEK_SET);

    if(lumps[LUMP_VIS_DATA].length) {
        fread(&(_clusters.clusters_num),     1, sizeof(int), fp);
        fread(&(_clusters.bytes_per_cluster), 1, sizeof(int), fp);

        int size = _clusters.clusters_num * _clusters.bytes_per_cluster;
        _clusters.bitsets.resize(size);

        fread(&_clusters.bitsets[0], 1, sizeof(uint8_t) * size, fp);
    }

    size_t brushes_num = lumps[LUMP_BRUSHES].length / sizeof(int);
    _brushes.resize(brushes_num);

    fseek(fp, lumps[LUMP_BRUSHES].offset, SEEK_SET);
    fread(&_brushes[0], brushes_num, sizeof(BSPBrush), fp);

    size_t brush_sides_num = lumps[LUMP_BRUSH_SIDES].length / sizeof(int);
    _brush_sides.resize(brush_sides_num);

    // Read in the brush sides data
    fseek(fp, lumps[LUMP_BRUSH_SIDES].offset, SEEK_SET);
    fread(&_brush_sides[0], brush_sides_num, sizeof(BSPBrushSide), fp);

    size_t leaf_brushes_num = lumps[LUMP_LEAF_BRUSHES].length / sizeof(int);
    _leaf_brushes.resize(leaf_brushes_num);

    fseek(fp, lumps[LUMP_LEAF_BRUSHES].offset, SEEK_SET);
    fread(&_leaf_brushes[0], leaf_brushes_num, sizeof(int), fp);

    fclose(fp);

    _faces_drawn.resize(faces_num);

    return true;
}

int Quake3Bsp::find_leaf(const glm::vec3 &pos) {
    int i = 0;
    float distance = 0.0f;

    // Continue looping until we find a negative index
    while(i >= 0) {
        // Get the current node, then find the slitter plane from that
        // node's plane index.  Notice that we use a constant reference
        // to store the plane and node so we get some optimization.
        const BSPNode&  node = _nodes[i];
        const BSPPlane& plane = _planes[node.plane];

        // Use the Plane Equation (Ax + by + Cz + D = 0) to find if the
        // camera is in front of or behind the current splitter plane.
        distance =    plane.normal.x * pos.x +
                    plane.normal.y * pos.y +
                    plane.normal.z * pos.z - plane.d;

        // If the camera is in front of the plane
        if(distance >= 0) {
            // Assign the current node to the node in front of itself
            i = node.front;
        }
        // Else if the camera is behind the plane
        else {
            // Assign the current node to the node behind itself
            i = node.back;
        }
    }

    // Return the leaf index (same thing as saying:  return -(i + 1)).
    return ~i;  // Binary operation
}

inline int Quake3Bsp::is_cluster_visible(int current, int test) {
    // Make sure we have valid memory and that the current cluster is > 0.
    // If we don't have any memory or a negative cluster, return a visibility (1).
    if(_clusters.bitsets.empty() || current < 0) return 1;

    // Use binary math to get the 8 bit visibility set for the current cluster
    uint8_t visSet = _clusters.bitsets[(current*_clusters.bytes_per_cluster) + (test / 8)];

    // Now that we have our vector (bitset), do some bit shifting to find if
    // the "test" cluster is visible from the "current" cluster, according to the bitset.
    int result = visSet & (1 << ((test) & 7));

    // Return the result ( either 1 (visible) or 0 (not visible) )
    return ( result );
}

glm::vec3 Quake3Bsp::try_step(glm::vec3 start, glm::vec3 end) {

    // Go through and check different heights to step up
    for(float height = 1.0f; height <= MAX_STEP_HEIGHT; height++) {
        // Reset our variables for each loop interation
        _is_collided = false;
        _is_try_step = false;

        // Here we add the current height to our y position of a new start and end.
        // If these 2 new start and end positions are okay, we can step up.
        glm::vec3 stepStart = glm::vec3(start.x, start.y + height, start.z);
        glm::vec3 stepEnd   = glm::vec3(end.x, start.y + height, end.z);

        // Test to see if the new position we are trying to step collides or not
        glm::vec3 stepPosition = trace(stepStart, stepEnd);

        if(!_is_collided) {

            // Return the current position since we stepped up somewhere
            return stepPosition;
        }
    }

    // If we can't step, then we just return the original position of the collision
    return start;
}

glm::vec3 Quake3Bsp::trace(glm::vec3 start, glm::vec3 end) {
    // Initially we set our trace ratio to 1.0f, which means that we don't have
    // a collision or intersection point, so we can move freely.
    _traceRatio = 1.0f;

    // We start out with the first node (0), setting our start and end ratio to 0 and 1.
    // We will recursively go through all of the nodes to see which brushes we should check.
    check_node(0, 0.0f, 1.0f, start, end);

    // If the traceRatio is STILL 1.0f, then we never collided and just return our end position
    if(_traceRatio == 1.0f) {
        return end;
    }
    else {
        // Set our new position to a position that is right up to the brush we collided with
        glm::vec3 newPosition = start + ((end - start) * _traceRatio);

        // Get the distance from the end point to the new position we just got
        glm::vec3 move = end - newPosition;

        // Get the distance we need to travel backwards to the new slide position.
        // This is the distance of course along the normal of the plane we collided with.
        float distance = glm::dot(move, _collisionNormal);

        // Get the new end position that we will end up (the slide position).
        glm::vec3 endPosition = end - _collisionNormal*distance;

        // Since we got a new position for our sliding vector, we need to check
        // to make sure that new sliding position doesn't collide with anything else.
        newPosition = trace(newPosition, endPosition);

        if(_collisionNormal.y > 0.2f || _is_grounded)
            _is_grounded = true;
        else
            _is_grounded = false;

        // Return the new position to be used by our camera (or player)
        return newPosition;
    }
}

glm::vec3 Quake3Bsp::trace_ray(glm::vec3 start, glm::vec3 end) {
    // We don't use this function, but we set it up to allow us to just check a
    // ray with the BSP tree brushes.  We do so by setting the trace type to TYPE_RAY.
    _traceType = TYPE_RAY;

    // Run the normal trace() function with our start and end
    // position and return a new position
    return trace(start, end);
}


glm::vec3 Quake3Bsp::trace_sphere(glm::vec3 start, glm::vec3 end, float radius) {
    // Here we initialize the type of trace (SPHERE) and initialize other data
    _traceType = TYPE_SPHERE;
    _is_collided = false;

    // Here we initialize our variables for a new round of collision checks
    _is_try_step = false;
    _is_grounded = false;

    _traceRadius = radius;

    // Get the new position that we will return to the camera or player
    glm::vec3 newPosition = trace(start, end);

    // Let's check to see if we collided with something and we should try to step up
    if(_is_collided && _is_try_step) {
        // Try and step up what we collided with
        newPosition = try_step(newPosition, end);
    }

    // Return the new position to be changed for the camera or player
    return newPosition;
}

glm::vec3 Quake3Bsp::trace_box(glm::vec3 start, glm::vec3 end, glm::vec3 min, glm::vec3 max) {
    _traceType = TYPE_BOX;            // Set the trace type to a BOX
    _traceMaxs = max;            // Set the max value of our AABB
    _traceMins = min;            // Set the min value of our AABB
    _is_collided = false;            // Reset the collised flag


    // Here we initialize our variables for a new round of collision checks
    _is_try_step = false;
    _is_grounded = false;

    // Grab the extend of our box (the largest size for each x, y, z axis)
    _extents = glm::vec3(-_traceMins.x > _traceMaxs.x ? -_traceMins.x : _traceMaxs.x,
                          -_traceMins.y > _traceMaxs.y ? -_traceMins.y : _traceMaxs.y,
                          -_traceMins.z > _traceMaxs.z ? -_traceMins.z : _traceMaxs.z);


    // Check if our movement collided with anything, then get back our new position
    glm::vec3 newPosition = trace(start, end);


    // Let's check to see if we collided with something and we should try to step up
    if(_is_collided && _is_try_step) {
        // Try and step up what we collided with
        newPosition = try_step(newPosition, end);
    }

    // Return our new position
    return newPosition;
}


void Quake3Bsp::check_node(int nodeIndex, float startRatio, float endRatio, glm::vec3 start, glm::vec3 end) {
    // Check if the next node is a leaf
    if(nodeIndex < 0) {
        // If this node in the BSP is a leaf, we need to negate and add 1 to offset
        // the real node index into the _leafs[] array.  You could also do [~nodeIndex].
        BSPLeaf *pLeaf = &_leafs[-(nodeIndex + 1)];

        // We have a leaf, so let's go through all of the brushes for that leaf
        for(int i = 0; i < pLeaf->leaf_brushes_num; i++) {
            // Get the current brush that we going to check
            BSPBrush *pBrush = &_brushes[_leaf_brushes[pLeaf->leaf_brush + i]];

            // Check if we have brush sides and the current brush is solid and collidable
            if((pBrush->brush_sides_num > 0) && (_textures[pBrush->texture_id].texture_type & 1)) {
                // Now we delve into the dark depths of the real calculations for collision.
                // We can now check the movement vector against our brush planes.
                check_brush(pBrush, start, end);
            }
        }

        // Since we found the brushes, we can go back up and stop recursing at this level
        return;
    }

    // Grad the next node to work with and grab this node's plane data
    BSPNode *pNode = &_nodes[nodeIndex];
    BSPPlane *pPlane = &_planes[pNode->plane];

    // Here we use the plane equation to find out where our initial start position is
    // according the the node that we are checking.  We then grab the same info for the end pos.
    float startDistance = glm::dot(start, pPlane->normal) - pPlane->d;
    float endDistance = glm::dot(end, pPlane->normal) - pPlane->d;
    float offset = 0.0f;

    // If we are doing sphere collision, include an offset for our collision tests below
    if(_traceType == TYPE_SPHERE)
        offset = _traceRadius;

    // Here we check to see if we are working with a BOX or not
    else if(_traceType == TYPE_BOX) {
        // Get the distance our AABB is from the current splitter plane
        offset = (float)(fabs( _extents.x * pPlane->normal.x ) +
                         fabs( _extents.y * pPlane->normal.y ) +
                         fabs( _extents.z * pPlane->normal.z ) );
    }

    // Here we check to see if the start and end point are both in front of the current node.
    // If so, we want to check all of the nodes in front of this current splitter plane.
    if(startDistance >= offset && endDistance >= offset) {
        // Traverse the BSP tree on all the nodes in front of this current splitter plane
        check_node(pNode->front, startDistance, endDistance, start, end);
    }
    // If both points are behind the current splitter plane, traverse down the back nodes
    else if(startDistance < -offset && endDistance < -offset) {
        // Traverse the BSP tree on all the nodes in back of this current splitter plane
        check_node(pNode->back, startDistance, endDistance, start, end);
    }
    else {
        // If we get here, then our ray needs to be split in half to check the nodes
        // on both sides of the current splitter plane.  Thus we create 2 ratios.
        float Ratio1 = 1.0f, Ratio2 = 0.0f, middleRatio = 0.0f;
        glm::vec3 middle;    // This stores the middle point for our split ray

        // Start of the side as the front side to check
        int side = pNode->front;

        // Here we check to see if the start point is in back of the plane (negative)
        if(startDistance < endDistance) {
            // Since the start position is in back, let's check the back nodes
            side = pNode->back;

            // Here we create 2 ratios that hold a distance from the start to the
            // extent closest to the start (take into account a sphere and epsilon).
            float inverseDistance = 1.0f / (startDistance - endDistance);
            Ratio1 = (startDistance - offset - M_EPS) * inverseDistance;
            Ratio2 = (startDistance + offset + M_EPS) * inverseDistance;
        }
        // Check if the starting point is greater than the end point (positive)
        else if(startDistance > endDistance) {
            // This means that we are going to recurse down the front nodes first.
            // We do the same thing as above and get 2 ratios for split ray.
            float inverseDistance = 1.0f / (startDistance - endDistance);
            Ratio1 = (startDistance + offset + M_EPS) * inverseDistance;
            Ratio2 = (startDistance - offset - M_EPS) * inverseDistance;
        }

        // Make sure that we have valid numbers and not some weird float problems.
        // This ensures that we have a value from 0 to 1 as a good ratio should be :)
        if (Ratio1 < 0.0f) Ratio1 = 0.0f;
        else if (Ratio1 > 1.0f) Ratio1 = 1.0f;

        if (Ratio2 < 0.0f) Ratio2 = 0.0f;
        else if (Ratio2 > 1.0f) Ratio2 = 1.0f;

        // Just like we do in the trace() function, we find the desired middle
        // point on the ray, but instead of a point we get a middleRatio percentage.
        middleRatio = startRatio + ((endRatio - startRatio) * Ratio1);
        middle = start + ((end - start) * Ratio1);

        // Now we recurse on the current side with only the first half of the ray
        check_node(side, startRatio, middleRatio, start, middle);

        // Now we need to make a middle point and ratio for the other side of the node
        middleRatio = startRatio + ((endRatio - startRatio) * Ratio2);
        middle = start + ((end - start) * Ratio2);

        // Depending on which side should go last, traverse the bsp with the
        // other side of the split ray (movement vector).
        if(side == pNode->back)
            check_node(pNode->front, middleRatio, endRatio, middle, end);
        else
            check_node(pNode->back, middleRatio, endRatio, middle, end);
    }
}

void Quake3Bsp::check_brush(BSPBrush *pBrush, glm::vec3 start, glm::vec3 end) {
    float startRatio = -1.0f;        // Like in BrushCollision.htm, start a ratio at -1
    float endRatio = 1.0f;            // Set the end ratio to 1
    bool startsOut = false;            // This tells us if we starting outside the brush

    // Go through all of the brush sides and check collision against each plane
    for(int i = 0; i < pBrush->brush_sides_num; i++) {
        // Here we grab the current brush side and plane in this brush
        BSPBrushSide *pBrushSide = &_brush_sides[pBrush->brush_side + i];
        BSPPlane *pPlane = &_planes[pBrushSide->plane];

        // Let's store a variable for the offset (like for sphere collision)
        float offset = 0.0f;

        // If we are testing sphere collision we need to add the sphere radius
        if(_traceType == TYPE_SPHERE)
            offset = _traceRadius;

        // Test the start and end points against the current plane of the brush side.
        // Notice that we add an offset to the distance from the origin, which makes
        // our sphere collision work.
        float startDistance = glm::dot(start, pPlane->normal) - (pPlane->d + offset);
        float endDistance = glm::dot(end, pPlane->normal) - (pPlane->d + offset);

        // Store the offset that we will check against the plane
        glm::vec3 offset_vec = glm::vec3(0, 0, 0);

        // If we are using AABB collision
        if(_traceType == TYPE_BOX) {
            // Grab the closest corner (x, y, or z value) that is closest to the plane
            offset_vec.x = (pPlane->normal.x < 0)    ? _traceMaxs.x : _traceMins.x;
            offset_vec.y = (pPlane->normal.y < 0)    ? _traceMaxs.y : _traceMins.y;
            offset_vec.z = (pPlane->normal.z < 0)    ? _traceMaxs.z : _traceMins.z;

            // Use the plane equation to grab the distance our start position is from the plane.
            startDistance = glm::dot(start + offset_vec, pPlane->normal) - pPlane->d;

            // Get the distance our end position is from this current brush plane
            endDistance   = glm::dot(end + offset_vec, pPlane->normal) - pPlane->d;
        }

        // Make sure we start outside of the brush's volume
        if(startDistance > 0)    startsOut = true;

        // Stop checking since both the start and end position are in front of the plane
        if(startDistance > 0 && endDistance > 0)
            return;

        // Continue on to the next brush side if both points are behind or on the plane
        if(startDistance <= 0 && endDistance <= 0)
            continue;

        // If the distance of the start point is greater than the end point, we have a collision!
        if(startDistance > endDistance) {
            // This gets a ratio from our starting point to the approximate collision spot
            float Ratio1 = (startDistance - M_EPS) / (startDistance - endDistance);

            // If this is the first time coming here, then this will always be true,
            if(Ratio1 > startRatio) {
                // Set the startRatio (currently the closest collision distance from start)
                startRatio = Ratio1;
                _is_collided = true;        // Let us know we collided!

                // Store the normal of plane that we collided with for sliding calculations
                _collisionNormal = pPlane->normal;


                // This checks first tests if we actually moved along the x or z-axis,
                // meaning that we went in a direction somewhere.  The next check makes
                // sure that we don't always check to step every time we collide.  If
                // the normal of the plane has a Y value of 1, that means it's just the
                // flat ground and we don't need to check if we can step over it, it's flat!
                if((start.x != end.x || start.z != end.z) && pPlane->normal.y != 1) {
                    // We can try and step over the wall we collided with
                    _is_try_step = true;
                }

                // Here we make sure that we don't slide slowly down walls when we
                // jump and collide into them.  We only want to say that we are on
                // the ground if we actually have stopped from falling.  A wall wouldn't
                // have a high y value for the normal, it would most likely be 0.
                if(_collisionNormal.y >= 0.2f)
                    _is_grounded = true;
            }
        }
        else {
            // Get the ratio of the current brush side for the endRatio
            float Ratio = (startDistance + M_EPS) / (startDistance - endDistance);

            // If the ratio is less than the current endRatio, assign a new endRatio.
            // This will usually always be true when starting out.
            if(Ratio < endRatio)
                endRatio = Ratio;
        }
    }

    // If we didn't start outside of the brush we don't want to count this collision - return;
    if(startsOut == false) {
        return;
    }

    // If our startRatio is less than the endRatio there was a collision!!!
    if(startRatio < endRatio) {
        // Make sure the startRatio moved from the start and check if the collision
        // ratio we just got is less than the current ratio stored in _traceRatio.
        // We want the closest collision to our original starting position.
        if(startRatio > -1 && startRatio < _traceRatio) {
            // If the startRatio is less than 0, just set it to 0
            if(startRatio < 0)
                startRatio = 0;

            // Store the new ratio in our member variable for later
            _traceRatio = startRatio;
        }
    }
}

void Quake3Bsp::render_face(int faceIndex) {
    // Here we grab the face from the index passed in
    BSPFace *pFace = &_faces[faceIndex];

    // Assign our array of face vertices for our vertex arrays and enable vertex arrays
    glVertexPointer(3, GL_FLOAT, sizeof(BSPVertex), &(_verts[pFace->start_vert_index].position));
    glEnableClientState(GL_VERTEX_ARRAY);

    // If we want to render the textures
    if(g_bTextures) {
        // Set the current pass as the first texture (For multi-texturing)
        glActiveTextureARB(GL_TEXTURE0_ARB);

        // Give OpenGL the texture coordinates for the first texture, and enable that texture
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glTexCoordPointer(2, GL_FLOAT, sizeof(BSPVertex),
                                       &(_verts[pFace->start_vert_index].texture_coord));

        // Set our vertex array client states for allowing texture coordinates
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        // Turn on texture arrays for the first pass
        glClientActiveTextureARB(GL_TEXTURE0_ARB);

        // Turn on texture mapping and bind the face's texture map
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,  _textures_list[pFace->texture_id]);
    }

    if(g_bLightmaps) {
        // Set the current pass as the second lightmap texture_
        glActiveTextureARB(GL_TEXTURE1_ARB);

        // Turn on texture arrays for the second lightmap pass
        glClientActiveTextureARB(GL_TEXTURE1_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        // Next, we need to specify the UV coordinates for our lightmaps.  This is done
        // by switching to the second texture and giving OpenGL our lightmap array.
        glClientActiveTextureARB(GL_TEXTURE1_ARB);
        glTexCoordPointer(2, GL_FLOAT, sizeof(BSPVertex),
                                       &(_verts[pFace->start_vert_index].lightmap_coord));

        // Turn on texture mapping and bind the face's lightmap over the texture
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,  _lightmaps_list[pFace->lightmap_id]);
    }

    // Render our current face to the screen with vertex arrays
    glDrawElements(GL_TRIANGLES, pFace->indices_num, GL_UNSIGNED_INT, &(_indices[pFace->start_index]) );
}


void Quake3Bsp::render(const glm::vec3 &pos) {
    // Reset our bitset so all the slots are zero.
    std::fill(_faces_drawn.begin(), _faces_drawn.end(), 0);

    // Grab the leaf index that our camera is in
    int leafIndex = find_leaf(pos);

    // Grab the cluster that is assigned to the leaf
    int cluster = _leafs[leafIndex].cluster;

    // Initialize our counter variables (start at the last leaf and work down)
    int i = _leafs_num;
    g_VisibleFaces = 0;

    // Go through all the leafs and check their visibility
    while(i--) {
        // Get the current leaf that is to be tested for visibility from our camera's leaf
        BSPLeaf *pLeaf = &(_leafs[i]);

        // If the current leaf can't be seen from our cluster, go to the next leaf
        if(!is_cluster_visible(cluster, pLeaf->cluster))
            continue;

        // If we get here, the leaf we are testing must be visible in our camera's view.
        // Get the number of faces that this leaf is in charge of.
        int faceCount = pLeaf->leaf_faces_num;

        // Loop through and render all of the faces in this leaf
        while(faceCount--) {
            // Grab the current face index from our leaf faces array
            int faceIndex = _leaf_faces[pLeaf->leafface + faceCount];

            // Since many faces are duplicated in other leafs, we need to
            // make sure this face already hasn't been drawn.
            if(!_faces_drawn[faceIndex])  {
                // Increase the rendered face count to display for fun
                g_VisibleFaces++;

                _faces_drawn[faceIndex] = true;
                render_face(faceIndex);
            }
        }
    }
}

void Quake3Bsp::destroy() {
    glDeleteTextures(_textures_num, _textures_list);
    glDeleteTextures(_lightmaps_num, _lightmaps_list);
}

Quake3Bsp::~Quake3Bsp() {
    destroy();
}
