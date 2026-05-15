#include "Operation.h"
#include "WL4EditorWindow.h"
#include "WL4Constants.h"
#include "ROMUtils.h"

#include <deque>

extern WL4EditorWindow *singleton;

// Globals used by the undo system
static std::deque<struct OperationParams *> operationHistory;
static unsigned int operationIndex;

static unsigned int CurrentTilesetOperationId = 0;
static unsigned int CurrentSpritestuffOperationId = 0;
static unsigned int CurrentAnimatedTileGroupOperationId = 0;

/// <summary>
/// Perform an operation based on its parameters.
/// </summary>
/// <remarks>
/// This function does not take into consideration the undo deque. It only performs an operation.
/// </remarks>
/// <param name="operation">
/// The operation to perform.
/// </param>
void PerformOperation(struct OperationParams *operation)
{
    LevelComponents::Room *room;
    if (operation->tileChange)
    {
        room = singleton->GetCurrentLevel()->GetRooms()[operation->tileChangeRoomID];
        int tl1 = -1, tl2 = -1; // there are 2 target layers only when doing cross layer rect-copy
        QVector<LevelComponents::Tileinfo> tilechangelist, tilechangelist2;
        for (auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            unsigned int index;
            if (!tcp->targetLayer) // i.e. targetLayer is Layer 0
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer0Width();
            }
            else
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer1Width();
            }
            layer->GetLayerData()[index] = tcp->newTile;

            if(tl1 == -1)
            {
                tl1 = tcp->targetLayer;
            }
            if(tl1 != -1 && tl2 == -1 && tl1 != tcp->targetLayer)
            {
                tl2 = tcp->targetLayer;
            }
            struct LevelComponents::Tileinfo tinfo;
            tinfo.tileX = tcp->tileX;
            tinfo.tileY = tcp->tileY;
            tinfo.tileID = tcp->newTile;
            if(tl1 == tcp->targetLayer)
            {
                tilechangelist.push_back(tinfo);
                continue;
            }
            if(tl2 == tcp->targetLayer)
            {
                tilechangelist2.push_back(tinfo);
                continue;
            }
        }
        // Update graphic changes
        singleton->RenderScreenTilesChange(tilechangelist, tl1);
        if(tl2 == -1) return;
        singleton->RenderScreenTilesChange(tilechangelist2, tl2);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo tile changes."));
    }
    if (operation->layer0Change)
    {
        auto *p = operation->layer0ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer0 = room->GetLayer(0);
        if (layer0->GetLayerWidth() == p->layerWidth && layer0->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer0->GetLayerData(), p->newLayerData, 2 * p->layerWidth * p->layerHeight);
            layer0->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Layer 0 changes."));
    }
    if (operation->layer1Change)
    {
        auto *p = operation->layer1ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer1 = room->GetLayer(1);
        if (layer1->GetLayerWidth() == p->layerWidth && layer1->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer1->GetLayerData(), p->newLayerData, 2 * p->layerWidth * p->layerHeight);
            layer1->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Layer 1 changes."));
    }
    if (operation->layer2Change)
    {
        auto *p = operation->layer2ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer2 = room->GetLayer(2);
        if (layer2->GetLayerWidth() == p->layerWidth && layer2->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer2->GetLayerData(), p->newLayerData, 2 * p->layerWidth * p->layerHeight);
            layer2->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Layer 2 changes."));
    }
    if (operation->roomConfigChange)
    {
        // change the width and height for all layers
        singleton->RoomConfigReset(operation->lastRoomConfigParams, operation->newRoomConfigParams);
        singleton->RenderScreenFull();
        singleton->SetEditModeDockWidgetLayerEditability();
        singleton->SetEditModeWidgetDifficultyRadioBox(1);
        singleton->ResetEntitySetDockWidget();
        singleton->ResetCameraControlDockWidget();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Room Config changes."));
    }
    if (operation->objectPositionChange)
    {
        struct ObjectMoveParams *om=operation->objectMoveParams;
        LevelComponents::Room *currentRoom = singleton->GetCurrentLevel()->GetRooms()[om->roomID];

        // If the entity exists
        if (om->objectID != -1)
        {
            if (currentRoom->IsNewEntityPositionInsideRoom(om->nextPositionX, om->nextPositionY))
            {
                currentRoom->SetEntityPosition(om->nextPositionX, om->nextPositionY, om->objectID);
                if (singleton->GetCurrentRoom()->GetRoomID() == om->roomID)
                    singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, om->objectID);
                int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
                currentRoom->SetEntityListDirty(difficulty, true);
                singleton->SetUnsavedChanges(true);
                singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Entity move changes."));
            }
        }
    }
    if (operation->doorMoveChange)
    {
        struct DoorMoveParams *dm = operation->doorMoveParams;
        LevelComponents::LevelDoorVector &tmpDoorVec = singleton->GetCurrentLevel()->GetDoorListRef();
        int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(dm->roomID, dm->objectID);
        auto curDoor = tmpDoorVec.GetDoor(globalDoorId);

        int deltaX = curDoor.x2 - curDoor.x1;
        int deltaY = curDoor.y2 - curDoor.y1;

        if (dm->objectID != -1 && curDoor.DoorTypeByte)
        {
            LevelComponents::Room *doorRoom = singleton->GetCurrentLevel()->GetRooms()[dm->roomID];
            if (doorRoom->IsNewDoorPositionInsideRoom(dm->nextPositionX, dm->nextPositionX + deltaX, dm->nextPositionY, dm->nextPositionY + deltaY))
            {
                tmpDoorVec.SetDoorPlace(globalDoorId,
                                         dm->nextPositionX, dm->nextPositionX + deltaX,
                                         dm->nextPositionY, dm->nextPositionY + deltaY);
                if (singleton->GetCurrentRoom()->GetRoomID() == dm->roomID)
                    singleton->RenderScreenElementsLayersUpdate((unsigned int) dm->objectID, -1);
                singleton->SetUnsavedChanges(true);
            }
        }
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo door move changes."));
    }
    if (operation->entityAdd)
    {
        auto *ep = operation->entityAddParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        bool success = room->AddEntity(ep->XPos, ep->YPos, ep->EntityTypeLocalID, ep->difficulty);
        if(!success)
        {
            singleton->GetOutputWidgetPtr()->PrintString("Cannot add more entity under the current difficulty in this room");
            return;
        }
        room->SetEntityListDirty(ep->difficulty, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo entity add."));
    }
    if (operation->entityDelete)
    {
        auto *ep = operation->entityDeleteParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        auto list = room->GetEntityListData(ep->difficulty);
        for (unsigned int i = 0; i < list.size(); ++i)
        {
            if (list[i].XPos == ep->XPos && list[i].YPos == ep->YPos && list[i].EntityID == ep->EntityTypeLocalID)
            {
                room->DeleteEntity(ep->difficulty, i);
                break;
            }
        }
        room->SetEntityListDirty(ep->difficulty, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo entity delete."));
    }
    if (operation->entityNormalChange)
    {
        auto *ep = operation->entityNormalChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(1, ep->newEntityList);
        room->SetEntityListDirty(1, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Entity Normal list changes."));
    }
    if (operation->entityHardChange)
    {
        auto *ep = operation->entityHardChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(0, ep->newEntityList);
        room->SetEntityListDirty(0, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Entity Hard list changes."));
    }
    if (operation->entitySHardChange)
    {
        auto *ep = operation->entitySHardChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(2, ep->newEntityList);
        room->SetEntityListDirty(2, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Entity S-Hard list changes."));
    }
    if (operation->doorVectorChange)
    {
        auto *dv = operation->doorVectorChangeParams;
        singleton->GetCurrentLevel()->SetDoorVec(*(dv->newDoorVec));
        // Sync each room's entity set from its first door
        int numRooms = singleton->GetCurrentLevel()->GetRooms().size();
        for (int i = 0; i < numRooms; ++i)
        {
            LevelComponents::Room *r = singleton->GetCurrentLevel()->GetRooms()[i];
            auto rDoors = singleton->GetCurrentLevel()->GetDoorListRef().GetDoorsByRoomID((unsigned char) i);
            if (!rDoors.isEmpty())
                r->SetCurrentEntitySet(rDoors[0].EntitySetID);
        }
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->ResetEntitySetDockWidget();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo door changes."));
    }
    if (operation->CreditChange)
    {
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
               operation->newCreditsEditParams->newCreditData,
               NUMBEROFCREDITSSCREEN * 1280);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo credit changes."));
    }
    if (operation->levelConfigChange)
    {
        LevelComponents::Level *level = singleton->GetCurrentLevel();
        level->SetLevelName(operation->newLevelConfigParams->newLevelName);
        level->SetLevelName(operation->newLevelConfigParams->newLevelNameJ, 1);
        level->SetTimeCountdownCounter(LevelComponents::HardDifficulty,
                                        (unsigned int) operation->newLevelConfigParams->newHModeTimer);
        level->SetTimeCountdownCounter(LevelComponents::NormalDifficulty,
                                        (unsigned int) operation->newLevelConfigParams->newNModeTimer);
        level->SetTimeCountdownCounter(LevelComponents::SHardDifficulty,
                                        (unsigned int) operation->newLevelConfigParams->newSHModeTimer);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo level config changes."));
    }
    if (operation->TilesetChange)
    {
        // Update Rooms's Tileset in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        int tilesetId = operation->newTilesetEditParams->currentTilesetIndex;
        ROMUtils::singletonTilesets[tilesetId] = operation->newTilesetEditParams->newTileset;
        for(int i = 0; i < roomnum; ++i)
        {
            if(singleton->GetCurrentLevel()->GetRooms()[i]->GetTilesetID() == tilesetId)
            {
                singleton->GetCurrentLevel()->GetRooms()[i]->SetTileset(operation->newTilesetEditParams->newTileset, tilesetId);
            }
        }

        // Update the Tileset using the current ROMUtils::animatedTileGroups instances
        ROMUtils::singletonTilesets[tilesetId]->UpdateAllAnimatedTileFromGlobalSingletons();

        // UI update if needed
        if (singleton->GetCurrentRoom()->GetTilesetID() == tilesetId)
        {
            singleton->GetTile16DockWidgetPtr()->SetTileset(tilesetId);
            singleton->RenderScreenFull();
        }
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Tileset changes."));
    }
    if (operation->SpritesSpritesetChange)
    {
        // Update new Entities and Entitysets to global singeltons
        for (LevelComponents::Entity *entityIter: operation->newSpritesAndSetParam->entities)
        {
            ROMUtils::entities[entityIter->GetEntityGlobalID()] = entityIter;
        }
        for (LevelComponents::EntitySet *entitySetIter: operation->newSpritesAndSetParam->entitySets)
        {
            ROMUtils::entitiessets[entitySetIter->GetEntitySetId()] = entitySetIter;
        }

        // Update Rooms's Entities and Entitysets in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        for(int i = 0; i < roomnum; ++i)
        {
            LevelComponents::Room *curRoom = singleton->GetCurrentLevel()->GetRooms()[i];
            curRoom->SetCurrentEntitySet(curRoom->GetCurrentEntitySetID());
        }

        singleton->ResetEntitySetDockWidget();
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Sprites and Spritesets changes."));
    }
    if (operation->AnimatedTileGroupChange)
    {
        // update all animated tile group to global singletons
        for (LevelComponents::AnimatedTile8x8Group *&animatedTileGroupIter : operation->newAnimatedTileEditParam->animatedTileGroups)
        {
            ROMUtils::animatedTileGroups[animatedTileGroupIter->GetGlobalID()] = animatedTileGroupIter;
        }

        // Update all the Tilesets using the current ROMUtils::animatedTileGroups instances
        for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets)) / sizeof(ROMUtils::singletonTilesets[0]); ++i)
        {
            ROMUtils::singletonTilesets[i]->UpdateAllAnimatedTileFromGlobalSingletons();
        }

        singleton->GetTile16DockWidgetPtr()->SetTileset(singleton->GetCurrentRoom()->GetTilesetID());
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Animated Tile Group changes."));
    }
    if (operation->WallPaintChange)
    {
        auto *wp = operation->wallPaintChangeParams;
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], wp->newGFXData, 1024 * 5 * 6);
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], wp->newPassageColor, 32 * 5 * 6);
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], wp->newPassageGray, 32 * 5 * 6);
        for (auto *block : wp->scatteredBlocks)
            memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[block->romAddr], block->newData, block->size);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Wall Paint changes."));
    }
    if (operation->cameraControlChange)
    {
        auto *cp = operation->cameraControlChangeParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[cp->roomID];

        room->SetCameraControlType(cp->newCameraControlType);
        room->SetCameraControlRecords(cp->newCameraControlRecords);

        if (singleton->GetCurrentRoom()->GetRoomID() == cp->roomID)
        {
            singleton->ResetCameraControlDockWidget();
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        }

        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Perform/Redo Camera Control changes."));
    }
}

