#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <RadioButton.h>
#include <Slider.h>
#include <LayoutBuilder.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <Alert.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>

enum {
    kMsgCellClick   = 'CELL',
    kMsgShuffle     = 'SHUF',
    kMsgNewGame     = 'NEWG',
    kMsgSize3       = 'SIZ3',
    kMsgSize4       = 'SIZ4',
    kMsgSize5       = 'SIZ5',
    kMsgShuffleStep = 'STEP'
};

class CellButton : public BButton {
public:
    CellButton(int r, int c, BHandler* target)
        : BButton("", new BMessage(kMsgCellClick)), _r(r), _c(c) {
        Message()->AddInt32("r", r);
        Message()->AddInt32("c", c);
        SetTarget(target);
        SetExplicitMinSize(BSize(48, 48));
        SetExplicitPreferredSize(BSize(56, 56));
        SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
    }
private:
    int _r, _c;
};

class PuzzleWindow : public BWindow {
public:
    PuzzleWindow()
    : BWindow(BRect(100,100,900,660), "Haiku N-Puzzle Pro", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
    , _N(4), _moves(0), _shuffleSteps(200) {
        _menubar = new BMenuBar(BRect(0,0,1,1), "menubar");
        BMenu* mSize = new BMenu("Size");
        mSize->AddItem(new BMenuItem("3 x 3", new BMessage(kMsgSize3), '3'));
        mSize->AddItem(new BMenuItem("4 x 4", new BMessage(kMsgSize4), '4'));
        mSize->AddItem(new BMenuItem("5 x 5", new BMessage(kMsgSize5), '5'));
        _menubar->AddItem(mSize);

        _btnNew     = new BButton("New",     new BMessage(kMsgNewGame));
        _btnShuffle = new BButton("Shuffle", new BMessage(kMsgShuffle));
        _status     = new BStringView("status", "Moves: 0");
        _status->SetExplicitMinSize(BSize(120, B_SIZE_UNSET));

        _size3 = new BRadioButton(BRect(0,0,1,1), "size3", "3 x 3", new BMessage(kMsgSize3));
        _size4 = new BRadioButton(BRect(0,0,1,1), "size4", "4 x 4", new BMessage(kMsgSize4));
        _size5 = new BRadioButton(BRect(0,0,1,1), "size5", "5 x 5", new BMessage(kMsgSize5));
        _size4->SetValue(B_CONTROL_ON);

        _slider = new BSlider(BRect(0,0,200,40), "shuffleSlider", "Shuffle steps",
                              new BMessage(kMsgShuffleStep), 20, 1000, B_HORIZONTAL, B_BLOCK_THUMB);
        _slider->SetModificationMessage(new BMessage(kMsgShuffleStep));
        _slider->SetValue(_shuffleSteps);
        _slider->SetLimitLabels("20", "1000");

        _btnNew->SetTarget(this);
        _btnShuffle->SetTarget(this);
        _size3->SetTarget(this);
        _size4->SetTarget(this);
        _size5->SetTarget(this);
        _slider->SetTarget(this);

        _gridHost = new BView("gridHost", B_WILL_DRAW);
        _grid = new BGridLayout();
        _gridHost->SetLayout(_grid);

        BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING)
            .Add(_menubar)
            .AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
                .Add(_btnNew)
                .Add(_btnShuffle)
                .AddStrut(10)
                .Add(_size3)
                .Add(_size4)
                .Add(_size5)
                .AddStrut(10)
                .Add(_slider, 1.0f)
                .AddStrut(10)
                .Add(_status)
            .End()
            .AddStrut(4)
            .Add(_gridHost, 1.0f);

        NewGame();
        Show();
    }

