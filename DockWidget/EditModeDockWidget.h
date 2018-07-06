#ifndef EDITMODEDOCKWIDGET_H
#define EDITMODEDOCKWIDGET_H

#include <QDockWidget>
#include <QAbstractButton>

namespace Ui {
    class EditModeDockWidget;

    // Enumeration of the edit modes supported by the main window
    enum EditMode
    {
        LayerEditMode  = 0,
        EntityEditMode = 1,
        DoorEditMode   = 2,
        CameraEditMode = 3
    };

    // This struct defines the parameters that are obtained from the edit mode dock widget UI
    struct EditModeParams
    {
        enum EditMode editMode = LayerEditMode;
        int selectedLayer = 0;
        bool layersEnabled[4] = { true, true, true, true };
        bool entitiesEnabled = true;
        bool doorsEnabled = true;
        bool cameraAreasEnabled = true;
        bool alphaBlendingEnabled = true;
        int seleteddifficulty = 0;
    };
}

class EditModeDockWidget : public QDockWidget
{
    Q_OBJECT

private:
    Ui::EditModeDockWidget *ui;

    std::map<QAbstractButton*, enum Ui::EditMode> modeEnums;
    std::map<QAbstractButton*, int> layerIndices;
    std::map<QAbstractButton*, int> difficultyIndices;

public:
    explicit EditModeDockWidget(QWidget *parent = 0);
    ~EditModeDockWidget();
    struct Ui::EditModeParams GetEditModeParams();
};

#endif // EDITMODEDOCKWIDGET_H
