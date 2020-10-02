#ifndef CREDITSEDITDIALOG_H
#define CREDITSEDITDIALOG_H

#include <QDialog>
#include <QString>
#include <QImage>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QColorDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItem>

#include "WL4Constants.h"
#include "Dialog/SelectColorDialog.h"
#include "Dialog/CustomQTableView.h"
// #include "ROMUtils.h" // can be find in Room.h
#include "LevelComponents/Room.h"


namespace DialogParams
{
    struct CreditsEditParams
    {
        // Default constructor
        CreditsEditParams() { memset(this, 0, sizeof(struct CreditsEditParams)); }
        ~CreditsEditParams() {}
    };
}

namespace Ui {
    class CreditsEditDialog;
}

class CreditsEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreditsEditDialog(QWidget *parent, DialogParams::CreditsEditParams *creditsEditParams);
    ~CreditsEditDialog();

    static void StaticInitialization();
private slots:

    void on_buttonBox_accepted();

private:
    Ui::CreditsEditDialog *ui;

    // members
    DialogParams::CreditsEditParams* creditsEditParams;
    QWidget* tabs[13];
    CustomQTableView* tables_view[13];
    short data_to_save[13][1280]; //0x500

    static std::map<short,char> CreditTileMap;

    // clang-format off
    static std::map< std::pair<std::string,std::string>, short> CreditTileReverseMap;
    static constexpr const short CreditTileMapData[0x108] =
    {
        //One-tile high
        0x4340,'A',
        0x4341,'B',
        0x4342,'C',
        0x4343,'D',
        0x4344,'E',
        0x4345,'F',
        0x4346,'G',
        0x4347,'H',
        0x4348,'I',
        0x4349,'J',
        0x434A,'K',
        0x434B,'L',
        0x434C,'M',
        0x434D,'N',
        0x434E,'O',
        0x434F,'P',
        0x4350,'Q',
        0x4351,'R',
        0x4352,'S',
        0x4353,'T',
        0x4354,'U',
        0x4355,'V',
        0x4356,'W',
        0x4357,'X',
        0x4358,'Y',
        0x4359,'Z',
        0x435A,'.',
        0x435B,',',  // 0x37

        0x0360,'A',
        0x0361,'B',
        0x0362,'C',
        0x0363,'D',
        0x0364,'E',
        0x0365,'F',
        0x0366,'G',
        0x0367,'H',
        0x0368,'I',
        0x0369,'J',
        0x036A,'K',
        0x036B,'L',
        0x036C,'M',
        0x036D,'N',
        0x036E,'O',
        0x036F,'P',
        0x0370,'Q',
        0x0371,'R',
        0x0372,'S',
        0x0373,'T',
        0x0374,'U',
        0x0375,'V',
        0x0376,'W',
        0x0377,'X',
        0x0378,'Y',
        0x0379,'Z',

        0x03A0,'a',
        0x03A1,'b',
        0x03A2,'c',
        0x03A3,'d',
        0x03A4,'e',
        0x03A5,'f',
        0x03A6,'g',
        0x03A7,'h',
        0x03A8,'i',
        0x03A9,'j',
        0x03AA,'k',
        0x03AB,'l',
        0x03AC,'m',
        0x03AD,'n',
        0x03AE,'o',
        0x03AF,'p',
        0x03B0,'q',
        0x03B1,'r',
        0x03B2,'s',
        0x03B3,'t',
        0x03B4,'u',
        0x03B5,'v',
        0x03B6,'w',
        0x03B7,'x',
        0x03B8,'y',
        0x03B9,'z',  // 0x37 + 0x68 = 0x9F

        //Two-tile high, lower half
        0x0380,'A',
        0x0381,'B',
        0x0382,'C',
        0x0383,'D',
        0x0384,'E',
        0x0385,'F',
        0x0386,'G',
        0x0387,'H',
        0x0388,'I',
        0x0389,'J',
        0x038A,'K',
        0x038B,'L',
        0x038C,'M',
        0x038D,'N',
        0x038E,'O',
        0x038F,'P',
        0x0390,'Q',
        0x0391,'R',
        0x0392,'S',
        0x0393,'T',
        0x0394,'U',
        0x0395,'V',
        0x0396,'W',
        0x0397,'X',
        0x0398,'Y',
        0x0399,'Z',

        0x03C0,'a',
        0x03C1,'b',
        0x03C2,'c',
        0x03C3,'d',
        0x03C4,'e',
        0x03C5,'f',
        0x03C6,'g',
        0x03C7,'h',
        0x03C8,'i',
        0x03C9,'j',
        0x03CA,'k',
        0x03CB,'l',
        0x03CC,'m',
        0x03CD,'n',
        0x03CE,'o',
        0x03CF,'p',
        0x03D0,'q',
        0x03D1,'r',
        0x03D2,'s',
        0x03D3,'t',
        0x03D4,'u',
        0x03D5,'v',
        0x03D6,'w',
        0x03D7,'x',
        0x03D8,'y',
        0x03D9,'.'  // 0x9F + 0x68 = 0x107
    };
    // clang-format on


    /*pair<string, string> key = make_pair("bob", "sue");
    mymap[ key ] = 5; // you can inline make_pair if you prefer.

    // or you can use insert method
    mymap.insert( key, 5 );*/
};

#endif // CREDITSEDITDIALOG_H
