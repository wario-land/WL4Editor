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
    /// <param name="roomDataPtr">
    /// Pointer to the start of the room data header.
    /// </param>
    /// <param name="_RoomID">
    /// Zero-based ID for the room in the level.
    /// </param>
    /// <param name="_LevelID">
    /// Level index value from 0x03000023 at run-time.
    /// </param>
    Room::Room(int roomDataPtr, unsigned char _RoomID, unsigned int _LevelID) :
        RoomID(_RoomID),
        TilesetID(ROMUtils::CurrentFile[roomDataPtr])
    {
        memset(RenderedLayers, 0, sizeof(RenderedLayers));

        // Copy the room header information
        memcpy(&RoomHeader, ROMUtils::CurrentFile + roomDataPtr, sizeof(struct __RoomHeader));

        // Set up tileset
        int tilesetPtr = WL4Constants::TilesetDataTable + TilesetID * 36;
        tileset = new Tileset(tilesetPtr, TilesetID);

        // Set up the layer data
        int dimensionPointer = ROMUtils::PointerFromData(roomDataPtr + 12);
        Width = ROMUtils::CurrentFile[dimensionPointer];
        Height = ROMUtils::CurrentFile[dimensionPointer + 1];
        for(int i = 0; i < 4; ++i)
        {
            enum LayerMappingType mappingType = static_cast<enum LayerMappingType>(ROMUtils::CurrentFile[roomDataPtr + i + 1] & 0x30);
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
        for(int i = 0; i < 3; i++)
        {
            int Listaddress = ROMUtils::PointerFromData(roomDataPtr + 28 + 4 * i);
            int k = 0;
            while(ROMUtils::CurrentFile[Listaddress + 3 * k] != (unsigned char) '\xFF') // maximum entity count is 46
            {
                EntityRoomAttribute tmpEntityroomattribute;
                tmpEntityroomattribute.YPos     = (int) ROMUtils::CurrentFile[Listaddress + 3 * k];
                tmpEntityroomattribute.XPos     = (int) ROMUtils::CurrentFile[Listaddress + 3 * k + 1];
                tmpEntityroomattribute.EntityID = (int) ROMUtils::CurrentFile[Listaddress + 3 * k + 2];
                EntityList[i].push_back(tmpEntityroomattribute);
                k++;
            }
        }

        // TODO
    }

    /// <summary>
    /// Render an entire graphics scene for the Room.
    /// </summary>
    /// <remarks>
    /// There are different ways to render the graphics for the room; these ways are defined in
    /// the LevelComponents::RenderUpdateType enum, and the parameters for it are stored in <paramref name="renderParams"/>.
    /// </remarks>
    /// <param name="scene">
    /// The graphics scene object which will be fully rendered, or contain pre-rendered graphics to re-render.
    /// </param>
    /// <param name="renderParams">
    /// A struct containing the parameters for how the scene should be rendered or re-rendered.
    /// </param>
    /// <return>
    /// A graphics scene containing fully rendered pixmap layers in proper Z order.
    /// </return>
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
                if(scene) { delete scene; } // Make a new graphics scene to draw to
                scene = new QGraphicsScene(0, 0, qMax(sceneWidth, 16 * this->GetLayer(0)->GetLayerWidth()), sceneHeight);
                int Z = 0;

                // This represents the EVA alpha layer, which will be rendered in passes before the alpha layer is finalized
                QPixmap alphaPixmap(sceneWidth, sceneHeight);
                QPainter alphaPainter(&alphaPixmap);

                // Render the 4 layers in the order of their priority
                for(int i = 0; i < 4; ++i)
                {
                    QPixmap pixmap = drawLayers[i]->layer->RenderLayer(tileset);
                    // If this is a layer composed of 8x8 tiles, then repeat the layer in X and Y to the size of the other layers
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

                    // Add the rendered layer to the graphics scene
                    QGraphicsPixmapItem *pixmapItem = scene->addPixmap(pixmap);
                    pixmapItem->setZValue(Z++);
                    RenderedLayers[drawLayers[i]->index] = pixmapItem;

                    // Render alpha blended composite pixmap for layer 0 if alpha blending is enabled
                    if(Layer0ColorBlending && (Layer0ColorBlendCoefficient_EVB != 0))
                    {
                        // If this is a pass for a layer under the alpha layer, draw the rendered layer to the EVA component image
                        if((3 - i) > layers[0]->GetLayerPriority())
                            alphaPainter.drawImage(0, 0, RenderedLayers[drawLayers[i]->index]->pixmap().toImage());
                        else if((3 - i) == layers[0]->GetLayerPriority())
                        {
                            // Blend the EVA and EVB pixels for the new layer
                            QImage imageA = RenderedLayers[0]->pixmap().toImage();
                            QImage imageB = alphaPixmap.toImage();
                            for(int j = 0; j < sceneHeight; ++j)
                            {
                                for(int k = 0; k < sceneWidth; ++k)
                                {
                                    QColor PXA = QColor(imageA.pixel(k, j)), PXB = QColor(imageB.pixel(k, j));
                                    int R = MIN((Layer0ColorBlendCoefficient_EVA * PXA.red()) / 16 + (Layer0ColorBlendCoefficient_EVB * PXB.red()) / 16, 255);
                                    int G = MIN((Layer0ColorBlendCoefficient_EVA * PXA.green()) / 16 + (Layer0ColorBlendCoefficient_EVB * PXB.green()) / 16, 255);
                                    int B = MIN((Layer0ColorBlendCoefficient_EVA * PXA.blue()) / 16 + (Layer0ColorBlendCoefficient_EVB * PXB.blue()) / 16, 255);
                                    imageA.setPixel(k, j, QColor(R, G, B).rgb());
                                }
                            }

                            // Add the alpha pixmap above the non-blended layer 0, but below the next one to be rendered
                            QGraphicsPixmapItem *alphaItem = scene->addPixmap(QPixmap::fromImage(imageA));
                            alphaItem->setZValue(Z++);
                            RenderedLayers[7] = alphaItem;
                        };
                    }
                    else RenderedLayers[7] = nullptr;
                }

                // TODO render entity layer

                // Render door layer
                QPixmap doorPixmap(sceneWidth, sceneHeight);
                doorPixmap.fill(Qt::transparent);
                QPainter doorPainter(&doorPixmap);
                QPen DoorPen = QPen(QBrush(Qt::blue), 2);
                DoorPen.setJoinStyle(Qt::MiterJoin);
                doorPainter.setPen(DoorPen);
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

                // Render camera box layer
                QPixmap CameraLimitationPixmap(sceneWidth, sceneHeight);
                CameraLimitationPixmap.fill(Qt::transparent);
                QPainter CameraLimitationPainter(&CameraLimitationPixmap);
                CameraLimitationPainter.setRenderHint(QPainter::Antialiasing);
                QPen CameraLimitationPen = QPen(QBrush(Qt::red), 2);
                QPen CameraLimitationPen2 = QPen(QBrush(Qt::green), 2);
                CameraLimitationPen.setJoinStyle(Qt::MiterJoin);
                CameraLimitationPen2.setJoinStyle(Qt::MiterJoin);
                CameraLimitationPainter.setPen(CameraLimitationPen);

                if(CameraControlType == LevelComponents::FixedY)
                {
                    // Use Wario original position when getting out of a door to figure out the Camera Limitator Y position
                    // CameraY and WarioYPos here are 4 times the real values
                    int CameraY = 0x80 - 32;
                    int WarioYPos = doors[0]->GetWarioOriginalPosition_x4().y(); // Use the first door in the data
                    if(WarioYPos > 0x260)
                    {
                        do
                            CameraY += 0x240;
                        while(WarioYPos > (CameraY + 0x280));
                    }
                    // Force the value to be normal
                    CameraY = CameraY / 4;
                    // Get the first Camera limitator Y value
                    while (CameraY > 0xA0)
                        CameraY -= 0x90;
                    // Draw Camera Limitation
                    while ((CameraY + 0xA0) < (int) (Height * 16))
                    {
                        CameraLimitationPainter.drawRect(0x20, CameraY, (int) Width * 16 - 0x40, 0xA0);
                        CameraY += 0x90;
                    }
                }
                else if(CameraControlType == LevelComponents::NoLimit)
                {
                    CameraLimitationPainter.drawRect(0x20, 0x20, (int) Width * 16 - 0x40, (int) Height * 16 - 0x40);
                }
                else if(CameraControlType == LevelComponents::HasControlAttrs)
                {
                    for(unsigned int i = 0; i < CameraControlRecords.size(); i++)
                    {
                        CameraLimitationPainter.drawRect(
                            16 * ((int) CameraControlRecords[i]->x1) + 1,
                            16 * ((int) CameraControlRecords[i]->y1) + 1,
                            16 * (MIN((int) CameraControlRecords[i]->x2, (int) Width - 3) - (int) CameraControlRecords[i]->x1 + 1) - 2,
                            16 * (MIN((int) CameraControlRecords[i]->y2, (int) Height - 3) - (int) CameraControlRecords[i]->y1 + 1) - 2
                        );
                        if(CameraControlRecords[i]->x3 != (unsigned char) '\xFF')
                        {
                            // Draw a box around the block which triggers the camera box, and a line connecting it
                            CameraLimitationPainter.drawRect(
                                16 * ((int) CameraControlRecords[i]->x3) + 2,
                                16 * ((int) CameraControlRecords[i]->y3) + 2,
                                12,
                                12
                            );
                            CameraLimitationPainter.drawLine(
                                16 * ((int) CameraControlRecords[i]->x1) + 1,
                                16 * ((int) CameraControlRecords[i]->y1) + 1,
                                16 * ((int) CameraControlRecords[i]->x3) + 2,
                                16 * ((int) CameraControlRecords[i]->y3) + 2
                            );
                            CameraLimitationPainter.setPen(CameraLimitationPen2);
                            int SetNum[4] =
                            {
                                (int) CameraControlRecords[i]->x1,
                                (int) CameraControlRecords[i]->x2,
                                (int) CameraControlRecords[i]->y1,
                                (int) CameraControlRecords[i]->y2
                            };
                            int k = (int) CameraControlRecords[i]->ChangeValueOffset;
                            SetNum[k] = (int) CameraControlRecords[i]->ChangedValue;
                            CameraLimitationPainter.drawRect(
                                16 * SetNum[0],
                                16 * SetNum[2],
                                16 * (MIN(SetNum[1], (int) Width - 3) - SetNum[0] + 1),
                                16 * (MIN(SetNum[3], (int) Height - 3) - SetNum[2] + 1)
                            );
                            CameraLimitationPainter.setPen(CameraLimitationPen);
                        }
                    }
                }
                else
                {
                    // TODO other camera control type
                }

                QGraphicsPixmapItem *CameraLimitationpixmapItem = scene->addPixmap(CameraLimitationPixmap);
                CameraLimitationpixmapItem->setZValue(Z++);
                RenderedLayers[6] = CameraLimitationpixmapItem;
            }
            // Fall through to layer enable section
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

    /// <summary>
    /// Get the layer data pointer for a layer.
    /// </summary>
    /// <remarks>
    /// The pointer starts in the 0x8000000 range, so the 28th bit is set to 0 to normalize the address.
    /// </remarks>
    /// <param name="LayerNum">
    /// The number of the layer to retrieve the data pointer for.
    /// </param>
    /// <return>
    /// The normalized data pointer for the requested layer.
    /// </return>
    int Room::GetLayersDataPtr(int LayerNum)
    {
        switch(LayerNum)
        {
            case 0:
                return RoomHeader.Layer0Data & 0x7FFFFFF;
            case 1:
                return RoomHeader.Layer1Data & 0x7FFFFFF;
            case 2:
                return RoomHeader.Layer2Data & 0x7FFFFFF;
            case 3:
                return RoomHeader.Layer3Data & 0x7FFFFFF;
            default:
                // ERROR
                return 0;
        }
    }
}
