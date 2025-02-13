/*  
 * Holomplot v1.0 (GL)
 * ===================
 *
 * A program for plotting expressions in holomorphic functions using OpenGL.
 *
 * Libraries
 * ---------
 * - wxWidgets 3.2.6  (GUI framework)
 * - GLEW 2.2.0       (OpenGL Extension Wrangler)
 * - GLM 0.9.9        (Mathematics library for OpenGL)
 * - TBB 2022.0.0     (Parallel processing)
 *
 * Usage
 * -----
 * - Enter an expression in the provided input field.
 * - Enter desired accuracy / resolution.
 * - Adjust camera position using mouse dragging and wheel.
 *
 * Example Expressions
 * -------------------
 * 1. atan(-10 + x^2 + y^2 / 5)
 * 2. 2sqrt(max(0,1-x^2/64-y^2/64)) * cos(sqrt(x^2+y^2))
 * 3. sin(ln(exp(z)))
 * 4. (sin(x^2 - y^2)) / (1 + sqrt(x^2 + y^2))
 * 5. sqrt(max(0,1-(sqrt(x^2+y^2)-2)^2))
 * 6. (1+i)(sqrt(max(0,1-(x+1)^2-y^2)) + sqrt(max(0,1-(x+1)^2*16-y^2*16))/8
 *       + sqrt(max(0,1-(x-1.1)^2-y^2)) + sqrt(max(0,1-(x-1.1)^2*16-y^2*16))/8)
 * 7. z^7 * exp(-abs(z)^2)
 *
 * 
 * File: window.cpp
 * ----------------
 * Implements the main application window and event handling.
 */


#include "window.h"
#include "canvas.h"

IMPLEMENT_APP(MyApp)

// "main()"
bool MyApp::OnInit()
{
    frame = new mainFrame("Holomorphic Function Plotter");
    frame->Show(true);

    return true;
} 

// Events for the Canvas class
BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
    EVT_MOUSE_EVENTS(Canvas::OnMouse)
    EVT_PAINT(Canvas::OnPaint)
    EVT_SIZE(Canvas::OnSize)
END_EVENT_TABLE()


// Events for the main window
BEGIN_EVENT_TABLE(mainFrame, wxFrame)
    EVT_BUTTON(ID_BTN_PLOT, mainFrame::OnButtonPlot)
    EVT_BUTTON(ID_BTN_CLEAR, mainFrame::OnButtonClear)
    EVT_CHECKBOX(ID_CB_STYLE, mainFrame::OnCheckBoxStyle)
    EVT_CHECKBOX(ID_CB_IMAG, mainFrame::OnCheckBoxImag)
    EVT_SPINCTRL(ID_SP_RES, mainFrame::OnSpinResolution)
    EVT_MENU(ID_MENU_LOG, mainFrame::OnMenuLog)
    EVT_MENU(wxID_ABOUT, mainFrame::OnMenuAbout)
    EVT_MENU(wxID_EXIT, mainFrame::OnMenuQuit)
END_EVENT_TABLE()

