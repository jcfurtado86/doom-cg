#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <vector>
#include <string>

class MapLoader {
public:
    bool load(const char* filename);

    const std::vector<std::string>& data() const { return mapData; }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    float getPlayerStartX() const { return playerStartX; }
    float getPlayerStartZ() const { return playerStartZ; }

private:
    std::vector<std::string> mapData;
    int width = 0;
    int height = 0;

    float playerStartX = 1.0f;
    float playerStartZ = 1.0f;
};

#endif