    void MessageReceived(BMessage* msg) override {
        switch (msg->what) {
            case kMsgNewGame: NewGame(); break;
            case kMsgShuffle: Shuffle(_slider->Value()); break;
            case kMsgSize3: SetSize(3); break;
            case kMsgSize4: SetSize(4); break;
            case kMsgSize5: SetSize(5); break;
            case kMsgShuffleStep: _shuffleSteps = _slider->Value(); break;
            case kMsgCellClick: {
                int32 r, c;
                if (msg->FindInt32("r", &r) == B_OK && msg->FindInt32("c", &c) == B_OK) TryMove(r, c);
                break;
            }
            case B_KEY_DOWN: {
                const char* bytes = nullptr;
                if (msg->FindString("bytes", &bytes) == B_OK && bytes) {
                    if (bytes[0]=='3') SetSize(3);
                    else if (bytes[0]=='4') SetSize(4);
                    else if (bytes[0]=='5') SetSize(5);
                }
                break;
            }
            default: BWindow::MessageReceived(msg);
        }
    }

private:
    void SetSize(int n) {
        if (_N == n) return;
        _N = n;
        if (n == 3) _size3->SetValue(B_CONTROL_ON);
        if (n == 4) _size4->SetValue(B_CONTROL_ON);
        if (n == 5) _size5->SetValue(B_CONTROL_ON);
        BuildGrid();
        NewGame();
    }
    void NewGame() {
        _board.assign(_N * _N, 0);
        for (int i=0;i<_N*_N-1;i++) _board[i] = i+1;
        _board[_N*_N-1] = 0;
        _moves = 0;
        UpdateButtons();
        UpdateStatus();
        Shuffle(_shuffleSteps);
    }
    void Shuffle(int steps) {
        std::mt19937 rng{std::random_device{}()};
        for (int s=0; s<steps; ++s) {
            auto z = FindZero();
            int zr=z.first, zc=z.second;
            std::vector<std::pair<int,int>> neigh;
            if (zr>0)        neigh.push_back({zr-1, zc});
            if (zr<_N-1)     neigh.push_back({zr+1, zc});
            if (zc>0)        neigh.push_back({zr, zc-1});
            if (zc<_N-1)     neigh.push_back({zr, zc+1});
            auto pick = neigh[rng()%neigh.size()];
            Swap({zr,zc}, pick);
        }
        _moves = 0;
        UpdateButtons();
        UpdateStatus();
    }
    void TryMove(int r, int c) {
        auto z = FindZero();
        int zr=z.first, zc=z.second;
        if ((abs(zr-r)+abs(zc-c)) == 1) {
            Swap({zr,zc}, {r,c});
            _moves++;
            UpdateButtons();
            UpdateStatus();
            if (IsSolved()) {
                BAlert* ok = new BAlert("Solved", "Hail your return, O Exalted Sovereign!", "OK");
                ok->Go();
            }
        }
    }
    bool IsSolved() const {
        for (int i=0;i<_N*_N-1;i++) if (_board[i] != i+1) return false;
        return _board.back() == 0;
    }
    std::pair<int,int> FindZero() const {
        int pos = 0;
        for (; pos < (int)_board.size(); ++pos) if (_board[pos]==0) break;
        return { pos/_N, pos%_N };
    }
    void Swap(std::pair<int,int> a, std::pair<int,int> b) {
        int ia = a.first*_N + a.second;
        int ib = b.first*_N + b.second;
        std::swap(_board[ia], _board[ib]);
    }
    void UpdateStatus() {
        char buf[64]; snprintf(buf, sizeof(buf), "Moves: %d", _moves);
        _status->SetText(buf);
    }
    void ClearGrid() {
        for (int i = _grid->CountItems()-1; i>=0; --i) _grid->RemoveItem(i);
        _cells.clear();
    }
    void BuildGrid() {
        ClearGrid();
        _cells.resize(_N*_N);
        for (int r=0; r<_N; ++r) {
            for (int c=0; c<_N; ++c) {
                int idx = r*_N + c;
                _cells[idx] = new CellButton(r, c, this);
                _grid->AddView(_cells[idx], c, r);
            }
        }
        _gridHost->InvalidateLayout();
    }
    void UpdateButtons() {
        if ((int)_cells.size() != _N*_N) BuildGrid();
        for (int r=0;r<_N;r++){
            for (int c=0;c<_N;c++){
                int idx=r*_N+c;
                int val=_board[idx];
                if (val==0){
                    _cells[idx]->SetLabel("");
                    _cells[idx]->SetEnabled(false);
                } else {
                    _cells[idx]->SetEnabled(true);
                    char lbl[8]; snprintf(lbl, sizeof(lbl), "%d", val);
                    _cells[idx]->SetLabel(lbl);
                }
            }
        }
    }

    int _N, _moves, _shuffleSteps;
    std::vector<int> _board;
    std::vector<CellButton*> _cells;
    BView* _gridHost;
    BGridLayout* _grid;
    BButton* _btnNew;
    BButton* _btnShuffle;
    BRadioButton* _size3;
    BRadioButton* _size4;
    BRadioButton* _size5;
    BSlider* _slider;
    BStringView* _status;
    BMenuBar* _menubar;
};

class App : public BApplication {
public:
    App(): BApplication("application/x-vnd.ptit-npuzzle") {}
    void ReadyToRun() override { new PuzzleWindow(); }
};

int main(){ App().Run(); return 0; }