/// <summary>
/// Backtrack an operation based on its parameters.
/// </summary>
/// <remarks>
/// This function does not take into consideration the undo deque. It only resets the effect of an operation.
/// </remarks>
/// <param name="operation">
/// The operation to backtrack.
/// </param>
void BackTrackOperation(struct OperationParams *operation)
{
    LevelComponents::Room *room;
    if (operation->tileChange)
    {
        room = singleton->GetCurrentLevel()->GetRooms()[operation->tileChangeRoomID];
        int tl1 = -1, tl2 = -1; // there are 2 target layers only when doing cross layer rect-copy
        QVector<LevelComponents::Tileinfo> tilechangelist, tilechangelist2;
        for (auto iter = operation->tileChangeParams.begin(); iter != operation->tileChangeParams.end(); ++iter)
        {
            struct TileChangeParams *tcp = *iter;
            LevelComponents::Layer *layer = room->GetLayer(tcp->targetLayer);
            unsigned int index;
            if (!tcp->targetLayer) // i.e. targetLayer is Layer 0
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer0Width();
            }
            else
            {
                index = tcp->tileX + tcp->tileY * room->GetLayer1Width();
            }
            layer->GetLayerData()[index] = tcp->oldTile;

            if(tl1 == -1)
            {
                tl1 = tcp->targetLayer;
            }
            if(tl1 != -1 && tl2 == -1 && tl1 != tcp->targetLayer)
            {
                tl2 = tcp->targetLayer;
            }
            struct LevelComponents::Tileinfo tinfo;
            tinfo.tileX = tcp->tileX;
            tinfo.tileY = tcp->tileY;
            tinfo.tileID = tcp->oldTile;
            if(tl1 == tcp->targetLayer)
            {
                tilechangelist.push_back(tinfo);
                continue;
            }
            if(tl2 == tcp->targetLayer)
            {
                tilechangelist2.push_back(tinfo);
                continue;
            }
        }
        // Update graphic changes
        singleton->RenderScreenTilesChange(tilechangelist, tl1);
        if(tl2 == -1) return;
        singleton->RenderScreenTilesChange(tilechangelist2, tl2);

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo tile changes."));
    }
    if (operation->layer0Change)
    {
        auto *p = operation->layer0ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer0 = room->GetLayer(0);
        if (layer0->GetLayerWidth() == p->layerWidth && layer0->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer0->GetLayerData(), p->oldLayerData, 2 * p->layerWidth * p->layerHeight);
            layer0->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Layer 0 changes."));
    }
    if (operation->layer1Change)
    {
        auto *p = operation->layer1ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer1 = room->GetLayer(1);
        if (layer1->GetLayerWidth() == p->layerWidth && layer1->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer1->GetLayerData(), p->oldLayerData, 2 * p->layerWidth * p->layerHeight);
            layer1->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Layer 1 changes."));
    }
    if (operation->layer2Change)
    {
        auto *p = operation->layer2ChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[p->roomID];
        auto *layer2 = room->GetLayer(2);
        if (layer2->GetLayerWidth() == p->layerWidth && layer2->GetLayerHeight() == p->layerHeight)
        {
            memcpy(layer2->GetLayerData(), p->oldLayerData, 2 * p->layerWidth * p->layerHeight);
            layer2->SetDirty(true);
        }
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Layer 2 changes."));
    }
    if (operation->roomConfigChange)
    {
        // new to last
        singleton->RoomConfigReset(operation->newRoomConfigParams, operation->lastRoomConfigParams);
        singleton->RenderScreenFull();
        singleton->SetEditModeDockWidgetLayerEditability();
        singleton->SetEditModeWidgetDifficultyRadioBox(1);
        singleton->ResetEntitySetDockWidget();

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Room Config changes."));
    }
    if (operation->objectPositionChange)
    {
        struct ObjectMoveParams *om = operation->objectMoveParams;
        LevelComponents::Room *currentRoom = singleton->GetCurrentLevel()->GetRooms()[om->roomID];

        // If the entity exists and if it is still in the room
        if (om->objectID != -1)
        {
            if (currentRoom->IsNewEntityPositionInsideRoom(om->previousPositionX, om->previousPositionY))
            {
                currentRoom->SetEntityPosition(om->previousPositionX, om->previousPositionY, om->objectID);
                if (singleton->GetCurrentRoom()->GetRoomID() == om->roomID)
                    singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, om->objectID);
                int difficulty = singleton->GetEditModeWidgetPtr()->GetEditModeParams().selectedDifficulty;
                currentRoom->SetEntityListDirty(difficulty, true);
                singleton->SetUnsavedChanges(true);
            }
        }


        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Entity move changes."));
    }
    if (operation->doorMoveChange)
    {
        struct DoorMoveParams *dm = operation->doorMoveParams;
        LevelComponents::LevelDoorVector &tmpDoorVec = singleton->GetCurrentLevel()->GetDoorListRef();
        int globalDoorId = tmpDoorVec.GetGlobalIDByLocalID(dm->roomID, dm->objectID);
        auto curDoor = tmpDoorVec.GetDoor(globalDoorId);

        int deltaX = curDoor.x2 - curDoor.x1;
        int deltaY = curDoor.y2 - curDoor.y1;

        if (dm->objectID != -1 && curDoor.DoorTypeByte)
        {
            LevelComponents::Room *doorRoom = singleton->GetCurrentLevel()->GetRooms()[dm->roomID];
            if (doorRoom->IsNewDoorPositionInsideRoom(dm->previousPositionX, dm->previousPositionX + deltaX, dm->previousPositionY, dm->previousPositionY + deltaY))
            {
                tmpDoorVec.SetDoorPlace(globalDoorId,
                                         dm->previousPositionX, dm->previousPositionX + deltaX,
                                         dm->previousPositionY, dm->previousPositionY + deltaY);
                if (singleton->GetCurrentRoom()->GetRoomID() == dm->roomID)
                    singleton->RenderScreenElementsLayersUpdate((unsigned int) dm->objectID, -1);
                singleton->SetUnsavedChanges(true);
            }
        }
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo door move changes."));
    }
    if (operation->entityAdd)
    {
        auto *ep = operation->entityAddParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        std::vector<struct LevelComponents::EntityRoomAttribute> list = room->GetEntityListData(ep->difficulty);
        for (int i = (int)list.size() - 1; i >= 0; --i)
        {
            if (list[i].XPos == ep->XPos && list[i].YPos == ep->YPos && list[i].EntityID == ep->EntityTypeLocalID)
            {
                room->DeleteEntity(ep->difficulty, i);
                break;
            }
        }
        room->SetEntityListDirty(ep->difficulty, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo entity add."));
    }
    if (operation->entityDelete)
    {
        auto *ep = operation->entityDeleteParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->AddEntity(ep->XPos, ep->YPos, ep->EntityTypeLocalID, ep->difficulty);
        room->SetEntityListDirty(ep->difficulty, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate(0xFFFFFFFFu, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo entity delete."));
    }
    if (operation->entityNormalChange)
    {
        auto *ep = operation->entityNormalChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(1, ep->oldEntityList);
        room->SetEntityListDirty(1, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Entity Normal list changes."));
    }
    if (operation->entityHardChange)
    {
        auto *ep = operation->entityHardChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(0, ep->oldEntityList);
        room->SetEntityListDirty(0, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Entity Hard list changes."));
    }
    if (operation->entitySHardChange)
    {
        auto *ep = operation->entitySHardChangeParams;
        room = singleton->GetCurrentLevel()->GetRooms()[ep->roomID];
        room->SetEntityListData(2, ep->oldEntityList);
        room->SetEntityListDirty(2, true);
        if (singleton->GetCurrentRoom()->GetRoomID() == ep->roomID)
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Entity S-Hard list changes."));
    }
    if (operation->doorVectorChange)
    {
        auto *dv = operation->doorVectorChangeParams;
        singleton->GetCurrentLevel()->SetDoorVec(*(dv->oldDoorVec));
        // Sync each room's entity set from its first door
        int numRooms = singleton->GetCurrentLevel()->GetRooms().size();
        for (int i = 0; i < numRooms; ++i)
        {
            LevelComponents::Room *r = singleton->GetCurrentLevel()->GetRooms()[i];
            auto rDoors = singleton->GetCurrentLevel()->GetDoorListRef().GetDoorsByRoomID((unsigned char) i);
            if (!rDoors.isEmpty())
                r->SetCurrentEntitySet(rDoors[0].EntitySetID);
        }
        singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        singleton->ResetEntitySetDockWidget();
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo door changes."));
    }
    if (operation->CreditChange)
    {
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::CreditsTiles],
               operation->lastCreditsEditParams->oldCreditData,
               NUMBEROFCREDITSSCREEN * 1280);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo credit changes."));
    }
    if (operation->levelConfigChange)
    {
        LevelComponents::Level *level = singleton->GetCurrentLevel();
        level->SetLevelName(operation->lastLevelConfigParams->oldLevelName);
        level->SetLevelName(operation->lastLevelConfigParams->oldLevelNameJ, 1);
        level->SetTimeCountdownCounter(LevelComponents::HardDifficulty,
                                        (unsigned int) operation->lastLevelConfigParams->oldHModeTimer);
        level->SetTimeCountdownCounter(LevelComponents::NormalDifficulty,
                                        (unsigned int) operation->lastLevelConfigParams->oldNModeTimer);
        level->SetTimeCountdownCounter(LevelComponents::SHardDifficulty,
                                        (unsigned int) operation->lastLevelConfigParams->oldSHModeTimer);
        singleton->SetUnsavedChanges(true);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo level config changes."));
    }
    if (operation->TilesetChange)
    {
        // Update Rooms's Tileset in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        int tilesetId = operation->lastTilesetEditParams->currentTilesetIndex;
        ROMUtils::singletonTilesets[tilesetId] = operation->lastTilesetEditParams->newTileset;
        for(int i = 0; i < roomnum; ++i)
        {
            if(singleton->GetCurrentLevel()->GetRooms()[i]->GetTilesetID() == tilesetId)
            {
                singleton->GetCurrentLevel()->GetRooms()[i]->SetTileset(operation->lastTilesetEditParams->newTileset, tilesetId);
            }
        }

        // Update the Tileset using the current ROMUtils::animatedTileGroups instances
        ROMUtils::singletonTilesets[tilesetId]->UpdateAllAnimatedTileFromGlobalSingletons();

        // UI update if needed
        if (singleton->GetCurrentRoom()->GetTilesetID() == tilesetId)
        {
            singleton->GetTile16DockWidgetPtr()->SetTileset(tilesetId);
            singleton->RenderScreenFull();
        }
        CurrentTilesetOperationId = operationIndex;

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Tileset changes."));
    }
    if (operation->SpritesSpritesetChange)
    {
        // Update old Entities and Entitysets to global singeltons
        for (LevelComponents::Entity *entityIter: operation->lastSpritesAndSetParam->entities)
        {
            ROMUtils::entities[entityIter->GetEntityGlobalID()] = entityIter;
        }
        for (LevelComponents::EntitySet *entitySetIter: operation->lastSpritesAndSetParam->entitySets)
        {
            ROMUtils::entitiessets[entitySetIter->GetEntitySetId()] = entitySetIter;
        }

        // Update Rooms's Entities and Entitysets in CurrentLevel
        int roomnum = singleton->GetCurrentLevel()->GetRooms().size();
        for(int i = 0; i < roomnum; ++i)
        {
            LevelComponents::Room *curRoom = singleton->GetCurrentLevel()->GetRooms()[i];
            curRoom->SetCurrentEntitySet(curRoom->GetCurrentEntitySetID());
        }

        singleton->GetEntitySetDockWidgetPtr()->ResetEntitySet(singleton->GetCurrentRoom());
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        CurrentSpritestuffOperationId = operationIndex;

        // hint to show undo operation
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Sprites and Spritesets changes."));
    }
    if (operation->AnimatedTileGroupChange)
    {
        // update all animated tile group to global singletons
        for (LevelComponents::AnimatedTile8x8Group *&animatedTileGroupIter : operation->lastAnimatedTileEditParam->animatedTileGroups)
        {
            ROMUtils::animatedTileGroups[animatedTileGroupIter->GetGlobalID()] = animatedTileGroupIter;
        }

        // Update all the Tilesets using the current ROMUtils::animatedTileGroups instances
        for(int i = 0; i < (sizeof(ROMUtils::singletonTilesets)) / sizeof(ROMUtils::singletonTilesets[0]); ++i)
        {
            ROMUtils::singletonTilesets[i]->UpdateAllAnimatedTileFromGlobalSingletons();
        }

        singleton->GetTile16DockWidgetPtr()->SetTileset(singleton->GetCurrentRoom()->GetTilesetID());
        singleton->RenderScreenFull();
        singleton->SetUnsavedChanges(true);
        CurrentAnimatedTileGroupOperationId = operationIndex;
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Animated Tile Group changes."));
    }
    if (operation->WallPaintChange)
    {
        auto *wp = operation->wallPaintChangeParams;
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintGFXAddr], wp->oldGFXData, 1024 * 5 * 6);
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageColor], wp->oldPassageColor, 32 * 5 * 6);
        memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[WL4Constants::WallPaintPalPassageGray], wp->oldPassageGray, 32 * 5 * 6);
        for (auto *block : wp->scatteredBlocks)
            memcpy(&ROMUtils::ROMFileMetadata->ROMDataPtr[block->romAddr], block->oldData, block->size);
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Wall Paint changes."));
    }
    if (operation->cameraControlChange)
    {
        auto *cp = operation->cameraControlChangeParams;
        LevelComponents::Room *room = singleton->GetCurrentLevel()->GetRooms()[cp->roomID];

        room->SetCameraControlType(cp->oldCameraControlType);
        room->SetCameraControlRecords(cp->oldCameraControlRecords);

        if (singleton->GetCurrentRoom()->GetRoomID() == cp->roomID)
        {
            singleton->ResetCameraControlDockWidget();
            singleton->RenderScreenElementsLayersUpdate((unsigned int) -1, -1);
        }

        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("Undo Camera Control changes."));
    }
}

