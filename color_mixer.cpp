#include <Application.h>
#include <Window.h>
#include <View.h>
#include <ColorControl.h>
#include <Slider.h>
#include <StringView.h>
#include <LayoutBuilder.h>
#include <GraphicsDefs.h>
#include <stdio.h>

enum { kMsgColorA = 'Acol', kMsgColorB = 'Bcol', kMsgBlend = 'mixr' };

class SwatchView : public BView {
public:
    SwatchView() : BView("swatch", B_WILL_DRAW), f{255,255,255,255} {
        SetExplicitMinSize(BSize(120,120));
    }
    void Set(rgb_color c) { f = c; Invalidate(); }
    void Draw(BRect) override {
        SetHighColor(f); FillRect(Bounds());
        SetHighColor(0,0,0); StrokeRect(Bounds());
    }
private: rgb_color f;
};

class MainWindow : public BWindow {
public:
    MainWindow()
    : BWindow(BRect(100,100,700,400), "Color Mixer",
              B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
    {
        fLeft  = new BColorControl(BPoint(0,0), B_CELLS_16x16, 8.0f, "left",  new BMessage(kMsgColorA));
        fRight = new BColorControl(BPoint(0,0), B_CELLS_16x16, 8.0f, "right", new BMessage(kMsgColorB));

        fBlend = new BSlider(BRect(0,0,0,0), "blend", "Blend A?B",
                             new BMessage(kMsgBlend), 0, 255);
        fBlend->SetModificationMessage(new BMessage(kMsgBlend));
        fBlend->SetLimitLabels("A","B");
        fBlend->SetValue(128);

        fOut = new SwatchView();
        fHex = new BStringView("hex", "#FFFFFF  rgb(255,255,255)");

        BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING)
            .AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
                .Add(fLeft, 1.0f)
                .Add(fRight, 1.0f)
            .End()
            .Add(fBlend)
            .Add(fOut)
            .Add(fHex);

        fLeft->SetValue(make_color(255,0,0));
        fRight->SetValue(make_color(0,0,255));
        UpdateMix();
    }

    void MessageReceived(BMessage* m) override {
        switch (m->what) {
            case kMsgColorA:
            case kMsgColorB:
            case kMsgBlend:
                UpdateMix(); break;
            default: BWindow::MessageReceived(m);
        }
    }

private:
    void UpdateMix() {
        rgb_color a = fLeft->ValueAsColor();
        rgb_color b = fRight->ValueAsColor();
        uint8 amt = (uint8)fBlend->Value();
        rgb_color mixed = mix_color(a, b, amt);

        fOut->Set(mixed);
        char buf[64];
        snprintf(buf, sizeof(buf), "#%02X%02X%02X  rgb(%d,%d,%d)",
                 mixed.red, mixed.green, mixed.blue,
                 mixed.red, mixed.green, mixed.blue);
        fHex->SetText(buf);
    }

    BColorControl *fLeft, *fRight;
    BSlider *fBlend;
    SwatchView *fOut;
    BStringView *fHex;
};

class App : public BApplication {
public:
    App(): BApplication("application/x-vnd.ptit-color-mixer") {}
    void ReadyToRun() override { (new MainWindow())->Show(); }
};

int main() { App app; app.Run(); }

