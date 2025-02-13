// shader.hpp:
// Defines a Shader class that handles loading and compiling of a vertex and fragment shader.
#pragma once
#include <fstream>
#ifdef __APPLE__
    #include <CoreFoundation/CoreFoundation.h>
#endif

using namespace std;

class Shader {
    GLuint program;
    bool ready;
    string vertex_fname, frag_fname;

#ifdef __APPLE__
    string getResourcePath(const string& filename) {
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (!mainBundle) {
            cerr << "Failed to get main bundle\n";
            return "";
        }

        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        if (!resourcesURL) {
            cerr << "Failed to get resources directory\n";
            return "";
        }

        char path[PATH_MAX];
        if (!CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8*)path, PATH_MAX)) {
            cerr << "Failed to get path\n";
            CFRelease(resourcesURL);
            return "";
        }

        CFRelease(resourcesURL);

        return string(path) + "/" + filename;
    }
#else
    string getResourcePath(const string& filename) {
        return filename;
    }
#endif

    char *readFile(const string& filename) {
        ifstream file(getResourcePath(filename));
        file.seekg(0, file.end);
        int n = file.tellg();
        file.seekg(0, file.beg);
        char *buffer = new char [n+1];
        file.read(buffer, n);
        buffer[n] = '\0';
        return buffer;
    }

    bool checkShaderStatus(const string& name="", GLuint obj=0) {
        GLint status;
        bool link = (name=="");

        if (link) glGetProgramiv(program, GL_LINK_STATUS, &status);
        else glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            char buffer[512];
            if (link) {
                glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
                cerr << "Linking errors:" << endl << buffer;
            } else {
                glGetShaderInfoLog(obj, sizeof(buffer), NULL, buffer);
                cerr << name << " compilation errors:" << endl << buffer << endl;
            }
            ready = false;
        }
        return ready;
    }

public:
    Shader(const string& vertex_fname, const string& frag_fname) : ready(true), vertex_fname(vertex_fname), frag_fname(frag_fname) {}

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

    template <class T> void uniform(const string& s, T v) const {
        glUniform1f(glGetUniformLocation(program, s.c_str()), v);
    }
    template <> void uniform<glm::mat4>(const string& s, glm::mat4 mat) const {
        glUniformMatrix4fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    template <> void uniform<glm::mat3>(const string& s, glm::mat3 mat) const {
        glUniformMatrix3fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    template <> void uniform<glm::mat2>(const string& s, glm::mat2 mat) const {
        glUniformMatrix2fv(glGetUniformLocation(program, s.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    template <> void uniform<glm::vec3>(const string& s, glm::vec3 v) const {
        glUniform3f(glGetUniformLocation(program, s.c_str()), v.x, v.y, v.z);
    }
    template <> void uniform<glm::vec2>(const string& s, glm::vec2 v) const {
        glUniform2f(glGetUniformLocation(program, s.c_str()), v.x, v.y);
    }
    template <> void uniform<int>(const string& s, int v) const {
        glUniform1i(glGetUniformLocation(program, s.c_str()), v);
    }
};