/* 
 * File: buffers.hpp
 * -----------------
 *
 * Defines OpenGL-related classes for buffering and binding.
 * class Texture uploads and handles a given number of textures.
 * class VertexArray is responsible for one VAO and handles buffering of data.
 */

#pragma once
#include "shader.hpp"

class Texture {
    GLuint *texId;
    int n;
    std::vector<std::string> uniforms;

public:
    struct Image {
        unsigned char *data;
        int width;
        int height;
    };

    ~Texture() {
        delete[] texId;
    }

    Texture() : texId(nullptr) {}

    void buffer(const std::map<std::string, Image>& textures, GLint minFilter=GL_LINEAR_MIPMAP_LINEAR, GLint magFilter=GL_LINEAR, GLint format=GL_RGBA) {
        n = textures.size();
        delete[] texId;
        texId = new GLuint[n];

        glGenTextures(n, texId);

        int index=0;
        for (auto &tex : textures) {
            uniforms.push_back(tex.first);

            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(GL_TEXTURE_2D, texId[index]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
            if (format != GL_RGBA)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
            glTexImage2D(GL_TEXTURE_2D, 0, format, tex.second.width, tex.second.height, 0, format, GL_UNSIGNED_BYTE, tex.second.data);
            glGenerateMipmap(GL_TEXTURE_2D);
            ++index;
        }
    }

    void use(const Shader& shader) {
        shader.use();
        for (int i=0; i < n; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, texId[i]);
            shader.uniform(uniforms[i], texId[i]);
        }
    }
};

class VertexArray {
    std::map<std::string,GLuint> attribs;
    GLuint *vbo, ebo, vao;
    GLuint num_vertices;
    int num_buffers;
    inline static VertexArray* current=nullptr;

public:
    VertexArray(int num_buffers=1) : num_buffers(num_buffers), num_vertices(0), ebo(0) {
        vbo = new GLuint[num_buffers]{0};
    }

    ~VertexArray() {
        clear();
        delete[] vbo;
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
    }

    void init() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(num_buffers, vbo);
    }

    void use() {
        if (current == this) return;
        current = this;
        glBindVertexArray(vao);
    }

    void clear(int buffer=-1) {
        if (buffer < 0) {
            for (int i=0; i < num_buffers; ++i)
                if (vbo[i]) clear(i);
            return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo[buffer]);
        glDeleteBuffers(num_buffers, &vbo[buffer]);
        vbo[buffer] = 0;
    }

    void elements(const std::vector<int>& indices) {
        use();
        if (!ebo) {
            glGenBuffers(1, &ebo);
        }
        num_vertices = indices.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_vertices * sizeof(int), indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        current = nullptr;
    }

    void buffer(const std::map<std::string,std::vector<std::vector<float> > >& data, const Shader& shader, int buffer=0) {
        use();
        GLuint m = 0;
        GLuint n = data.begin()->second.size();

        for (const auto& d : data) {
            m += d.second[0].size();
            if (d.second.size() != n) throw std::invalid_argument("dimensions inconsistent.");
        }
        
        float *vertices = new float [m * n];

        for (int i=0; i < n; ++i) {
            int j=0;
            for (const auto& d : data) {
                int l;
                if ((l=d.second[i].size()) != d.second[0].size()) throw std::invalid_argument("dimensions inconsistent.");
                for (int k=0; k < l; ++k)
                    vertices[i*m + j+k] = d.second[i][k];
                j += l;
            }
        }

        if (!vbo[buffer]) {
            glGenBuffers(1, &vbo[buffer]);
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo[buffer]);
        glBufferData(GL_ARRAY_BUFFER, m * n * sizeof(float), vertices, GL_STATIC_DRAW);

        delete [] vertices;

        // assign attribs to locations in the map
        std::map<std::string, GLuint> attribs;
        size_t offset = 0;
        for (const auto& d : data) {
            attribs[d.first] = glGetAttribLocation(shader.id(), d.first.c_str());
            glVertexAttribPointer(attribs[d.first], d.second[0].size(), GL_FLOAT, GL_FALSE, m*sizeof(float), (void*) offset);
            glEnableVertexAttribArray(attribs[d.first]);
            offset += d.second[0].size() * sizeof(float);
        }
        if (!ebo) {
            num_vertices = n;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw(GLenum mode=GL_TRIANGLES) {
        use();
        if (ebo) {
            glDrawElements(mode, num_vertices, GL_UNSIGNED_INT, 0);
        } else if (num_vertices > 0) {
            glDrawArrays(mode, 0, num_vertices);
        }
    }
};