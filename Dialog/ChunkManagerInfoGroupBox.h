#ifndef CHUNKEDITORINFOGROUPBOX_H
#define CHUNKEDITORINFOGROUPBOX_H

#include "ROMUtils.h"

#include <QGroupBox>
#include <QBoxLayout>

class ChunkManagerInfoGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    ChunkManagerInfoGroupBox(QWidget *parent = nullptr);
    ~ChunkManagerInfoGroupBox();
private:
    QVector<QWidget*> GetInfoFromChunk(unsigned int chunk);
    void ClearLayout(QLayout *layout);
    QBoxLayout Layout;
public slots:
    void UpdateContents(const QModelIndex &current, const QModelIndex &previous);
};

#endif // CHUNKEDITORINFOGROUPBOX_H