/// <summary>
/// Perform an operation based on its parameters, and add it to the undo deque.
/// </summary>
/// <param name="operation">
/// The operation to perform.
/// </param>
/// <param name="operationHist">
/// The the history deque from which to execute an operation.
/// </param>
/// <param name="operationIdx">
/// The operation indexer to modify.
/// </param>
void ExecuteOperation(struct OperationParams *operation)
{
    // Commit any pending camera control operation to maintain chronological order
    singleton->CommitPendingCameraOperation();

    PerformOperation(operation);
    // If we perform an action after a series of undo, then delete the "undone" operations from history
    while (operationIndex)
    {
        --operationIndex;
        struct OperationParams *frontOP = operationHistory[0];
        delete frontOP;
        operationHistory.pop_front();
    }
    operationHistory.push_front(operation);
    singleton->SetUnsavedChanges(true);
}

void UndoOperation()
{
    if (operationIndex < operationHistory.size())
    {
        BackTrackOperation(operationHistory[(operationIndex)++]);
        if (operationIndex == operationHistory.size())
        {
            // TODO uncomment this once all operations that change the level go through Operation.cpp
            // singleton->SetUnsavedChanges(false);
        }
    }
    else
    {
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("No more operation to undo."));
    }
}

/// <summary>
/// Redo previously undone operation from the undo deque.
/// </summary>
/// <remarks>
/// This function does not add the operation to the deque.
/// Instead, an index is used within the deque to track which operation should be redone next.
/// That way, an operation can be undone and redone multiple times.
/// </remarks>
/// <param name="operationHist">
/// The the history deque from which to undo an operation.
/// </param>
/// <param name="operationIdx">
/// The operation indexer to modify.
/// </param>
void RedoOperation()
{
    if (operationIndex)
    {
        PerformOperation(operationHistory[--operationIndex]);
        singleton->SetUnsavedChanges(true);
    }
    else
    {
        singleton->GetOutputWidgetPtr()->PrintString(QObject::tr("No more operation to redo."));
    }
}

