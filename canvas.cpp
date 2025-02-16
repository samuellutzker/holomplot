/*
 * File: canvas.cpp
 * ----------------
 * 
 * Defines a class Canvas derived from wxGLCanvas.
 * It handles the graphical visualization of an expression, 
 * and mouse events to control the camera.
 */

#include "canvas.h"
#include <tbb/parallel_for.h>

using std::complex;
using std::vector;
using std::map;
using std::max;
using std::min;
using std::string;

// Creates a monochrome bitmap from a text
unsigned char* renderText(const wxString& text, const wxFont& font, int* width, int* height) {
    const wxColour bgColor(*wxBLACK);
    const wxColour fgColor(*wxWHITE);
    wxMemoryDC dc;

    dc.SetFont(font);

    dc.GetMultiLineTextExtent(text, width, height); // Measure size

    wxBitmap bitmap(*width, *height);
    dc.SelectObject(bitmap);
    dc.SetBackground(wxBrush(bgColor));
    dc.Clear();
    dc.SetBackgroundMode(wxPENSTYLE_SOLID);
    dc.SetTextBackground(bgColor);
    dc.SetTextForeground(fgColor);
    dc.DrawText(text, 0, 0);
    dc.SelectObject(wxNullBitmap); // Detach

    unsigned char* data = bitmap.ConvertToImage().GetData();
    int size = *width * *height;
    unsigned char* out = new unsigned char [size];
    for (int i=0; i < size; ++i)
        out[i] = data[3*i]; // R channel is enough

    return out;
}


Canvas::Canvas(mainFrame* parent, const wxGLAttributes& canvasAttrs)
: wxGLCanvas(parent, canvasAttrs), parent(parent), resolution(50), scr_h(0), scr_w(0),
  graphShader("graph_vertex.glsl", "graph_frag.glsl"), labelShader("label_vertex.glsl", "label_frag.glsl"),
  gridWorld(false), imagWorld(false), isInitialized(false),  needsRecalc(false) {

    reset();

    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
    oglCtx = new wxGLContext(this, NULL, &ctxAttrs);

    if (!oglCtx->IsOK())
    {
        wxMessageBox("This program needs an OpenGL 3.3 capable driver.", "OpenGL error", wxOK | wxICON_INFORMATION, this);
        delete oglCtx;
        oglCtx = nullptr;
    }

    SetCurrent(*oglCtx);
}

Canvas::~Canvas() {
    delete oglCtx;
}

// Called by OnSize after the canvas is shown
void Canvas::initGL() {
    if (!oglCtx)
        return;

    SetCurrent(*oglCtx);

    if (glewInit() != GLEW_OK)
    {
        wxMessageBox("GLEW Initialization failed.", "OpenGL error", wxOK | wxICON_INFORMATION, this);
        return;
    }

    graphShader.init();
    labelShader.init();
    graph.init();
    axis.init();
    label.init();

    isInitialized = true;
    needsRecalc = true;

    setResolution(); // Calculate elements array

}

void Canvas::OnSize(wxSizeEvent& event) {
    event.Skip();

   bool firstApperance = IsShownOnScreen() && !isInitialized;

    if (firstApperance) {
        initGL();
    }

    if (isInitialized) {
        auto size = event.GetSize() * GetContentScaleFactor();
        scr_h = size.y;
        scr_w = size.x;
        glViewport(0, 0, max(1, (GLsizei)size.x), max(1, (GLsizei)size.y));
    }

    Refresh(false);
}

// Calculate the camera position with given angles and distance
void Canvas::refreshCam() {
    camPos = glm::vec3(camDist * cos(theta) * cos(rho), camDist * cos(theta) * sin(rho), camDist * sin(theta));
    Refresh(false);
}

// Clear the plot and move the cam to initial position
void Canvas::reset() {
    theta = 0.7f; // M_PI / 3.0f;
    rho = -1.8f; // 5.0f * M_PI / 12.0f;
    camDist = 15.0f;
    axisLength = 10.0f;
    exprStr = "0";
    expr = Expr<complex<double> >();
    needsRecalc = true;
    graph.clear();
    refreshCam();
}