// Main constructor: Sets up the UI window and OpenGL canvas.
mainFrame::mainFrame(const wxString& title) 
: wxFrame(nullptr, wxID_ANY,  title, wxPoint(50,50), wxSize(1024, 768)) {

    // Log window
    logWin = new wxLogWindow(this, "Log", false, false);
    wxLog::SetActiveTarget(logWin);

    // Try to initialize OpenGL with platform defaults
    wxGLAttributes vAttrs;
    vAttrs.PlatformDefaults().Defaults().EndList();
    bool accepted = wxGLCanvas::IsDisplaySupported(vAttrs);

    if (!accepted)
    {
        // Try again without sample buffers
        vAttrs.Reset();
        vAttrs.PlatformDefaults().RGBA().DoubleBuffer().Depth(16).EndList();
        accepted = wxGLCanvas::IsDisplaySupported(vAttrs);

        if (!accepted)
        {
            wxMessageBox("Visual attributes for OpenGL are not accepted.\nThe app will exit now.",
                         "Error with OpenGL", wxOK | wxICON_ERROR);
            throw std::runtime_error("No canvas");
        }
    }

    // Create canvas
    canvas = new Canvas(this, vAttrs);
    canvas->SetMinSize(FromDIP(wxSize(640, 480)));

    // Menu
    wxMenu* fileMenu = new wxMenu;
    wxMenuBar* menuBar = new wxMenuBar;
    fileMenu->Append(wxID_ABOUT, "&About", "About the holomorphic 4D plotter");
    fileMenu->Append(ID_MENU_LOG, "&Log", "Show log window");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "&Quit", "Quit this app");
    menuBar->Append(fileMenu, "&File");
    SetMenuBar(menuBar);

    // Create sizers to arrange the controls
    wxBoxSizer* ctrlSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* opSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Enter expression");
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create controls and inputs
    inputExpr = new wxTextCtrl(this, ID_INP_EXPR, wxString(""));
    inputRes = new wxSpinCtrl(this, ID_SP_RES, wxString("100"));
    btnClear = new wxButton(this, ID_BTN_CLEAR, wxString("Reset"));
    btnPlot = new wxButton(this, ID_BTN_PLOT, wxString("Plot"));
    cbStyle = new wxCheckBox(this, ID_CB_STYLE, wxString("Grid Style"));
    cbImag = new wxCheckBox(this, ID_CB_IMAG, wxString("Imaginary Z"));

    // Structure the layout with the sizers
    opSizer->Add(inputExpr, 6, wxCENTER | wxALL, 5);
    opSizer->Add(btnPlot, 1, wxCENTER | wxALL, 5);
    opSizer->Add(btnClear, 1, wxCENTER | wxALL, 5);
    opSizer->Add(inputRes, 0, wxCENTER | wxALL, 5);
    opSizer->Add(cbStyle, 0, wxCENTER | wxALL, 5);
    opSizer->Add(cbImag, 0, wxCENTER | wxALL, 5);
    ctrlSizer->Add(opSizer, 12, wxEXPAND);
    mainSizer->Add(ctrlSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(canvas, 1, wxEXPAND);

    SetSizer(mainSizer);
    SetAutoLayout(true);
    SetMinSize(wxSize(250, 200));

    // Bind an event handler for reacting to the enter key 
    inputExpr->Bind(wxEVT_KEY_DOWN, &mainFrame::OnKeyPress, this);
    inputRes->Bind(wxEVT_KILL_FOCUS, &mainFrame::OnUnfocus, this);

    inputRes->SetRange(1, 400);
    inputRes->SetIncrement(10);
    canvas->setResolution(inputRes->GetValue());


    typedef std::complex<double> MyT; // We are parsing expressions in complex numbers

    // Assign custom defined 1-arg functions to the expression parser class
    Expr<MyT>::funcs1 = {
        {"sin", [](MyT x) { return sin(x); }}, 
        {"cos", [](MyT x) { return cos(x); }}, 
        {"log", [](MyT x) { return log(x); }}, 
        {"ln", [](MyT x) { return log(x); }}, 
        {"exp", [](MyT x) { return exp(x); }},
        {"sqrt", [](MyT x) { return sqrt(x); }},
        {"tan", [](MyT x) { return tan(x); }},
        {"atan", [](MyT x) { return atan(x); }},
        {"asin", [](MyT x) { return asin(x); }},
        {"acos", [](MyT x) { return acos(x); }},
        {"abs", [](MyT x) { return (MyT) abs(x); }},
        {"re", [](MyT x) { return (MyT) x.real(); }},
        {"im", [](MyT x) { return (MyT) x.imag(); }},
        {"conj", [](MyT x) { return conj(x); }},
    };

    // 2-arg functions. "Fake" max/min....
    Expr<MyT>::funcs2 = {
        {"max", [](MyT x, MyT y) { return x.real() > y.real() ? x : y; }}, 
        {"min", [](MyT x, MyT y) { return x.real() < y.real() ? x : y; }},
    };
}

mainFrame::~mainFrame() {}

// Takes the string in inputExpr and passes it to the canvas
void mainFrame::plotExpr() {
    try {
        // Get input string and transform to lowercase
        std::string s(inputExpr->GetValue());
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { 
                return std::tolower(c); 
            }
        );

        canvas->setExpression(s); // Pass the string to the canvas to evaluate

    } catch (std::invalid_argument& e) {
        wxMessageBox(e.what(),
            "Error in expression", wxOK | wxICON_INFORMATION, this);
    }
}

void mainFrame::OnKeyPress(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_RETURN)
        plotExpr();

    event.Skip();
}

void mainFrame::OnUnfocus(wxFocusEvent& event) {
    plotExpr();
    event.Skip();
}

void mainFrame::OnButtonPlot(wxCommandEvent& event) {
    plotExpr();
    event.Skip();
}

void mainFrame::OnButtonClear(wxCommandEvent& event) {
    canvas->reset();
    event.Skip();
}

void mainFrame::OnCheckBoxStyle(wxCommandEvent& event) {
    canvas->setGraphStyle(cbStyle->GetValue());
    event.Skip();
}

void mainFrame::OnCheckBoxImag(wxCommandEvent& event) {
    canvas->setGraphImag(cbImag->GetValue());
    event.Skip();
}

void mainFrame::OnSpinResolution(wxSpinEvent& event) {
    canvas->setResolution(inputRes->GetValue());
    event.Skip();
}

void mainFrame::OnMenuAbout(wxCommandEvent& event) {
    wxGenericMessageDialog dlg(nullptr,
        "Sam's OpenGL-powered holomorphic function plotter.\n"
        "Enter any expression using the complex variable z = x + i*y.\n"
        "By default, the Z-axis represents the real part of the function evaluation.",
        "About Plot4D",
        wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
}

void mainFrame::OnMenuLog(wxCommandEvent& event) {
    if (logWin->GetFrame()->IsIconized())
        logWin->GetFrame()->Restore();

    if (!logWin->GetFrame()->IsShown())
        logWin->Show();

    logWin->GetFrame()->Raise();
    logWin->GetFrame()->SetFocus();
}

void mainFrame::OnMenuQuit(wxCommandEvent& event) {
    Close(true);
}