/// <summary>
/// Reset the undo deque.
/// </summary>
/// <remarks>
/// This is necessary to ensure that undo history does not persist between multiple levels.
/// </remarks>
void ResetUndoHistory()
{
    for (unsigned int j = 0; j < operationHistory.size(); ++j)
    {
        if (operationHistory[j]->TilesetChange)
        {
            if (CurrentTilesetOperationId > j)
            {
                if (operationIndex <= j) // redo happened: new* is current global state, already freed by ~WL4EditorWindow()
                {
                    if (operationHistory[j]->newTilesetEditParams)
                    {
                        operationHistory[j]->newTilesetEditParams->newTileset = nullptr;
                        delete operationHistory[j]->newTilesetEditParams;
                        operationHistory[j]->newTilesetEditParams = nullptr;
                    }
                    if (operationHistory[j]->lastTilesetEditParams)
                    {
                        delete operationHistory[j]->lastTilesetEditParams->newTileset;
                        operationHistory[j]->lastTilesetEditParams->newTileset = nullptr;
                        delete operationHistory[j]->lastTilesetEditParams;
                        operationHistory[j]->lastTilesetEditParams = nullptr;
                    }
                    delete operationHistory[j];
                }
                else
                {
                    delete operationHistory[j];
                }
            }
            else
            {
                if (operationHistory[j]->newTilesetEditParams)
                    delete operationHistory[j]->newTilesetEditParams;
                if (operationHistory[j]->lastTilesetEditParams)
                {
                    delete operationHistory[j]->lastTilesetEditParams->newTileset;
                    operationHistory[j]->lastTilesetEditParams->newTileset = nullptr;
                    delete operationHistory[j]->lastTilesetEditParams;
                }
            }
        }
        else if (operationHistory[j]->SpritesSpritesetChange)
        {
            if (CurrentSpritestuffOperationId > j)
            {
                if (operationIndex <= j) // redo happened: new* is current global state, already freed by ~WL4EditorWindow()
                {
                    if (operationHistory[j]->newSpritesAndSetParam)
                    {
                        operationHistory[j]->newSpritesAndSetParam->entities.clear();
                        operationHistory[j]->newSpritesAndSetParam->entitySets.clear();
                        delete operationHistory[j]->newSpritesAndSetParam;
                        operationHistory[j]->newSpritesAndSetParam = nullptr;
                    }
                    if (operationHistory[j]->lastSpritesAndSetParam)
                    {
                        qDeleteAll(operationHistory[j]->lastSpritesAndSetParam->entities);
                        qDeleteAll(operationHistory[j]->lastSpritesAndSetParam->entitySets);
                        delete operationHistory[j]->lastSpritesAndSetParam;
                        operationHistory[j]->lastSpritesAndSetParam = nullptr;
                    }
                    delete operationHistory[j];
                }
                else
                {
                    delete operationHistory[j];
                }
            }
            else
            {
                if (operationHistory[j]->newSpritesAndSetParam)
                    delete operationHistory[j]->newSpritesAndSetParam;
                if (operationHistory[j]->lastSpritesAndSetParam)
                {
                    qDeleteAll(operationHistory[j]->lastSpritesAndSetParam->entities);
                    qDeleteAll(operationHistory[j]->lastSpritesAndSetParam->entitySets);
                    delete operationHistory[j]->lastSpritesAndSetParam;
                }
            }
        }
        else if (operationHistory[j]->AnimatedTileGroupChange)
        {
            if (CurrentAnimatedTileGroupOperationId > j)
            {
                if (operationIndex <= j) // redo happened: new* is current global state, already freed by ~WL4EditorWindow()
                {
                    if (operationHistory[j]->newAnimatedTileEditParam)
                    {
                        operationHistory[j]->newAnimatedTileEditParam->animatedTileGroups.clear();
                        delete operationHistory[j]->newAnimatedTileEditParam;
                        operationHistory[j]->newAnimatedTileEditParam = nullptr;
                    }
                    if (operationHistory[j]->lastAnimatedTileEditParam)
                    {
                        qDeleteAll(operationHistory[j]->lastAnimatedTileEditParam->animatedTileGroups);
                        delete operationHistory[j]->lastAnimatedTileEditParam;
                        operationHistory[j]->lastAnimatedTileEditParam = nullptr;
                    }
                    delete operationHistory[j];
                }
                else
                {
                    delete operationHistory[j];
                }
            }
            else
            {
                if (operationHistory[j]->newAnimatedTileEditParam)
                    delete operationHistory[j]->newAnimatedTileEditParam;
                if (operationHistory[j]->lastAnimatedTileEditParam)
                {
                    qDeleteAll(operationHistory[j]->lastAnimatedTileEditParam->animatedTileGroups);
                    delete operationHistory[j]->lastAnimatedTileEditParam;
                }
            }
        }
        else
        {
            delete operationHistory[j];
        }
    }
    operationHistory.clear();
    operationIndex = 0;
    CurrentTilesetOperationId = 0;
    CurrentSpritestuffOperationId = 0;
    CurrentAnimatedTileGroupOperationId = 0;
}