// Drag the camera rotation
void Canvas::OnMouse(wxMouseEvent& event) {
    event.Skip();

    if (isBusy)
        return;

    if (event.LeftIsDown()) {
        wxPoint newPos = event.GetPosition();
        if (!event.Dragging()) {
            dragPos = newPos;
        } else {
            rho += (newPos.x - dragPos.x) * 0.01;
            theta += (newPos.y - dragPos.y) * 0.01;
            theta = max(min(theta, (float)M_PI * 0.48f), -(float)M_PI * 0.48f); // don't allow skipping
            dragPos = newPos;
            refreshCam();
        }
    }

    if (event.LeftUp()) {
        wxLogMessage("Cam @ radius=%.4g, theta=%.3g, rho=%.3g.", camDist, theta, rho);
    }

    const int maxRotation = 50;
    int rotation = max(-maxRotation, min(event.GetWheelRotation(), maxRotation));

    if (rotation != 0) {
        const double speed = .001;

        camDist += speed * camDist * rotation;
        // After a certain threshold we need to recalc the graph
        if (camDist < axisLength / 2.0) {
            axisLength /= 2.0;
            needsRecalc = true;
        } else if (camDist > axisLength * 2.0) {
            axisLength *= 4.0;
            needsRecalc = true;
        }
        refreshCam();
    }
}

// Main rendering routine
void Canvas::OnPaint(wxPaintEvent &WXUNUSED(event)) {
    wxPaintDC dc(this);

    if (!isInitialized)
        return;

    SetCurrent(*oglCtx);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CLIP_DISTANCE0); // Use this to trim extreme vertices
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    

    if (needsRecalc) {
        needsRecalc = false;

        auto start = std::chrono::high_resolution_clock::now();
    
        calcGraph();
    
        // Compute duration in microseconds
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
        wxLogMessage("------");
        wxLogMessage("Evaluated f(z)=%s.", exprStr);
        wxLogMessage("Processed %d evaluations.", resolution * resolution);
        wxLogMessage("Time elapsed: %d µs.", (int)duration.count());
    }

    graphShader.use();
    graph.use();

    // Light decay
    float dist = camDist + axisLength; // Far away
    graphShader.uniform("fLinear", 1.0f / dist);
    graphShader.uniform("fQuadratic", 1.0f / (dist * dist));
    
    // Graph color, my position, static color off, imaginary z axis
    graphShader.uniform("fColor", glm::vec3(1.0f, 0.0f, 0.0f));
    graphShader.uniform("camPos", camPos);
    graphShader.uniform("axisLength", axisLength);
    graphShader.uniform("staticColor", glm::vec3(0.0f, 0.0f, 0.0f));
    graphShader.uniform("staticColorMix", 0.0f);
    graphShader.uniform("zIsImag", (int)imagWorld);

    // z value of the (not normalized) normals 
    float resStep = 2.0f * axisLength / (resolution-1);
    graphShader.uniform("normZ", resStep * resStep);

    // MVP Matrices
    auto proj = glm::perspective(glm::radians(45.0f), (float)scr_w / scr_h, camDist * 0.01f, 5.0f * (axisLength + camDist));
    auto view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    graphShader.uniform("model", glm::mat4(1.0f)); // Static model
    graphShader.uniform("normal", glm::mat3(1.0f));
    graphShader.uniform("proj", proj);
    graphShader.uniform("view", view);

    // Surface
    if (!gridWorld) {
        graph.draw();
        graphShader.uniform("staticColorMix", 1.0f);
    }

    // Grid
    graph.draw(GL_LINES);

    // Labels
    glDepthFunc(GL_ALWAYS); // Draw over everything
    glEnable(GL_BLEND); // Blend text background
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    labelShader.use();    
    labelShader.uniform("proj", proj);
    labelShader.uniform("view", view);

    auto pos = proj * view * glm::vec4(labelUnit, 0.0f, 0.0f, 1.0f);
    glm::vec2 translate(pos / pos.z);
    glm::vec2 shift(translate.x * labelCX, translate.y * labelCY);
    labelX.use(labelShader);
    labelShader.uniform("translate", translate + shift);
    label.draw(GL_TRIANGLES);

    pos = proj * view * glm::vec4(0.0f, labelUnit, 0.0f, 1.0f);
    translate = glm::vec2(pos / pos.z);
    shift = glm::vec2(translate.x * labelCX, translate.y * labelCY);
    labelY.use(labelShader);
    labelShader.uniform("translate", translate + shift);
    label.draw(GL_TRIANGLES);

    glDisable(GL_BLEND);

    // Axis
    
    glDepthMask(GL_FALSE);
    graphShader.use();
    graphShader.uniform("staticColorMix", 1.0f);
    // In front of graph
    glDepthFunc(GL_LEQUAL);
    graphShader.uniform("staticColor", glm::vec3(1.0f, 0.0f, 0.0f));
    axis.draw(GL_LINES);
    // Behind graph
    glDepthFunc(GL_GREATER);
    graphShader.uniform("staticColor", glm::vec3(0.4f, 0.0f, 0.0f));
    axis.draw(GL_LINES);


    SwapBuffers();
}

