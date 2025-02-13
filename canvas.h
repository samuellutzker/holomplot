/*
 * File: canvas.h
 * --------------
 *
 * Declares a subclass Canvas from wxGLCanvas.
 * It handles the graphical visualization of an expression, 
 * and mouse events to control the camera.
 */

 #pragma once
#include <vector>
#include <cstdio>
#include <complex>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "window.h"
#include "wx/wx.h"
#include "wx/glcanvas.h"
#include "expr.hpp"
#include "shader.hpp"
#include "buffers.hpp"

class Canvas : public wxGLCanvas
{
    mainFrame *parent;      // Parent window
    wxSize scrSize;         // Screen size
    wxGLContext* oglCtx;    // OpenGL context

    Shader graphShader, labelShader;
    VertexArray graph, axis, label;
    Texture labelX, labelY, labelZ;

    // Expression to evaluate:
    std::string exprStr;
    Expr<std::complex<double> > expr;
    glm::vec3 camPos;   // Camera position
    glm::vec3 camDir;   // Camera direction
    glm::vec3 camUp;    // (0,0,1) projected onto the viewing plane
    glm::vec3 camRight; // crossproduct(dir, up)

    int scr_h, scr_w;       // Screen height, width
    int resolution;         // Grid resolution
    float theta, rho;       // Angles for camera rotation around origin
    float camDist;          // Zoom level
    float labelCX, labelCY; // Center of label

    wxPoint dragPos;
    bool needsRecalc;   // Need to call evalExpression
    bool isInitialized; // OpenGL ready flag
    bool gridWorld;     // Style of graph should be a grid
    bool imagWorld;     // z axis should be imaginary value
    bool isBusy;        // Calculation in progress

    float axisLength;
    float labelUnit;
    
    void setupIndices();
    void setupLabels();
    void refreshCam();  // Apply rotation of the cam
    void render(wxDC&); // Main drawing routine

    void calcGraph();   // Evaluate the expression and buffer GL data
    void initGL();

public:
    Canvas(mainFrame* parent, const wxGLAttributes& attrs);
    ~Canvas();

    void reset();
    
    // Event handlers:
    void OnPaint(wxPaintEvent&);    
    void OnMouse(wxMouseEvent&);
    void OnSize(wxSizeEvent&);

    void setExpression(const std::string&);
    void setGraphStyle(bool);
    void setGraphImag(bool);
    void setResolution(int res=0);

    wxDECLARE_EVENT_TABLE();
};