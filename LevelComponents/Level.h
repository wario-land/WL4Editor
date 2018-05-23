#ifndef LEVEL_H
#define LEVEL_H

#include "Room.h"
#include "Door.h"
#include <string>
#include <vector>

namespace LevelComponents
{
    class Level
    {
    private:
        std::vector<Room> rooms;
        std::string LevelName;
        std::vector<Door*> doors;
    public:
        Level(int passage, int stage);
    };
}

#endif // LEVEL_H