// Change resolution, refill indices array and create new axis labels
void Canvas::setResolution(int res) {
    if (res)
        resolution = res+1;

    if (!isInitialized)
        return;

    setupIndices();
}

void Canvas::setupLabels() {
    map<string,vector<vector<float> > > buf;
    char s[20];

    wxFont font(wxFontInfo(48).Family(wxFONTFAMILY_MODERN));
    if (!font.IsOk())
        font = *wxSWISS_FONT;
    font.SetPointSize(24);

    labelUnit = axisLength / 2.0f;

    Texture::Image img;
    sprintf(s, " %.4g ", labelUnit);
    img.data = renderText(s, font, &img.width, &img.height);
    labelX.buffer({{ "tex", img }}, GL_LINEAR, GL_LINEAR, GL_RED);

    sprintf(s, " %.4gi", labelUnit);
    img.data = renderText(s, font, &img.width, &img.height);
    labelY.buffer({{ "tex", img }}, GL_LINEAR, GL_LINEAR, GL_RED);

    const float size = 0.02f;
    labelCX = size * img.width / img.height;
    labelCY = size;

    buf["vPos"] = {
        {-labelCX, -labelCY}, 
        {labelCX, -labelCY}, 
        {labelCX, labelCY}, 
        {-labelCX, labelCY}, 
    };

    buf["vTex"] = {
        {0.0f, 1.0f}, 
        {1.0f, 1.0f}, 
        {1.0f, 0.0f}, 
        {0.0f, 0.0f}, 
    };

    label.buffer(buf, labelShader);
    label.elements({ 0, 1, 2, 0, 2, 3 });

    // Axis
    buf.clear();
    buf["vPos"] = {};
    buf["vNorm"] = {};

    buf["vPos"].push_back({ axisLength / 2.0f, 0.0f, 0.0f, 0.0f });
    buf["vPos"].push_back({ 0.0f, 0.0f, 0.0f, 0.0f });
    buf["vPos"].push_back({ 0.0f, axisLength / 2.0f, 0.0f, 0.0f });
    buf["vPos"].push_back({ 0.0f, 0.0f, 0.0f, 0.0f });
    // buf["vPos"].push_back({ 0.0f, 0.0f, axisLength, axisLength });
    // buf["vPos"].push_back({ 0.0f, 0.0f, 0.0f, 0.0f });

    buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });
    buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });
    buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });
    buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });
    // buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });
    // buf["vNorm"].push_back({ 0.0f, 0.0f, 0.0f });

    axis.buffer(buf, graphShader);
}

// Fill up the elements buffer
void Canvas::setupIndices() {
    // Indices to draw
    auto idx = [&](int i, int j) { return i + j*resolution; };
    vector<int> indices;
    for (int j=0; j < resolution-1; ++j) {
        for (int i=0; i < resolution-1; ++i) {
            indices.push_back(idx(i, j));
            indices.push_back(idx(i+1, j));
            indices.push_back(idx(i, j+1));

            indices.push_back(idx(i, j+1));
            indices.push_back(idx(i+1, j));
            indices.push_back(idx(i+1, j+1));
        }
    }

    graph.elements(indices);
    needsRecalc = true;
}

