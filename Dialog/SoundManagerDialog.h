#ifndef SOUNDMANAGERDIALOG_H
#define SOUNDMANAGERDIALOG_H

#include <QDialog>
#include <QList>
#include <QModelIndex>

#include "Sound/SoundTypes.h"

class QGroupBox;
class QStandardItem;
class QStandardItemModel;

namespace Ui {
class SoundManagerDialog;
}

class SoundManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SoundManagerDialog(QWidget *parent = nullptr);
    ~SoundManagerDialog();

public slots:
    // QDialog::accept() is a virtual slot. The Save button in the .ui is
    // connected to accepted(), so overriding accept() lets the dialog save the
    // staged sound data before closing. reject() is the Cancel button: it
    // discards staged edits after confirmation.
    void accept() override;
    void reject() override;

    // Summary is logged after WL4EditorWindow reloads the ROM, because the
    // reload clears the Output dock.
    QString SaveSummary() const { return saveSummary; }

private slots:
    void on_lineEditSearch_textChanged(const QString &arg1);
    void on_checkBoxShowEmpty_checkStateChanged(const Qt::CheckState &arg1);
    void on_checkBoxShowChanged_checkStateChanged(const Qt::CheckState &arg1);
    void on_pushButtonExportAll_clicked();
    void on_pushButtonImportAll_clicked();

    // Connected manually to QItemSelectionModel::currentChanged. QTreeView
    // does not have a currentChanged signal, so an on_* auto-connection cannot
    // work here.
    void onEntryCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    enum class EntryType
    {
        Song = 1,
        Bank,
        Program,
        Sample,
        Player
    };

    Ui::SoundManagerDialog *ui;
    Sound::ParseResult soundData;
    QStandardItemModel *entryModel = nullptr;
    bool populatingTree = false;
    QString saveSummary;

    void ParseSoundData();
    void PopulateTree();
    void ApplyFilter();
    void UpdateStatusLabel();
    bool HasStagedChanges() const;
    void ReloadModelFromROM();

    void ClearDetailPanel();
    QGroupBox *CreateGroup(const QString &title);
    void AddReadOnlyRow(QGroupBox *group, const QString &label, const QString &value);
    void BuildSongDetail(const Sound::Song &song);
    void BuildBankDetail(const Sound::Bank &bank);
    void BuildProgramDetail(int bankId, int programIndex);
    void BuildSampleDetail(const Sound::Sample &sample);
    void BuildPlayerDetail(const Sound::Player &player);

    QList<QStandardItem *> CreateEntryRow(const QString &name, int id, EntryType type,
                                          const QString &typeText, const QString &status,
                                          int parentId = -1) const;
    const Sound::Bank *FindBank(int bankId) const;
    const Sound::Song *FindSong(int songId) const;
};

#endif // SOUNDMANAGERDIALOG_H
