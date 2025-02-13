/*
 * File: window.cpp
 * ----------------
 * Declares the main window and app classes.
 */

#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "wx/wx.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"

// Some IDs for wxWidgets elements
#define ID_INP_EXPR 10002
#define ID_BTN_PLOT 10003
#define ID_BTN_CLEAR 10004
#define ID_CB_STYLE 10005
#define ID_CB_IMAG 10006
#define ID_SP_RES 10007
#define ID_MENU_LOG 10008

class Canvas;

class mainFrame : public wxFrame {
    Canvas *canvas;
    wxButton *btnPlot, *btnClear;
    wxTextCtrl *inputExpr;
    wxSpinCtrl *inputRes;
    wxCheckBox *cbStyle, *cbImag;
    wxLogWindow *logWin;
    bool resChanged;

    void plotExpr();

    void OnButtonPlot(wxCommandEvent&);
    void OnButtonClear(wxCommandEvent&);
    void OnCheckBoxStyle(wxCommandEvent&);
    void OnCheckBoxImag(wxCommandEvent&);
    void OnSpinResolution(wxSpinEvent&);
    void OnKeyPress(wxKeyEvent&);
    void OnUnfocus(wxFocusEvent&);
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
