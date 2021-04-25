#include "ChunkManagerInfoGroupBox.h"
#include "WL4EditorWindow.h"

#include <QPushButton>

extern WL4EditorWindow *singleton;

/// <summary>
/// Construct an instance of the ChunkManagerInfoGroupBox.
/// </summary>
/// <param name="parent">
/// The parent QWidget.
/// </param>
ChunkManagerInfoGroupBox::ChunkManagerInfoGroupBox(QWidget *parent) : QGroupBox(parent), Layout(QBoxLayout::Direction::Down)
{
    // Configure the group box
    setLayout(&Layout);
}

ChunkManagerInfoGroupBox::~ChunkManagerInfoGroupBox()
{

}

void ChunkManagerInfoGroupBox::UpdateContents(const QModelIndex &current, const QModelIndex &previous)
{
    (void) previous;
    const QStandardItemModel *model = dynamic_cast<const QStandardItemModel*>(current.model());
    QStandardItem *item = model->itemFromIndex(current);
    QStandardItem *parent = item->parent();
    if(parent)
    {
        item = parent->child(current.row(), 1);
        unsigned int chunkAddress = item->text().mid(2).toUInt(nullptr, 16);
        QVector<QWidget*> details = GetInfoFromChunk(chunkAddress);
        ClearLayout(&Layout);
        for(QWidget *widget : details)
        {
            Layout.addWidget(widget);
        }
        Layout.addStretch();
    }
}

QVector<QWidget*> ChunkManagerInfoGroupBox::GetInfoFromChunk(unsigned int chunk)
{
    // Gather info from the chunk header
    QVector<QWidget*> widgets;
    unsigned char *chunkHeader = ROMUtils::CurrentFile + chunk;
    unsigned char *chunkData = chunkHeader + 12;
    unsigned int chunkLen = *reinterpret_cast<unsigned short*>(chunkHeader + 4);
    unsigned int extLen = (unsigned int) *reinterpret_cast<unsigned char*>(chunkHeader + 9) << 16;
    unsigned int chunkSize = chunkLen + extLen + 12;
    unsigned char chunkType = chunkHeader[8];

    // Add universal chunk info
    widgets.append(new QLabel(QString(tr("Chunk address: 0x%1"))
        .arg(QString::number(chunk, 16).toUpper())));
    widgets.append(new QLabel(QString(tr("Chunk size: %1"))
        .arg(QString::number(chunkSize))));

    if(chunkType >= CHUNK_TYPE_COUNT)
    {
        widgets.append(new QLabel(QString(tr("Unknown chunk type: 0x%1"))
            .arg(QString::number(chunkType, 16).toUpper())));
    }
    else
    {
        widgets.append(new QLabel(QString(tr("Chunk type: %1"))
            .arg(ROMUtils::ChunkTypeString[chunkType])));

        // Different info per chunk type
        switch(chunkType)
        {
        case ROMUtils::SaveDataChunkType::RoomHeaderChunkType:

            break;

        case ROMUtils::SaveDataChunkType::DoorChunkType:

            break;

        case ROMUtils::SaveDataChunkType::LayerChunkType:

            break;

        case ROMUtils::SaveDataChunkType::LevelNameChunkType:

            widgets.append(new QLabel(QString(tr("English: %1"))
                .arg("Foo")));
            widgets.append(new QLabel(QString(tr("Japanese: %1"))
                .arg("Bar")));
            break;

        case ROMUtils::SaveDataChunkType::EntityListChunk:

            break;

        case ROMUtils::SaveDataChunkType::CameraPointerTableType:

            break;

        case ROMUtils::SaveDataChunkType::CameraBoundaryChunkType:

            break;

        case ROMUtils::SaveDataChunkType::PatchListChunk:

            break;

        case ROMUtils::SaveDataChunkType::PatchChunk:

            break;

        case ROMUtils::SaveDataChunkType::TilesetForegroundTile8x8DataChunkType:

            break;

        case ROMUtils::SaveDataChunkType::TilesetMap16EventTableChunkType:

            break;

        case ROMUtils::SaveDataChunkType::TilesetMap16TerrainChunkType:

            break;

        case ROMUtils::SaveDataChunkType::TilesetMap16DataChunkType:

            break;

        case ROMUtils::SaveDataChunkType::TilesetPaletteDataChunkType:

            break;

        case ROMUtils::SaveDataChunkType::EntityTile8x8DataChunkType:

            break;

        case ROMUtils::SaveDataChunkType::EntityPaletteDataChunkType:

            break;

        case ROMUtils::SaveDataChunkType::EntitySetLoadTableChunkType:

            break;

        default:
            break;
        }
    }

    // Format labels
    for(auto widget : widgets)
    {
        QLabel *label = qobject_cast<QLabel*>(widget);
        if(label)
        {
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    }

    return widgets;
}

void ChunkManagerInfoGroupBox::ClearLayout(QLayout *layout)
{
    while(QLayoutItem *item = layout->takeAt(0))
    {
        if(QWidget *widget = item->widget())
        {
            widget->deleteLater();
        }
        if(QLayout *childLayout = item->layout())
        {
            ClearLayout(childLayout);
        }
        delete item;
    }
}
