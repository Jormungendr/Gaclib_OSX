//
//  directcode_simplewindow.h
//  GacOSX
//
//  Created by Robert Bu on 12/18/14.
//  Copyright (c) 2014 Robert Bu. All rights reserved.
//

#ifndef GacOSX_directcode_simplewindow_h
#define GacOSX_directcode_simplewindow_h

#include "../shared/gac_include.h"

using namespace vl::presentation::description;


void RunSimpleWindowByReflection()
{
    GetTypeDescriptor<GuiWindow>(); // should be called due to VC++'s static library bug
    
    // auto currentTheme=ITheme::GetCurrentTheme();
    Value currentTheme=Value::InvokeStatic(L"presentation::theme::ITheme", L"GetCurrentTheme");
    // auto windowStyle=currentTheme->CreateWindowStyle();
    Value windowStyle=currentTheme.Invoke(L"CreateWindowStyle");
    // auto window=new GuiWindow(windowStyle);
    Value window=Value::Create(L"presentation::controls::GuiWindow", (Value_xs(), windowStyle));
    
    // auto clientSize=Size(320, 240);
    Value clientSize=Value::From(L"x:320 y:240", GetTypeDescriptor(L"presentation::Size"));
    // window->SetText(L"Window By Reflection!");
    window.SetProperty(L"Text", BoxValue<WString>(L"Window By Reflection!"));
    // window->SetClientSize(clientSize);
    window.SetProperty(L"ClientSize", clientSize);
    // window->GetContainerComposition()->SetPreferredMinSize(clientSize);
    window.GetProperty(L"ContainerComposition").SetProperty(L"PreferredMinSize", clientSize);
    // window->MoveToScreenCenter();
    window.Invoke(L"MoveToScreenCenter");
    
    // auto buttonStyle=currentTheme->CreateButtonStyle();
    Value buttonStyle=currentTheme.Invoke(L"CreateButtonStyle");
    // auto button=new GuiButton(buttonStyle);
    Value button=Value::Create(L"presentation::controls::GuiButton", (Value_xs(), buttonStyle));
    // button->GetBoundsComposition()->SetAlignmentToParent(Margin(60, 60, 60, 60));
    button.GetProperty(L"BoundsComposition").SetProperty(L"AlignmentToParent", Value::From(L"left:60 top:60 right:60 bottom:60", GetTypeDescriptor(L"presentation::Margin")));
    // button->SetText(L"Click Me!");
    button.SetProperty(L"Text", BoxValue<WString>(L"Click Me!"));
    // FontProperties font;
    // font.fontFamily=L"Segoe UI";
    // font.size=16;
    // font.bold=true;
    // button->SetFont(font);
    button.SetProperty(L"Font", Value::From(L"fontFamily:{Segoe UI} size:16 bold:true", GetTypeDescriptor(L"presentation::FontProperties")));
    // window->AddChild(button);
    window.Invoke(L"AddChild", (Value_xs(), button));
    
    // button->Clicked.AttachLambda([=](GuiGraphicsComposition* sender, GuiEventArgs& arguments)
    // {
    //      button->SetText(L"You clicked but button!");
    // });
    Value handler=BoxParameter<Func<Value(Value, Value)>>(
                                                          LAMBDA([&button](Value sender, Value arguments)
                                                                 {
                                                                     button.SetProperty(L"Text", BoxValue<WString>(L"You clicked the button!"));
                                                                     return Value();
                                                                 }));
    button.AttachEvent(L"Clicked", handler);
    
    // GetApplication()->Run(window);
    Value application=Value::InvokeStatic(L"presentation::controls::GuiApplication", L"GetApplication");
    application.Invoke(L"Run", (Value_xs(), window));
    // delete window;
    window.DeleteRawPtr();
}

int main(int argc, const char * argv[])
{
    SetupOSXCoreGraphicsRenderer();
    
    return 0;
}

void GuiMain()
{
    RunSimpleWindowByReflection();
}


#endif