/// <summary>
/// Reset all the changes bools can be found through the whole history
/// </summary>
/// <remarks>
/// Sometimes the history saved the vanilla elements' data and we make some changes to it and save changes
/// now the current changed bool of the element is false and the first corresponding element in the history is false too
/// all the others corresponding elements' changed bools are true
/// usual undo and redo won't cause problems, but on the second or the third save
/// when the current element are marked unchanged, the save process cannot detect it.
/// So we have to always make it that only the current element changed bool to be false, and all the others be true.
/// call this function every time when finishing saving level
/// </remarks>
void ResetChangedBoolsThroughHistory()
{
    for (unsigned int j = 0; j < operationHistory.size(); ++j) // from old to new
    {
        // no need to exclude the current elements, they will be set changed bool to false in the SaveLevel function
        // so call this function before that part
        if (operationHistory[j]->TilesetChange)
        {
            operationHistory[j]->newTilesetEditParams->newTileset->SetChanged(true);
            operationHistory[j]->lastTilesetEditParams->newTileset->SetChanged(true);
        }
        else if (operationHistory[j]->SpritesSpritesetChange)
        {
            if (operationHistory[j]->newSpritesAndSetParam)
            {
                for (LevelComponents::Entity *entityIter: operationHistory[j]->newSpritesAndSetParam->entities)
                {
                    entityIter->SetChanged(true);
                }
                for (LevelComponents::EntitySet *entitySetIter: operationHistory[j]->newSpritesAndSetParam->entitySets)
                {
                    entitySetIter->SetChanged(true);
                }
            }
            if (operationHistory[j]->lastSpritesAndSetParam)
            {
                for (LevelComponents::Entity *entityIter: operationHistory[j]->lastSpritesAndSetParam->entities)
                {
                    entityIter->SetChanged(true);
                }
                for (LevelComponents::EntitySet *entitySetIter: operationHistory[j]->lastSpritesAndSetParam->entitySets)
                {
                    entitySetIter->SetChanged(true);
                }
            }
        }
        else if (operationHistory[j]->AnimatedTileGroupChange)
        {
            for (LevelComponents::AnimatedTile8x8Group *&animtedTileGroupIter: operationHistory[j]->lastAnimatedTileEditParam->animatedTileGroups)
            {
                animtedTileGroupIter->SetChanged(true);
            }
            for (LevelComponents::AnimatedTile8x8Group *&animtedTileGroupIter: operationHistory[j]->newAnimatedTileEditParam->animatedTileGroups)
            {
                animtedTileGroupIter->SetChanged(true);
            }
        }
    }
}

/// <summary>
/// Reset all the global elements operation indexes
/// </summary>
/// <remarks>
/// the ResetUndoHistory() function now resets all global element indexes back to 0,
/// by using this function, we can reset some global element operation indexes back to 0 when we need to load a new ROM
/// </remarks>
void ResetGlobalElementOperationIndexes()
{
    CurrentTilesetOperationId = 0;
    CurrentSpritestuffOperationId = 0;
    CurrentAnimatedTileGroupOperationId = 0;
}
