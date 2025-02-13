// window.h: Main window and app class.

#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "wx/wx.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"

class Canvas;

class mainFrame : public wxFrame {
    Canvas *canvas;
    wxButton *btnPlot, *btnClear;
    wxTextCtrl *inputExpr;
    wxSpinCtrl *inputRes;
    wxCheckBox *cbStyle, *cbImag;
    wxLogWindow *logWin;

    void OnButtonPlot(wxCommandEvent&);
    void OnButtonClear(wxCommandEvent&);
    void OnCheckBoxStyle(wxCommandEvent&);
    void OnCheckBoxImag(wxCommandEvent&);
    void OnSpinResolution(wxSpinEvent&);
    void OnKeyPress(wxKeyEvent&);
    void OnMenuAbout(wxCommandEvent&);
    void OnMenuQuit(wxCommandEvent&);
    void OnMenuLog(wxCommandEvent&);

public:

    mainFrame(const wxString& title);
    ~mainFrame();

    wxDECLARE_EVENT_TABLE();

    friend class Canvas;
};

class MyApp : public wxApp
{
    bool OnInit() wxOVERRIDE;
    
    mainFrame *frame;

public:
    MyApp() {}
};
