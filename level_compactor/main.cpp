#include <iostream>
#include <unordered_map>
#include <algorithm>
#include "util.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

typedef struct StagedBuffer {
    char* pointer;
    usize capacity;
    usize length;
} StagedBuffer;

global inline char* sbmalloc(StagedBuffer* heap, usize required_size) {
    usize would_be_offset = heap->length + required_size;

    if (would_be_offset > heap->capacity) {
        return nullptr;
    } else {
        char* pointer = heap->pointer + heap->length;
        heap->length += required_size;
        return pointer;
    }
}

global inline void sbclear(StagedBuffer* heap) {
    heap->length = 0;
}

global inline void sbfree(StagedBuffer* heap) {
    free(heap->pointer);
    heap->capacity = 0;
    heap->length = 0;
}

global inline void* sbcalloc(StagedBuffer* heap, u8 clear_value, usize required_size) {
    usize would_be_offset = heap->length + required_size;

    if (would_be_offset > heap->capacity) {
        return 0;
    } else {
        // set required_size bytes to 0 at position pointer + length
        char* pointer = heap->pointer + heap->length;
        memset(pointer, clear_value, required_size);

        heap->length += required_size;
        return pointer;
    }
}

global inline void sbinit(StagedBuffer* heap, usize allocation_size) {
    heap->length = 0;
    heap->capacity = allocation_size;
    heap->pointer = (char*)malloc(allocation_size);
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    bool operator==(const Vertex& other) const {
        return (position == other.position) && (normal == other.normal) && (uv == other.uv);
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                     (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

typedef struct Light {
    glm::vec4 position_falloff;
    glm::vec4 color_alpha;
} Light;

typedef struct LevelData {
    std::vector<Vertex> rendermesh_vertices;
    std::vector<u32> rendermesh_indices;
    std::vector<glm::vec3> physmesh_vertices;

    std::vector<Vertex> door_vertices;
    std::vector<u32> door_indices;
    std::vector<u32> door_requirements;
    std::vector<glm::vec4> door_positions_rotations;
    std::vector<u32> door_physmesh_range;

    std::vector<glm::vec3> keycard_positions;
    std::vector<glm::vec3> medium_positions;
    std::vector<glm::vec3> rat_positions;
    std::vector<glm::vec3> knight_positions;

    std::vector<Light> lights;

    glm::vec4 end_zone;
    glm::vec4 start_zone;
} LevelData;

void not_recognized_error(const std::string& name) {
    std::cerr << "Item \"" << name << "\" not recognized" << std::endl;
    exit(1);
}

void load_model(const char* file_path, LevelData* level_data) {//u32* out_vertex_count, Vertex* out_vertices, u32* out_index_count, u32* out_indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::vector<Vertex> vertices;
    std::vector<Vertex> all_vertices;
    std::unordered_map<Vertex, u32> unique_vertices{};
    std::vector<u32> indices;

    std::vector<Vertex> door_vertices;
    std::vector<Vertex> door_all_vertices;
    std::unordered_map<Vertex, u32> door_unique_vertices{};
    std::vector<u32> door_indices;
    std::vector<u32> door_requirements;
    std::vector<glm::vec4> door_position_rotations;

    std::vector<Light> lights;
    std::vector<glm::vec3> medium_positions;
    std::vector<glm::vec3> rat_positions;
    std::vector<glm::vec3> knight_positions;
    std::vector<glm::vec3> keycard_positions;
    u32 keycard_count = 0;
    keycard_positions.emplace_back(0.0, 0.0, 0.0);
    keycard_positions.emplace_back(0.0, 0.0, 0.0);
    keycard_positions.emplace_back(0.0, 0.0, 0.0);
    keycard_positions.emplace_back(0.0, 0.0, 0.0);

    glm::vec4 end_zone;
    glm::vec4 start_zone;

    bool physmesh_found = false, end_zone_found = false, start_zone_found = false;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path)) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Vertex> local_vertices;
    std::vector<Vertex> local_all_vertices;
    std::unordered_map<Vertex, u32> local_unique_vertices{};
    std::vector<u32> local_indices;
    printf("File: %s\n", file_path);
    for (const auto& shape: shapes) {
        local_vertices.clear();
        local_all_vertices.clear();
        local_unique_vertices.clear();
        local_indices.clear();
        f32 smallest_x = 4096.0, largest_x = -4096.0;
        f32 smallest_y = 4096.0, largest_y = -4096.0;

        for (const auto &index: shape.mesh.indices) {
            Vertex vertex;

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            vertex.uv = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2],
            };

            if (local_unique_vertices.count(vertex) == 0) {
                local_unique_vertices[vertex] = static_cast<u32>(local_vertices.size());
                local_vertices.push_back(vertex);
            }

            if(vertex.position.x < smallest_x) { smallest_x = vertex.position.x; }
            if(vertex.position.x > largest_x) { largest_x = vertex.position.x; }
            if(vertex.position.y < smallest_y) { smallest_y = vertex.position.y; }
            if(vertex.position.y > largest_y) { largest_y = vertex.position.y; }

            local_all_vertices.push_back(vertex);
            local_indices.push_back(local_unique_vertices[vertex]);
        }

        f32 diff_x = largest_x - smallest_x;
        f32 diff_y = largest_y - smallest_y;
        f32 rot = 0.0f;
        if(diff_x < diff_y) { rot = M_PI / 2.0; }

        glm::vec3 average_position = glm::vec3(0.0f, 0.0f, 0.0f);
        for(auto vertex: local_vertices) {
            average_position += vertex.position;
        }
        average_position /= (f32)local_vertices.size();

        glm::vec4 pr = glm::vec4(average_position.x, average_position.y, average_position.z, rot);

        if(shape.name.find("Door") == 0) {
            if(shape.name.find("Door_Model") == 0) {
                door_vertices = local_vertices;
                door_unique_vertices = local_unique_vertices;
                door_indices = local_indices;
                door_all_vertices = local_all_vertices;
            } else if(shape.name.find("Door_0") == 0) {
                door_position_rotations.push_back(pr);
                door_requirements.push_back(0);
            } else if(shape.name.find("Door_1") == 0) {
                door_position_rotations.push_back(pr);
                door_requirements.push_back(1);
            } else if(shape.name.find("Door_2") == 0) {
                door_position_rotations.push_back(pr);
                door_requirements.push_back(2);
            } else if(shape.name.find("Door_3") == 0) {
                door_position_rotations.push_back(pr);
                door_requirements.push_back(4);
            } else {
                not_recognized_error(shape.name);
            }
        } else if(shape.name.find("Light") == 0) {
            if(shape.name.length() < std::string("Light_000_000_000_000").length()) {
                not_recognized_error(shape.name);
            }
            char cfalloff[4];
            cfalloff[0] = shape.name[6];
            cfalloff[1] = shape.name[7];
            cfalloff[2] = shape.name[8];
            cfalloff[3] = '\0';
            char cr[4];
            cr[0] = shape.name[10];
            cr[1] = shape.name[11];
            cr[2] = shape.name[12];
            cr[3] = '\0';
            char cg[4];
            cg[0] = shape.name[14];
            cg[1] = shape.name[15];
            cg[2] = shape.name[16];
            cg[3] = '\0';
            char cb[4];
            cb[0] = shape.name[18];
            cb[1] = shape.name[19];
            cb[2] = shape.name[20];
            cb[3] = '\0';

            f32 falloff = (f32)atoi(cfalloff);
            f32 r = ((f32)atoi(cr))/100.0f;
            f32 g = ((f32)atoi(cg))/100.0f;
            f32 b = ((f32)atoi(cb))/100.0f;

            Light light = {};
            light.position_falloff = glm::vec4(pr.x, pr.y, pr.z, sqrtf(1.0f/falloff));
            light.color_alpha = glm::vec4(r, g, b, 1.0);
            lights.push_back(light);
        } else if(shape.name.find("Enemy") == 0) {
            if(shape.name.find("Enemy_0") == 0) {
                medium_positions.emplace_back(pr.x, pr.y, pr.z);
            } else if(shape.name.find("Enemy_1") == 0) {
                rat_positions.emplace_back(pr.x, pr.y, pr.z);
            } else if(shape.name.find("Enemy_2") == 0) {
                knight_positions.emplace_back(pr.x, pr.y, pr.z);
            } else {
                not_recognized_error(shape.name);
            }
        } else if(shape.name.find("Keycard") == 0) {
            //TODO(sean): if needed do some proper error checking here
            if(shape.name.find("Keycard_1") == 0) {
                keycard_count = std::max(keycard_count, 1u);
                keycard_positions[0] = glm::vec3(pr.x, pr.y, pr.z);
            } else if(shape.name.find("Keycard_2") == 0) {
                keycard_count = std::max(keycard_count, 2u);
                keycard_positions[1] = glm::vec3(pr.x, pr.y, pr.z);
            } else if(shape.name.find("Keycard_3") == 0) {
                keycard_count = std::max(keycard_count, 3u);
                keycard_positions[2] = glm::vec3(pr.x, pr.y, pr.z);
            } else if(shape.name.find("Keycard_4") == 0) {
                keycard_count = std::max(keycard_count, 4u);
                keycard_positions[3] = glm::vec3(pr.x, pr.y, pr.z);
            } else {
                not_recognized_error(shape.name);
            }
        } else if(shape.name.find("Physmesh") == 0) {
            physmesh_found = true;
            vertices = local_vertices;
            all_vertices = local_all_vertices;
            unique_vertices = local_unique_vertices;
            indices = local_indices;
        } else if(shape.name.find("Rendermesh") == 0) {
            printf("Found \"Rendermesh\", not doing anything.\n");
        } else if(shape.name.find("End_Zone") == 0) {
            end_zone_found = true;
            end_zone = glm::vec4(average_position, diff_x);
        } else if(shape.name.find("Start_Zone") == 0) {
            start_zone_found = true;
            start_zone = glm::vec4(average_position, diff_x);
        } else {
            not_recognized_error(shape.name);
        }
    }

    if(!physmesh_found) { throw "Physmesh not found!\n"; }
    if(!end_zone_found) { throw "End Zone not found!\n"; }
    if(!start_zone_found) { throw "Start Zone not found!\n"; }

    // load level mesh
    level_data->rendermesh_vertices = vertices;
    level_data->rendermesh_indices = indices;

    // load level physics mesh
    level_data->physmesh_vertices = std::vector<glm::vec3>();
    for(auto vertex: all_vertices) {
        level_data->physmesh_vertices.push_back(vertex.position);
    }

    // load door mesh
    level_data->door_vertices = door_vertices;
    level_data->door_indices = door_indices;

    level_data->door_positions_rotations = door_position_rotations;

    // load doors at positions
    u32 physmesh_index = level_data->physmesh_vertices.size();
    level_data->door_physmesh_range.push_back(physmesh_index);

    for(usize index = 0; index < door_position_rotations.size(); index += 1) {
        glm::vec4 pr = door_position_rotations[index];
        glm::vec3 p(pr.x, pr.y, pr.z);

        for (auto vertex: door_all_vertices) {
            f32 x = vertex.position.x;
            f32 y = vertex.position.y;
            f32 temp = y;
            if(pr.w > 0.5) {
                y = -x;//temp;
                x = temp;
            }

            level_data->physmesh_vertices.push_back(glm::vec3(x, y, vertex.position.z) + p);
        }

        physmesh_index = level_data->physmesh_vertices.size();
        level_data->door_physmesh_range.push_back(physmesh_index);
    }

    level_data->door_requirements = door_requirements;
    level_data->medium_positions = medium_positions;
    level_data->rat_positions = rat_positions;
    level_data->knight_positions = knight_positions;
    level_data->keycard_positions = keycard_positions;
    level_data->keycard_positions.resize(keycard_count);
    level_data->lights = lights;
    level_data->end_zone = end_zone;
    level_data->start_zone = start_zone;
}

