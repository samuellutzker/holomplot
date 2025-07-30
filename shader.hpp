/*
 * File: shader.hpp
 * ----------------
 *
 * Defines a Shader class that handles loading and compiling of a vertex
 * and fragment shader. Provides an overloaded function to manipulate
 * the shader's uniforms (to be amended as needed).
 */

#pragma once
#include <fstream>
#ifdef __APPLE__
    // We need this to query for the MacOS app bundle directory
    #include <CoreFoundation/CoreFoundation.h>
#endif

class Shader {
    GLuint program;
    bool ready;
    std::string vertex_fname, frag_fname;

#ifdef __APPLE__
    // Get the directory of resource files in the MacOS app bundle
    std::string getResourcePath(const std::string& filename) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (!mainBundle) {
            std::cerr << "Failed to get main bundle\n";
            return "";
        }

        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        if (!resourcesURL) {
            std::cerr << "Failed to get resources directory\n";
            return "";
        }

        char path[PATH_MAX];
        if (!CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8*)path, PATH_MAX)) {
            std::cerr << "Failed to get path\n";
            CFRelease(resourcesURL);
            return "";
        }

        CFRelease(resourcesURL);

        return std::string(path) + "/" + filename;
    }
#else
    std::string getResourcePath(const std::string& filename) {
        return filename;
    }
#endif
    // Read a file and return a C std::string
    char *readFile(const std::string& filename) {
        std::ifstream file(getResourcePath(filename));
        file.seekg(0, file.end);
        int n = file.tellg();
        file.seekg(0, file.beg);
        char *buffer = new char [n+1];
        file.read(buffer, n);
        buffer[n] = '\0';
        return buffer;
    }

    // Return true if compilation succeeded. Output errors to STDERR.
    bool checkShaderStatus(const std::string& name="", GLuint obj=0) {
        GLint status;
        bool link = (name=="");

        if (link) glGetProgramiv(program, GL_LINK_STATUS, &status);
        else glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            char buffer[512];
            if (link) {
                glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
                std::cerr << "Linking errors:" << std::endl << buffer;
            } else {
                glGetShaderInfoLog(obj, sizeof(buffer), NULL, buffer);
                std::cerr << name << " compilation errors:" << std::endl << buffer << std::endl;
            }
            ready = false;
        }
        return ready;
    }

public:
    Shader(const std::string& vertex_fname, const std::string& frag_fname) : ready(true), vertex_fname(vertex_fname), frag_fname(frag_fname) {}

    void init() {
        char *buffer = readFile(vertex_fname);
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &buffer, NULL);
        delete [] buffer;

        buffer = readFile(frag_fname);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &buffer, NULL);
        delete [] buffer;

        glCompileShader(vertexShader);
        checkShaderStatus("Vertex shader", vertexShader);
        glCompileShader(fragmentShader);
        checkShaderStatus("Fragment shader", fragmentShader);

        if (!ready) return;

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glBindFragDataLocation(program, 0, "outColor");
        glLinkProgram(program);
        checkShaderStatus();

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use() const { if (ready) glUseProgram(program); }
    GLuint id() const { return program; }
    bool ok() const { return ready; }

    void uniform(const std::string& s, unsigned int v) const {
        glUniform1ui(glGetUniformLocation(program, s.c_str()), v);
    }
    void uniform(const std::string& s, int v) const {
        glUniform1i(glGetUniformLocation(program, s.c_str()), v);
    }
    void uniform(const std::string& s, float v) const {
        glUniform1f(glGetUniformLocation(program, s.c_str()), v);
    }
    void uniform(const std::string& s, glm::mat4 mat) const {
        glUniformMatrix4fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void uniform(const std::string& s, glm::mat3 mat) const {
        glUniformMatrix3fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void uniform(const std::string& s, glm::mat2 mat) const {
        glUniformMatrix2fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void uniform(const std::string& s, glm::vec3 v) const {
        glUniform3f(glGetUniformLocation(program, s.c_str()), v.x, v.y, v.z);
    }
    void uniform(const std::string& s, glm::vec2 v) const {
        glUniform2f(glGetUniformLocation(program, s.c_str()), v.x, v.y);
    }
};