// Receive a new expression to plot
void Canvas::setExpression(const string& str) {
    Expr<complex<double> > newExpr(str);

    // Test expression (all variables assigned?),
    // throws invalid_argument if not.
    newExpr({
        {"x", complex<double>(0.0)},
        {"y", complex<double>(0.0)},
        {"z", complex<double>(0.0)},
        {"i", complex<double>(0.0, 1.0)},
        {"e", complex<double>(exp(1.0f), 0.0)},
        {"pi", complex<double>((float)M_PI, 0.0)},
    });

    expr = newExpr;
    exprStr = str;
    needsRecalc = true;
    Refresh(false);
}

// Minimalistic grid view on/off
void Canvas::setGraphStyle(bool grid) {
    gridWorld = grid;
    Refresh(false);
}

// Imaginary z-Axis on/off
void Canvas::setGraphImag(bool imag) {
    imagWorld = imag;
    Refresh(false);
}

// Fill up the graph VertexArray and calculate normals
void Canvas::calcGraph() {
    isBusy = true; // Lock mouse events

    map<string,vector<vector<float> > > buf;
    buf["vPos"] = vector<vector<float> >(resolution * resolution);
    buf["vNorm"] = vector<vector<float> >(resolution * resolution);

    // Evaluate function using parallel processing
    tbb::parallel_for(size_t(0), buf["vPos"].size(), [&](size_t index) {
        float x = -axisLength + 2.0f * (index % resolution) * axisLength / (resolution-1);
        float y = -axisLength + 2.0f * (index / resolution) * axisLength / (resolution-1);

        complex<double> z = expr({
            {"x", complex<double>(x)},
            {"y", complex<double>(y)},
            {"z", complex<double>(x, y)},
            {"i", complex<double>(0.0, 1.0)},
            {"e", complex<double>(exp(1.0), 0.0)},
            {"pi", complex<double>(M_PI, 0.0)},
        });
        // Real and complex part of the function value goes to the shader
        buf["vPos"][index] = { x, y, (float)z.real(), (float)z.imag() };
    });

    // Normals:
    
    // Get vPos at index as vec4
    auto getVec = [&](int index) {
        return glm::vec4(buf["vPos"][index][0], buf["vPos"][index][1], buf["vPos"][index][2], buf["vPos"][index][3]);
    };

    // Simultaenous "cross products" for the first two components of normals at (x,y,re(z)) and (x,y,im(z))
    auto cross = [&](const glm::vec4& a, const glm::vec4& b, const glm::vec4& c) {
        glm::vec4 d = a-c, e = b-c;
        return glm::vec4(d.y * e.z - d.z * e.y, d.z * e.x - d.x * e.z, d.y * e.w - d.w * e.y, d.w * e.x - d.x * e.w);
    };

    // Calculate normals using parallel processing
    tbb::parallel_for(size_t(0), buf["vNorm"].size(), [&](size_t index) {
        int i = index % resolution, j = index / resolution;

        glm::vec4 me(getVec(index));
        glm::vec4 left, top, bottom, right, norm(0.0f);

        if (i > 0)
            left = glm::vec4(getVec(index-1));
        if (j > 0)
            top = glm::vec4(getVec(index-resolution));
        if (i < resolution-1)
            right = glm::vec4(getVec(index+1));
        if (j < resolution-1)
            bottom = glm::vec4(getVec(index+resolution));

        int k=0;
        if (i>0 && j>0) { norm = cross(left, top, me); ++k; }
        if (i>0 && j<resolution-1) { norm += cross(bottom, left, me); ++k; }
        if (i<resolution-1 && j>0) { norm += cross(top, right, me); ++k; }
        if (i<resolution-1 && j<resolution-1) { norm += cross(right, bottom, me); ++k; }
        norm /= (float)k;
        // First two components of normals at (x,y,re(z)) and (x,y,im(z)). Third is uniform normZ.
        buf["vNorm"][index] = { norm.x, norm.y, norm.z, norm.w };
    });

    graph.buffer(buf, graphShader);

    setupLabels();

    isBusy = false; // Unlock mouse events
}