int main() {
    std::ifstream fs;
    fs.open("../levels_list.txt");
    if(!fs.is_open()) {
        throw std::runtime_error("Failed to open \"levels_list.txt\" \n");
    }

    std::vector<std::string> file_names;
    std::string temp;
    while(std::getline(fs, temp)) {
        file_names.push_back(temp);
    }

    for(auto name: file_names) {
        std::string level_path = "../data/levels/", model_path = "../data/models/";
        level_path.append(name);
        level_path.append(".level");
        model_path.append(name);
        model_path.append(".obj");

        LevelData level_data;

        // equivalent c++ code
        load_model(
            model_path.c_str(),
            &level_data
        );

        u32 model_vertex_count = level_data.rendermesh_vertices.size();
        Vertex* model_vertices = level_data.rendermesh_vertices.data();

        u32 model_index_count = level_data.rendermesh_indices.size();
        u32* model_indices = level_data.rendermesh_indices.data();

        u32 door_vertex_count = level_data.door_vertices.size();
        Vertex* door_vertices = level_data.door_vertices.data();

        u32 door_index_count = level_data.door_indices.size();
        u32* door_indices = level_data.door_indices.data();

        u32 physmesh_vertex_count = level_data.physmesh_vertices.size();
        glm::vec3* physmesh_vertices = level_data.physmesh_vertices.data();

        u32 door_count = level_data.door_positions_rotations.size();
        glm::vec4* door_position_rotations = level_data.door_positions_rotations.data();

        u32* door_requirements = level_data.door_requirements.data();

        u32 door_physmesh_range_count = level_data.door_physmesh_range.size();
        u32* door_physmesh_ranges = level_data.door_physmesh_range.data();

        u32 medium_count = level_data.medium_positions.size();
        glm::vec3* medium_positions = level_data.medium_positions.data();

        u32 rat_count = level_data.rat_positions.size();
        glm::vec3* rat_positions = level_data.rat_positions.data();

        u32 knight_count = level_data.knight_positions.size();
        glm::vec3* knight_positions = level_data.knight_positions.data();

        u32 light_count = level_data.lights.size();
        Light* lights = level_data.lights.data();

        u32 keycard_count = level_data.keycard_positions.size();
        glm::vec3* keycard_positions = level_data.keycard_positions.data();

        {
            FILE *fp = fopen(level_path.c_str(), "wb");

            // level model
            fwrite(&model_vertex_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < model_vertex_count; index += 1) {
                fwrite(&model_vertices[index].position.x, sizeof(f32), 1, fp);
                fwrite(&model_vertices[index].position.y, sizeof(f32), 1, fp);
                fwrite(&model_vertices[index].position.z, sizeof(f32), 1, fp);

                fwrite(&model_vertices[index].normal.x, sizeof(f32), 1, fp);
                fwrite(&model_vertices[index].normal.y, sizeof(f32), 1, fp);
                fwrite(&model_vertices[index].normal.z, sizeof(f32), 1, fp);

                fwrite(&model_vertices[index].uv.x, sizeof(f32), 1, fp);
                fwrite(&model_vertices[index].uv.y, sizeof(f32), 1, fp);
            }

            printf("model_vertex_count: %d\n", model_vertex_count);

            fwrite(&model_index_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < model_index_count; index += 1) {
                fwrite(&model_indices[index], sizeof(u32), 1, fp);
            }

            printf("model_index_count: %d\n", model_index_count);

            // door model
            fwrite(&door_vertex_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < door_vertex_count; index += 1) {
                fwrite(&door_vertices[index].position.x, sizeof(f32), 1, fp);
                fwrite(&door_vertices[index].position.y, sizeof(f32), 1, fp);
                fwrite(&door_vertices[index].position.z, sizeof(f32), 1, fp);

                fwrite(&door_vertices[index].normal.x, sizeof(f32), 1, fp);
                fwrite(&door_vertices[index].normal.y, sizeof(f32), 1, fp);
                fwrite(&door_vertices[index].normal.z, sizeof(f32), 1, fp);

                fwrite(&door_vertices[index].uv.x, sizeof(f32), 1, fp);
                fwrite(&door_vertices[index].uv.y, sizeof(f32), 1, fp);
            }

            printf("door_vertex_count: %d\n", door_vertex_count);

            fwrite(&door_index_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < door_index_count; index += 1) {
                fwrite(&door_indices[index], sizeof(u32), 1, fp);
            }

            printf("door_index_count: %d\n", door_index_count);

            // physmesh
            fwrite(&physmesh_vertex_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < physmesh_vertex_count; index += 1) {
                fwrite(&physmesh_vertices[index].x, sizeof(f32), 1, fp);
                fwrite(&physmesh_vertices[index].y, sizeof(f32), 1, fp);
                fwrite(&physmesh_vertices[index].z, sizeof(f32), 1, fp);
            }

            printf("physmesh_vertex_count: %d\n", physmesh_vertex_count);

            // door prs
            fwrite(&door_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < door_count; index += 1) {
                fwrite(&door_position_rotations[index].x, sizeof(f32), 1, fp);
                fwrite(&door_position_rotations[index].y, sizeof(f32), 1, fp);
                fwrite(&door_position_rotations[index].z, sizeof(f32), 1, fp);
                fwrite(&door_position_rotations[index].w, sizeof(f32), 1, fp);
            }

            // door requirements
            for(usize index = 0; index < door_count; index += 1) {
                fwrite(&door_requirements[index], sizeof(u32), 1, fp);
            }

            // door physmesh ranges
            fwrite(&door_physmesh_range_count, sizeof(u32), 1, fp);
            for (usize index = 0; index < door_physmesh_range_count; index += 1) {
                fwrite(&door_physmesh_ranges[index], sizeof(u32), 1, fp);
            }

            // medium positions
            fwrite(&medium_count, sizeof(u32), 1, fp);
            for(usize index = 0; index < medium_count; index += 1) {
                fwrite(&medium_positions[index].x, sizeof(f32), 1, fp);
                fwrite(&medium_positions[index].y, sizeof(f32), 1, fp);
                fwrite(&medium_positions[index].z, sizeof(f32), 1, fp);
            }

            // rat positions
            fwrite(&rat_count, sizeof(u32), 1, fp);
            for(usize index = 0; index < rat_count; index += 1) {
                fwrite(&rat_positions[index].x, sizeof(f32), 1, fp);
                fwrite(&rat_positions[index].y, sizeof(f32), 1, fp);
                fwrite(&rat_positions[index].z, sizeof(f32), 1, fp);
            }

            // knight positions
            fwrite(&knight_count, sizeof(u32), 1, fp);
            for(usize index = 0; index < knight_count; index += 1) {
                fwrite(&knight_positions[index].x, sizeof(f32), 1, fp);
                fwrite(&knight_positions[index].y, sizeof(f32), 1, fp);
                fwrite(&knight_positions[index].z, sizeof(f32), 1, fp);
            }

            // keycard positions
            fwrite(&keycard_count, sizeof(u32), 1, fp);
            for(usize index = 0; index < keycard_count; index += 1) {
                fwrite(&keycard_positions[index].x, sizeof(f32), 1, fp);
                fwrite(&keycard_positions[index].y, sizeof(f32), 1, fp);
                fwrite(&keycard_positions[index].z, sizeof(f32), 1, fp);
            }

            // light position falloff color alphas
            fwrite(&light_count, sizeof(u32), 1, fp);
            for(usize index = 0; index < light_count; index += 1) {
                fwrite(&lights[index].position_falloff.x, sizeof(f32), 1, fp);
                fwrite(&lights[index].position_falloff.y, sizeof(f32), 1, fp);
                fwrite(&lights[index].position_falloff.z, sizeof(f32), 1, fp);
                fwrite(&lights[index].position_falloff.w, sizeof(f32), 1, fp);

                fwrite(&lights[index].color_alpha.x, sizeof(f32), 1, fp);
                fwrite(&lights[index].color_alpha.y, sizeof(f32), 1, fp);
                fwrite(&lights[index].color_alpha.z, sizeof(f32), 1, fp);
                fwrite(&lights[index].color_alpha.w, sizeof(f32), 1, fp);
            }

            fwrite(&level_data.end_zone.x, sizeof(f32), 1, fp);
            fwrite(&level_data.end_zone.y, sizeof(f32), 1, fp);
            fwrite(&level_data.end_zone.z, sizeof(f32), 1, fp);
            fwrite(&level_data.end_zone.w, sizeof(f32), 1, fp);

            fwrite(&level_data.start_zone.x, sizeof(f32), 1, fp);
            fwrite(&level_data.start_zone.y, sizeof(f32), 1, fp);
            fwrite(&level_data.start_zone.z, sizeof(f32), 1, fp);
            fwrite(&level_data.start_zone.w, sizeof(f32), 1, fp);

            fclose(fp);
        }
    }
}