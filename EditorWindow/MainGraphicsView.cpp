#include "MainGraphicsView.h"
#include "Operation.h"
#include "LevelComponents/Tile.h"

#include <QScrollBar>
#include <QMouseEvent>

#include <iostream>

extern WL4EditorWindow *singleton;

// TODO why is this event not getting called?
void MainGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if(!singleton->FirstROMIsLoaded()) return;

    // Get the ID of the tile that was clicked
    int X = event->x() + horizontalScrollBar()->sliderPosition();
    int Y = event->y() + verticalScrollBar()->sliderPosition();
    int tileX = X / 32;
    int tileY = Y / 32;
    std::cout << "(" << tileX << ", " << tileY << ")" << std::endl;

    // Change the tile
    LevelComponents::Room *room = singleton->GetCurrentRoom();
    if(tileX < room->GetWidth() && tileY < room->GetHeight())
    {
        enum Ui::EditMode editMode = singleton->GetEditModeWidgetPtr()->GetEditModeParams().editMode;
        if(editMode == Ui::LayerEditMode)
        {
            unsigned short selectedTile = singleton->GetTile16DockWidgetPtr()->GetSelectedTile();
            if(selectedTile == 0xFFFF) return;
            int selectedLayer = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedLayer;
            LevelComponents::Layer *layer = room->GetLayer(selectedLayer);
            int selectedTileIndex = tileX + tileY * room->GetWidth();
            struct OperationParams *params = new struct OperationParams();
            params->type = ChangeTileOperation;
            params->tileChangeParams.push_back(TileChangeParams::Create(
                tileX,
                tileY,
                selectedLayer,
                selectedTile,
                layer->GetLayerData()[selectedTileIndex]
            ));
            ExecuteOperation(params);
        }
        // TODO add more cases for other edit mode types
    }
}
