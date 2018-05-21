#ifndef LEVEL_H
#define LEVEL_H

class Level
{
public:
    Level::Level(char *levelHeader);
    Level::Level(int passage, int stage);
    Room rooms[];
    std::string LevelName;
    std::vector<Door> doors;
};

#endif // LEVEL_H
