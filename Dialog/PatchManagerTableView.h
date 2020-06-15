#ifndef PATCHMANAGERTABLEVIEW_H
#define PATCHMANAGERTABLEVIEW_H

#include <QTableView>
#include <QStandardItemModel>
#include <PatchUtils.h>
#include <QHeaderView>

class PatchEntryTableModel : public QStandardItemModel
{
    Q_OBJECT

public:
    PatchEntryTableModel(QWidget *_parent);
    ~PatchEntryTableModel() { };
    void AddEntry(struct PatchEntryItem entry) { entries.append(entry); }
    QVector<struct PatchEntryItem> entries;
    void RemoveEntries(QModelIndexList entries);

private:
    QWidget *parent;
};

class PatchManagerTableView : public QTableView
{
    Q_OBJECT

private:
    PatchEntryTableModel EntryTableModel;
    ~PatchManagerTableView();

public:
    PatchManagerTableView(QWidget *param);
    void UpdateTableView();
    void RemoveSelected();
    struct PatchEntryItem GetSelectedEntry();
    QVector<struct PatchEntryItem> GetAllEntries() { return EntryTableModel.entries; }
    void AddEntry(struct PatchEntryItem entry);
    void UpdateEntry(int index, struct PatchEntryItem entry);
};

// https://forum.qt.io/topic/92742/empty-qtablewidget-vertical-header-not-showing-up/3
class PersistentHeader : public QHeaderView
{
    Q_OBJECT
    Q_DISABLE_COPY(PersistentHeader)

public:
    PersistentHeader(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR) : QHeaderView(orientation,parent) {}
    QSize sizeHint() const Q_DECL_OVERRIDE{
        const QSize original = QHeaderView::sizeHint();
        if(original.width() > 0 || original.height() > 0)
            return original;
        ensurePolished();
        QStyleOptionHeader opt;
        initStyleOption(&opt);
        opt.section = 0;
        QFont fnt = font();
        fnt.setBold(true);
        opt.fontMetrics = QFontMetrics(fnt);
        opt.text = QStringLiteral("0");
        if (isSortIndicatorShown())
            opt.sortIndicator = QStyleOptionHeader::SortDown;
        return style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), this);
    }
};

#endif // PATCHMANAGERTABLEVIEW_H
