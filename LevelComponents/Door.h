#ifndef DOOR_H
#define DOOR_H


class Door
{
public:
    Door::Door(char *doorData);
    enum LevelComponents::DoorType type;
    int RoomID;
    int X1, X2, Y1, Y2;
    int X_Destination, Y_Destination;
    int DestinationDoor; // index into currently loaded Door table
    int X_Displacement, Y_Displacement;
    int SpriteMapID;
    int BGM_ID;
};

#endif // DOOR_H
