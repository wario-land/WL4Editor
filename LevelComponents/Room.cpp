#include "Room.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

#include <cstdlib>
#include <cstring>
#include <QPainter>

#include <iostream>

#define MIN(x,y) ((x)<(y)?(x):(y))

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
        memset(RenderedLayers, 0, sizeof(RenderedLayers));

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
            layers[i] = new Layer(layerPtr, mappingType);
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
        if((Layer0ColorBlending = priorityFlag > 7))
        {
            switch((priorityFlag - 8) >> 2)
            {
            case  0: Layer0ColorBlendCoefficient_EVA =  7; Layer0ColorBlendCoefficient_EVB = 16; break;
            case  1: Layer0ColorBlendCoefficient_EVA = 10; Layer0ColorBlendCoefficient_EVB = 16; break;
            case  2: Layer0ColorBlendCoefficient_EVA = 13; Layer0ColorBlendCoefficient_EVB = 16; break;
            case  3: Layer0ColorBlendCoefficient_EVA = 16; Layer0ColorBlendCoefficient_EVB = 16; break;
            case  4: Layer0ColorBlendCoefficient_EVA = 16; Layer0ColorBlendCoefficient_EVB =  0; break;
            case  5: Layer0ColorBlendCoefficient_EVA = 13; Layer0ColorBlendCoefficient_EVB =  3; break;
            case  6: Layer0ColorBlendCoefficient_EVA = 10; Layer0ColorBlendCoefficient_EVB =  6; break;
            case  7: Layer0ColorBlendCoefficient_EVA =  7; Layer0ColorBlendCoefficient_EVB =  9; break;
            case  8: Layer0ColorBlendCoefficient_EVA =  5; Layer0ColorBlendCoefficient_EVB = 11; break;
            case  9: Layer0ColorBlendCoefficient_EVA =  3; Layer0ColorBlendCoefficient_EVB = 13; break;
            case 10: Layer0ColorBlendCoefficient_EVA =  0; Layer0ColorBlendCoefficient_EVB = 16;
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

        // Load Entity list for each difficulty level
        int Listaddress;
        EntityRoomAttribute tmpEntityroomattribute;
        int k;
        for(int i = 0; i < 3; i++)
        {
            Listaddress = ROMUtils::PointerFromData(roomDataPtr + 28 + 4 * i);
            k = 0;
            while(ROMUtils::CurrentFile[Listaddress + 3 * k] != (unsigned char) '\xFF') // maximum entity count is 46
            {
                tmpEntityroomattribute.YPos     = (int) ROMUtils::CurrentFile[Listaddress + 3 * k];
                tmpEntityroomattribute.XPos     = (int) ROMUtils::CurrentFile[Listaddress + 3 * k + 1];
                tmpEntityroomattribute.EntityID = (int) ROMUtils::CurrentFile[Listaddress + 3 * k + 2];
                EntityList[i].push_back(tmpEntityroomattribute);
                k++;
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
                struct DLS
                {
                    Layer *layer;
                    int index;
                } *drawLayers[4];
                for(int i = 0; i < 4; ++i)
                {
                    struct DLS *newDLS = new struct DLS;
                    newDLS->layer = layers[i];
                    newDLS->index = i;
                    drawLayers[i] = newDLS;
                }
                qsort(drawLayers, 4, sizeof(void*), [](const void *data1, const void *data2)
                {
                    struct DLS *layer1 = *(struct DLS**) data1;
                    struct DLS *layer2 = *(struct DLS**) data2;
                    return layer2->layer->GetLayerPriority() - layer1->layer->GetLayerPriority();
                });

                // Create a graphics scene with the layers added in order of priority
                int sceneWidth = Width * 16;
                int sceneHeight = Height * 16;
                if(scene) { delete scene; }
                scene = new QGraphicsScene(0, 0, sceneWidth, sceneHeight);
                int Z = 0;
                for(int i = 0; i < 4; ++i)
                {
                    QPixmap pixmap = drawLayers[i]->layer->RenderLayer(tileset);
                    if(drawLayers[i]->layer->GetMappingType() == LayerTile8x8)
                    {
                        QPixmap pixmap2(sceneWidth, sceneHeight);
                        QPainter painter(&pixmap2);
                        for(int y = 0; y < sceneHeight; y += drawLayers[i]->layer->GetLayerHeight() * 8)
                        {
                            for(int x = 0; x < sceneWidth; x += drawLayers[i]->layer->GetLayerWidth() * 8)
                            {
                                QPoint drawDestination(x, y);
                                painter.drawImage(drawDestination, pixmap.toImage());
                            }
                        }
                        pixmap = pixmap2;
                    }
                    QGraphicsPixmapItem *pixmapItem = scene->addPixmap(pixmap);
                    pixmapItem->setZValue(Z++);
                    RenderedLayers[drawLayers[i]->index] = pixmapItem;

                    // Render alpha blended composite pixmap for layer 0 if alpha blending is enabled
                    if(!drawLayers[i]->index & 0) // TODO this is still broken (remove "& 0")
                    {
                        if(Layer0ColorBlending)
                        {
                            // Overlay the EVA layers
                            QPixmap alphaPixmap(sceneWidth, sceneHeight);
                            alphaPixmap.fill(Qt::transparent);
                            QPainter alphaPainter(&alphaPixmap);
                            for(int j = 0; j < 3; ++j)
                            {
                                if(!drawLayers[j]->index) break;
                                alphaPainter.drawImage(0, 0, RenderedLayers[drawLayers[j]->index]->pixmap().toImage());
                            }

                            // Blend the EVA and EVB pixels for the new layer
                            QImage imageA = alphaPixmap.toImage();
                            QImage imageB = RenderedLayers[0]->pixmap().toImage();
                            for(int j = 0; j < sceneHeight; ++j)
                            {
                                for(int k = 0; k < sceneWidth; ++k)
                                {
                                    QColor PXA = QColor(imageA.pixel(k, j)), PXB = QColor(imageB.pixel(k, j));
                                    int R = MIN(PXA.red() + PXB.red(), 255);
                                    int G = MIN(PXA.green() + PXB.green(), 255);
                                    int B = MIN(PXA.blue() + PXB.blue(), 255);
                                    imageA.setPixel(k, j, QColor(R, G, B).rgb());
                                }
                            }

                            // Add the alpha pixmap above the non-blended layer 0, but below the next one to be rendered
                            QGraphicsPixmapItem *alphaItem = scene->addPixmap(alphaPixmap);
                            alphaItem->setZValue(Z++);
                            RenderedLayers[7] = alphaItem;
                        }
                        else RenderedLayers[7] = nullptr;
                    }
                }

                // TODO render entity layer

                // Render door layer
                QPixmap doorPixmap(sceneWidth, sceneHeight);
                doorPixmap.fill(Qt::transparent);
                QPainter doorPainter(&doorPixmap);
                QPen redPen = QPen(QBrush(Qt::blue), 2);
                redPen.setJoinStyle(Qt::MiterJoin);
                doorPainter.setPen(redPen);
                for(unsigned int i = 0; i < doors.size(); i++)
                {
                    int doorX = doors[i]->GetX1() * 16;
                    int doorY = doors[i]->GetY1() * 16;
                    int doorWidth = (qAbs(doors[i]->GetX1() - doors[i]->GetX2()) + 1) * 16;
                    int doorHeight = (qAbs(doors[i]->GetY1() - doors[i]->GetY2()) + 1) * 16;
                    doorPainter.drawRect(doorX, doorY, doorWidth, doorHeight);
                    doorPainter.fillRect(doorX + 1, doorY + 1, doorWidth - 2, doorHeight - 2, QColor(0, 0, 0xFF, 0x5F));
                }
                QGraphicsPixmapItem *doorpixmapItem = scene->addPixmap(doorPixmap);
                doorpixmapItem->setZValue(Z++);
                RenderedLayers[5] = doorpixmapItem;

                // TODO render camera box layer

            }
        case LayerEnable:
            {
                // Enable visibility of the foreground and background layers
                Ui::EditModeParams *layerVisibility = &(renderParams->mode);
                for(int i = 0; i < 4; ++i)
                {
                    RenderedLayers[i]->setVisible(layerVisibility->layersEnabled[i]);
                }

                // Enable the visibility of the sprite and editor overlay layers
                if(RenderedLayers[4]) RenderedLayers[4]->setVisible(layerVisibility->entitiesEnabled);
                if(RenderedLayers[5]) RenderedLayers[5]->setVisible(layerVisibility->doorsEnabled);
                if(RenderedLayers[6]) RenderedLayers[6]->setVisible(layerVisibility->cameraAreasEnabled);
                if(RenderedLayers[7]) RenderedLayers[7]->setVisible(layerVisibility->alphaBlendingEnabled);
            }
            return scene;
        case SingleTile:
            {
                // Re-render the QImage for the changed tile
                Layer *layer = layers[renderParams->mode.selectedLayer];
                layer->ReRenderTile(renderParams->tileX, renderParams->tileY, renderParams->tileID, tileset);

                // Obtain the old QPixmap from the previously-rendered graphic layers
                QGraphicsPixmapItem *item = RenderedLayers[renderParams->mode.selectedLayer];

                // Draw the new tile graphics over the position of the old tile in the QPixmap
                QPixmap pm(item->pixmap());
                int units = layer->GetMappingType() == LayerMap16 ? 16 : 8;
                int X = renderParams->tileX * units;
                int Y = renderParams->tileY * units;
                int tileDataIndex = renderParams->tileX + renderParams->tileY * layer->GetLayerWidth();
                layer->GetTiles()[tileDataIndex]->DrawTile(&pm, X, Y);

                // Set the new QPixmap for the graphics item on the QGraphicsScene
                item->setPixmap(pm);
            }
            return scene;
        }
        // ERROR
        return nullptr;
    }
}
