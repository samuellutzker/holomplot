//
//      Holomplot v1.0 (GL)
//
// Plot expressions in holomorphic functions.
// This program uses the following libraries:
// - wxWidgets (3.2.6)
// - GLEW (2.2.0)
// - GLM (0.9.9)
// - TBB (2022.0.0)

// window.cpp: Main window and app class.

// Some examples to test it on:
//
// atan(-10+x^2+y^2/5)
// 2sqrt(max(0,1-x^2/64-y^2/64))*cos(sqrt(x^2+y^2))
// (1+i)(sqrt(max(0,1-(x+1)^2-y^2))+sqrt(max(0,1-(x+1)^2*16-y^2*16))/8+sqrt(max(0,1-(x-1.1)^2-y^2))+sqrt(max(0,1-(x-1.1)^2*16-y^2*16))/8)
// (sin(x^2-y^2))/(1+sqrt(x^2+y^2))
// sqrt(max(0,1-(sqrt(x^2+y^2)-2)^2))
// sin(ln(exp(z)))
// z^7*exp(-abs(z)^2)


#include "window.h"
#include "canvas.h"

IMPLEMENT_APP(MyApp)

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
    EVT_BUTTON(10003, mainFrame::OnButtonPlot)
    EVT_BUTTON(10004, mainFrame::OnButtonClear)
    EVT_CHECKBOX(10005, mainFrame::OnCheckBoxStyle)
    EVT_SPINCTRL(10006, mainFrame::OnSpinResolution)
    EVT_CHECKBOX(10007, mainFrame::OnCheckBoxImag)
    EVT_MENU(10008, mainFrame::OnMenuLog) // wxID_ABOUT // bad behaviour on mac
    EVT_MENU(wxID_ABOUT, mainFrame::OnMenuAbout) // wxID_ABOUT // bad behaviour on mac
    EVT_MENU(wxID_EXIT, mainFrame::OnMenuQuit) // wxID_EXIT // bad behaviour on mac
END_EVENT_TABLE()

mainFrame::mainFrame(const wxString& title) 
: wxFrame(nullptr, wxID_ANY,  title, wxPoint(50,50), wxSize(1024, 768)) {

    // Log window
    logWin = new wxLogWindow(this, "Log", false, false);
    wxLog::SetActiveTarget(logWin);

    // The canvas
    canvas = NULL;
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
        }
    }

    if (accepted) {
        canvas = new Canvas(this, vAttrs);
        canvas->SetMinSize(FromDIP(wxSize(640, 480)));
    } else
        throw std::runtime_error("No canvas");

    // Menu
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_ABOUT, "&About", "About the holomorphic 4D plotter");
    fileMenu->Append(10008, "&Log", "Show log window");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "&Quit", "Quit this app");
    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");
    SetMenuBar(menuBar);

    // Sizers
    wxBoxSizer* ctrlSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* opSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Enter expression");
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Controls and inputs
    inputExpr = new wxTextCtrl(this, 10002, wxString(""));
    inputRes = new wxSpinCtrl(this, 10006, wxString("100"));
    btnClear = new wxButton(this, 10004, wxString("Reset"));
    btnPlot = new wxButton(this, 10003, wxString("Plot"));
    cbStyle = new wxCheckBox(this, 10005, wxString("Grid Style"));
    cbImag = new wxCheckBox(this, 10007, wxString("Imaginary Z"));

    opSizer->Add(inputExpr, 6, wxCENTER | wxALL, 5);
    opSizer->Add(btnPlot, 1, wxCENTER | wxALL, 5);
    opSizer->Add(btnClear, 1, wxCENTER | wxALL, 5);
    opSizer->Add(inputRes, 0, wxCENTER | wxALL, 5);
    opSizer->Add(cbStyle, 0, wxCENTER | wxALL, 5);
    opSizer->Add(cbImag, 0, wxCENTER | wxALL, 5);

    inputExpr->Bind(wxEVT_KEY_DOWN, &mainFrame::OnKeyPress, this);

    ctrlSizer->Add(opSizer, 12, wxEXPAND);
    mainSizer->Add(ctrlSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(canvas, 1, wxEXPAND);

    SetSizer(mainSizer);
    SetAutoLayout(true);
    SetMinSize(wxSize(250, 200));

    inputRes->SetRange(2, 300);
    inputRes->SetIncrement(10);
    canvas->setResolution(inputRes->GetValue());

    typedef complex<double> MyT;

    // Custom defined functions for Expr<complex>
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

    // "Fake" max/min....
    Expr<MyT>::funcs2 = {
        {"max", [](MyT x, MyT y) { return x.real() > y.real() ? x : y; }}, 
        {"min", [](MyT x, MyT y) { return x.real() < y.real() ? x : y; }},
    };
}

mainFrame::~mainFrame() {}

void mainFrame::OnKeyPress(wxKeyEvent& evt) {
    if (evt.GetKeyCode() == WXK_RETURN) {
        wxCommandEvent event(wxEVT_BUTTON, btnPlot->GetId());
        wxPostEvent(btnPlot, event);
    }
    evt.Skip(true);
}

void mainFrame::OnButtonPlot(wxCommandEvent& evt) {
    try {
        string s(inputExpr->GetValue());
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::tolower(c); });
        canvas->setExpression(s);
    } catch (invalid_argument& e) {
        wxMessageBox(e.what(),
            "Error in expression", wxOK | wxICON_INFORMATION, this);
    }
    evt.Skip();
}

void mainFrame::OnButtonClear(wxCommandEvent& evt) {
    canvas->reset();
    evt.Skip();
}

void mainFrame::OnCheckBoxStyle(wxCommandEvent& evt) {
    canvas->setGraphStyle(cbStyle->GetValue());
    evt.Skip();
}

void mainFrame::OnCheckBoxImag(wxCommandEvent& evt) {
    canvas->setGraphImag(cbImag->GetValue());
    evt.Skip();
}

void mainFrame::OnSpinResolution(wxSpinEvent& evt) {
    canvas->setResolution(inputRes->GetValue());
    evt.Skip();
}

void mainFrame::OnMenuAbout(wxCommandEvent& evt) {
    wxGenericMessageDialog dlg(nullptr,
        "Sam's OpenGL-powered holomorphic function plotter.\n"
        "Enter any expression using the complex variable z = x + i*y.\n"
        "By default, the Z-axis represents the real part of the function evaluation.",
        "About Plot4D",
        wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
}

void mainFrame::OnMenuLog(wxCommandEvent& evt) {
    if (logWin->GetFrame()->IsIconized())
        logWin->GetFrame()->Restore();

    if (!logWin->GetFrame()->IsShown())
        logWin->Show();

    logWin->GetFrame()->Raise();
    logWin->GetFrame()->SetFocus();
}

void mainFrame::OnMenuQuit(wxCommandEvent& evt) {
    Close(true);
}