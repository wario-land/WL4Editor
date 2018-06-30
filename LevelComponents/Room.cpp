#include "Room.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

#include <cstdlib>
#include <cstring>
#include <QPainter>

namespace LevelComponents
{
    /// <summary>
    /// Construct a new Room object.
    /// </summary>
    /// <param name="roomDataPtr">Pointer to the start of the room data header.</param>
    /// <param name="_RoomID">Zero-based ID for the room in the level.</param>
    /// <param name="_LevelID">0x03000023 level index value.</param>
    Room::Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID)
    {
        this->RoomID = _RoomID;

        // Copy the room header information
        memcpy(&this->RoomHeader, ROMUtils::CurrentFile + roomDataPtr, sizeof(struct __RoomHeader));

        // Set up tileset
        int tilesetIndex = ROMUtils::CurrentFile[roomDataPtr];
        this->TilesetID = tilesetIndex;
        int tilesetPtr = WL4Constants::TilesetDataTable + tilesetIndex * 36;
        tileset = new Tileset(tilesetPtr, tilesetIndex);

        // Set up the layer data
        int dimensionPointer = ROMUtils::PointerFromData(roomDataPtr + 12);
        Width = ROMUtils::CurrentFile[dimensionPointer];
        Height = ROMUtils::CurrentFile[dimensionPointer + 1];
        enum LayerMappingType mappingType;
        for(int i = 0; i < 4; ++i)
        {
            mappingType = static_cast<enum LayerMappingType>(ROMUtils::CurrentFile[roomDataPtr + i + 1] & 0x30);
            int layerPtr = ROMUtils::PointerFromData(roomDataPtr + i * 4 + 8);
            layers[i] = new Layer(layerPtr, mappingType, tileset);
        }

        // Prioritize the layers
        int priorityFlag = ROMUtils::CurrentFile[roomDataPtr + 26];
        switch(priorityFlag & 3)
        {
        case 0:
            layers[0]->SetLayerPriority(0);
            layers[1]->SetLayerPriority(1);
            layers[2]->SetLayerPriority(2);
            break;
        case 1:
        case 2:
            layers[0]->SetLayerPriority(1);
            layers[1]->SetLayerPriority(0);
            layers[2]->SetLayerPriority(2);
            break;
        case 3:
            layers[0]->SetLayerPriority(2);
            layers[1]->SetLayerPriority(0);
            layers[2]->SetLayerPriority(1);
        }
        layers[3]->SetLayerPriority(3);

        // Get the information about Layer 0 color blending, using priorityFlag
        if(priorityFlag > 7)
        {
            this->Layer0ColorBlending = true;
            switch((priorityFlag - 8) >> 2)
            {
            case 0: this->Layer0ColorBLendCoefficient_EVA = 7; this->Layer0ColorBLendCoefficient_EVB = 16; break;
            case 1: this->Layer0ColorBLendCoefficient_EVA = 10; this->Layer0ColorBLendCoefficient_EVB = 16; break;
            case 2: this->Layer0ColorBLendCoefficient_EVA = 13; this->Layer0ColorBLendCoefficient_EVB = 16; break;
            case 3: this->Layer0ColorBLendCoefficient_EVA = 16; this->Layer0ColorBLendCoefficient_EVB = 16; break;
            case 4: this->Layer0ColorBLendCoefficient_EVA = 16; this->Layer0ColorBLendCoefficient_EVB = 0; break;
            case 5: this->Layer0ColorBLendCoefficient_EVA = 13; this->Layer0ColorBLendCoefficient_EVB = 3; break;
            case 6: this->Layer0ColorBLendCoefficient_EVA = 10; this->Layer0ColorBLendCoefficient_EVB = 6; break;
            case 7: this->Layer0ColorBLendCoefficient_EVA = 7; this->Layer0ColorBLendCoefficient_EVB = 9; break;
            case 8: this->Layer0ColorBLendCoefficient_EVA = 5; this->Layer0ColorBLendCoefficient_EVB = 11; break;
            case 9: this->Layer0ColorBLendCoefficient_EVA = 3; this->Layer0ColorBLendCoefficient_EVB = 13; break;
            case 10: this->Layer0ColorBLendCoefficient_EVA = 0; this->Layer0ColorBLendCoefficient_EVB = 16;
            }
        }

        // Set up camera control data
        // TODO are there more types than 1, 2 and 3?
        if((CameraControlType = static_cast<enum __CameraControlType>(ROMUtils::CurrentFile[roomDataPtr + 24])) == HasControlAttrs)
        {
            int pLevelCameraControlPointerTable = ROMUtils::PointerFromData(WL4Constants::CameraControlPointerTable + _LevelID * 4);
            for(int i = 0; i < 16; i++)
            {
                int CurrentPointer = ROMUtils::PointerFromData(pLevelCameraControlPointerTable + i * 4);
                if(CurrentPointer == WL4Constants::CameraRecordSentinel)
                    break;
                if(ROMUtils::CurrentFile[CurrentPointer] == _RoomID)
                {
                    int RecordNum = ROMUtils::CurrentFile[CurrentPointer + 1];
                    struct __CameraControlRecord *recordPtr = (struct __CameraControlRecord*) (ROMUtils::CurrentFile + CurrentPointer + 2);
                    while(RecordNum--)
                    {
                        CameraControlRecords.push_back(recordPtr++);
                    }
                    break;
                }
            }
        }

        // TODO
    }

    QGraphicsScene *Room::RenderGraphicsScene(QGraphicsScene *scene, struct RenderUpdateParams *renderParams)
    {
        switch(renderParams->type)
        {
        case FullRender:
            {
                // Order the layers by their priority
                Layer *drawLayers[4];
                memcpy(drawLayers, layers, 4 * sizeof(Layer*));
                qsort(drawLayers, 4, sizeof(Layer*), [](const void *data1, const void *data2){
                    Layer *layer1 = *(Layer**) data1;
                    Layer *layer2 = *(Layer**) data2;
                    return layer2->GetLayerPriority() - layer1->GetLayerPriority();
                });

                // Create a graphics scene with the layers added in order of priority
                int sceneWidth = Width * 16;
                int sceneHeight = Height * 16;
                if(scene) { delete scene; }
                scene = new QGraphicsScene(0, 0, sceneWidth, sceneHeight);
                for(int i = 0; i < 4; ++i)
                {
                    if(drawLayers[i]->IsVisible())
                    {
                        QPixmap pixmap = drawLayers[i]->RenderLayer();
                        if(drawLayers[i]->GetMappingType() == LayerTile8x8)
                        {
                            QPixmap pixmap2(sceneWidth, sceneHeight);
                            QPainter painter(&pixmap2);
                            for(int y = 0; y < sceneHeight; y += drawLayers[i]->GetLayerheight() * 8)
                            {
                                for(int x = 0; x < sceneWidth; x += drawLayers[i]->GetLayerwidth() * 8)
                                {
                                    QPoint drawDestination(x, y);
                                    painter.drawImage(drawDestination, pixmap.toImage());
                                }
                            }
                            scene->addPixmap(pixmap2);
                        }
                        else
                        {
                            scene->addPixmap(pixmap);
                        }
                    }
                }
            }
            return scene;
        case SingleTile:
            // TODO
            return scene;
        case LayerEnable:
            // TODO
            return scene;
        }

    }
}
