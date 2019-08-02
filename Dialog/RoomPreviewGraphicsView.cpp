#include "RoomPreviewGraphicsView.h"

#include <LevelComponents/Layer.h>
#include <QScrollBar>
#include <cstring>

/// <summary>
/// Construct an instance of the RoomPreviewGraphicsView.
/// </summary>
/// <remarks>
/// Initialize the QGraphicsScene with empty pixmaps to be filled by the member functions.
/// </remarks>
/// <param name="parent">
/// The parent QWidget.
/// </param>
RoomPreviewGraphicsView::RoomPreviewGraphicsView(QWidget *param) : QGraphicsView(param), displayPixmap(0)
{
    memset(dataPointers, 0, sizeof(dataPointers));
    QGraphicsScene *scene = new QGraphicsScene();
    for (unsigned int i = 0; i < sizeof(pixmapItems) / sizeof(pixmapItems[0]); ++i)
    { pixmapItems[i] = scene->addPixmap(QPixmap()); }
    EnableSelectedPixmap();
    setScene(scene);
    setAlignment(Qt::AlignTop | Qt::AlignLeft);
}

/// <summary>
/// Deconstruct the RoomPreviewGraphicsView and clean up its instance objects on the heap.
/// </summary>
RoomPreviewGraphicsView::~RoomPreviewGraphicsView()
{
    delete scene(); // This should also delete the underlying pixmaps
}

/// <summary>
/// Enable only the selected pixmap for viewing.
/// </summary>
/// <remarks>
/// Which pixmap is "selected" is determined by the instance variable "displayPixmap".
/// </remarks>
void RoomPreviewGraphicsView::EnableSelectedPixmap()
{
    for (unsigned int i = 0; i < sizeof(pixmapItems) / sizeof(pixmapItems[0]); ++i)
    { pixmapItems[i]->setVisible(displayPixmap == i); }
}

/// <summary>
/// Display the next available pixmap graphic.
/// </summary>
void RoomPreviewGraphicsView::DisplayNextPixmap()
{
    if (displayPixmap > 2 || !dataPointers[++displayPixmap]) { displayPixmap = 0; }
    EnableSelectedPixmap();
}

/// <summary>
/// Swap between different viewing modes.
/// </summary>
/// <remarks>
/// If the current options for the graphics view do not include layer 0 with mapping type 0x20, it will not be a viewing
/// option.
/// </remarks>
/// <param name="event">
/// The object from which the mouse press position can be obtained.
/// </param>
void RoomPreviewGraphicsView::mousePressEvent(QMouseEvent *event)
{
    (void) event;
    DisplayNextPixmap();
    switch (displayPixmap)
    {
    case 0:
        infoLabel->setText("Tileset:");
        break;
    case 1:
        infoLabel->setText("Background:");
        break;
    case 2:
        infoLabel->setText("Layer 0:");
        break;
    }
    infoLabel->repaint();
}

/// <summary>
/// Show the graphics view, and set the scrollbars to the top left of the view.
/// </summary>
/// <remarks>
/// Without this override, the graphics view will initialize with the scrollbars in the middle of the scrollable area.
/// </remarks>
/// <param name="event">
/// Resize event information which is sent to the parent implementation of this function.
/// </param>
void RoomPreviewGraphicsView::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    if (!initialized)
    {
        initialized = true;
        verticalScrollBar()->setValue(0);
        horizontalScrollBar()->setValue(0);
    }
}

/// <summary>
/// Update one or more of the graphics layers that can be displayed by the graphics view.
/// </summary>
/// <remarks>
/// Any parameters equal to the existing instance data pointers in "dataPointers" will not be re-rendered.
/// </remarks>
/// <param name="tileset">
/// The tileset to display.
/// </param>
/// <param name="BGptr">
/// The BG layer to display.
/// </param>
/// <param name="L0ptr">
/// The mapping type 0x20 layer 0 to display.
/// </param>
void RoomPreviewGraphicsView::UpdateGraphicsItems(LevelComponents::Tileset *tileset, int BGptr, int L0ptr)
{
    // Update tilset
    if (tileset != dataPointers[0])
    {
        dataPointers[0] = tileset;
        QPixmap tilesetPixmap = tileset->Render(3);
        pixmapItems[0]->setPixmap(tilesetPixmap);
    }

    // Update BG layer preview
    if (BGptr != (long) dataPointers[1])
    {
        if ((dataPointers[1] = (void *) BGptr))
        {
            LevelComponents::Layer BGlayer(BGptr, LevelComponents::LayerTile8x8);
            QPixmap layerPixmap = BGlayer.RenderLayer(tileset);
            pixmapItems[1]->setPixmap(layerPixmap);
        }
    }

    // Update layer 0 preview
    if (L0ptr != (long) dataPointers[2])
    {
        if ((dataPointers[2] = (void *) L0ptr))
        {
            LevelComponents::Layer L0layer(L0ptr, LevelComponents::LayerTile8x8);
            QPixmap layerPixmap = L0layer.RenderLayer(tileset);
            pixmapItems[2]->setPixmap(layerPixmap);
        }
    }

    // Make sure the selected layer is viewable
    if (!dataPointers[displayPixmap]) { displayPixmap = 0; }
    EnableSelectedPixmap();
}
