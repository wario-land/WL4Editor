#ifndef EDITMODEDOCKWIDGET_H
#define EDITMODEDOCKWIDGET_H

#include <QAbstractButton>
#include <QDockWidget>

namespace Ui
{
    class EditModeDockWidget;

    // Enumeration of the edit modes supported by the main window
    enum EditMode
    {
        LayerEditMode = 0,
        EntityEditMode = 1,
        DoorEditMode = 2,
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
        bool entitiesboxesDisabled = false;
        bool hiddencoinsEnabled = false;
    };
} // namespace Ui

class EditModeDockWidget : public QDockWidget
{
    Q_OBJECT

private:
    Ui::EditModeDockWidget *ui;

    // Internal structures used to quickly obtain the selected options as a struct
    std::map<QAbstractButton *, enum Ui::EditMode> modeEnums;
    std::map<QAbstractButton *, int> layerIndices;
    std::map<QAbstractButton *, int> difficultyIndices;
    QButtonGroup *modeGroup;
    QButtonGroup *layerGroup;
    QButtonGroup *difficultyGroup;

public:
    explicit EditModeDockWidget(QWidget *parent = 0);
    struct Ui::EditModeParams GetEditModeParams();
    ~EditModeDockWidget();
    void SetLayersCheckBoxEnabled(int index, bool usable);
    void SetDifficultyRadioBox(int modeid);
    bool *GetLayersVisibilityArray();
    void UncheckHiddencoinsViewCheckbox();

private slots:
    void on_CheckBox_Layer0View_stateChanged(int arg1);
    void on_CheckBox_Layer1View_stateChanged(int arg1);
    void on_CheckBox_Layer2View_stateChanged(int arg1);
    void on_CheckBox_Layer3View_stateChanged(int arg1);
    void on_CheckBox_EntityView_stateChanged(int arg1);
    void on_CheckBox_DoorView_stateChanged(int arg1);
    void on_CheckBox_CameraView_stateChanged(int arg1);
    void on_CheckBox_AlphaView_stateChanged(int arg1);
    void on_RadioButton_DoorMode_toggled(bool checked);
    void on_RadioButton_NMode_toggled(bool checked);
    void on_RadioButton_HMode_toggled(bool checked);
    void on_RadioButton_SHMode_toggled(bool checked);
    void on_RadioButton_LayerMode_toggled(bool checked);
    void on_RadioButton_EntityMode_toggled(bool checked);
    void on_RadioButton_CameraMode_toggled(bool checked);
    void on_CheckBox_hiddencoinsView_stateChanged(int arg1);
};

#endif // EDITMODEDOCKWIDGET_H
