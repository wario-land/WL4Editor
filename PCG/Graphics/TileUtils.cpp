#include "TileUtils.h"

#include "ROMUtils.h"

#ifndef WINDOW_INSTANCE_SINGLETON
#define WINDOW_INSTANCE_SINGLETON
#include "WL4EditorWindow.h"
extern WL4EditorWindow *singleton;
#endif

PCG::GFXUtils::TileUtils::TileUtils(QObject *parent) : QObject{parent}
{
    // installation
}

int PCG::GFXUtils::TileUtils::GetFitness_CurTilesetJoinTile16_UL(unsigned int upper_tile16_id, unsigned int lower_tile16_id)
{
    int tileset_id = singleton->GetCurrentRoom()->GetTilesetID();
    LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[tileset_id];
    auto tile16s = tileset->GetMap16arrayPtr();
    return GetFitnessJoinTile16_UL(tile16s[upper_tile16_id], tile16s[lower_tile16_id]);
}

int PCG::GFXUtils::TileUtils::GetFitness_CurTilesetJoinTile16_LR(unsigned int left_tile16_id, unsigned int right_tile16_id)
{
    int tileset_id = singleton->GetCurrentRoom()->GetTilesetID();
    LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[tileset_id];
    auto tile16s = tileset->GetMap16arrayPtr();
    return GetFitnessJoinTile16_LR(tile16s[left_tile16_id], tile16s[right_tile16_id]);
}

/// <summary>
/// Check if an upper tile16 and a lower tile16 can be joined together.
/// </summary>
/// <returns>
/// return a fitness value of the join result.
/// a small fitness value means they tend to be join-able,
/// a big fitness value means they tend to be not join-able.
/// </returns>
int PCG::GFXUtils::TileUtils::GetFitnessJoinTile16_UL(LevelComponents::TileMap16 *Upper_tile16, LevelComponents::TileMap16 *Lower_tile16)
{
    QPixmap pixmap(16, 16 * 2);
    pixmap.fill(Qt::transparent);
    Upper_tile16->DrawTile(&pixmap, 0, 0);
    Lower_tile16->DrawTile(&pixmap, 0, 16);

    // get the color difference between the bottom row of the upper tile16 and the top row of the lower tile16 per pixel
    QColor border_u[16], border_l[16];
    int delta_r[16], delta_g[16], delta_b[16];
    QImage image = pixmap.toImage();
    for (int i = 0; i < 16; i++)
    {
        border_u[i] = image.pixel(i, 15);
        border_l[i] = image.pixel(i, 16);
        delta_r[i] = border_u[i].red() - border_l[i].red();
        delta_g[i] = border_u[i].green() - border_l[i].green();
        delta_b[i] = border_u[i].blue() - border_l[i].blue();
    }

    // the check if the 16 differences pixel pair looks similar in changes from the upper tile16 to lower tile16
    double ave_r = 0.0, ave_b = 0.0, ave_g = 0.0;
    for (int i = 0; i < 16; i++)
    {
        ave_r += (double)delta_r[i];
        ave_g += (double)delta_g[i];
        ave_b += (double)delta_b[i];
    }
    ave_r = ave_r / 16.0;
    ave_g = ave_g / 16.0;
    ave_b = ave_b / 16.0;
    double diff_r = 0.0, diff_b = 0.0, diff_g = 0.0;
    for (int i = 0; i < 16; i++)
    {
        diff_r += (double)delta_r[i] - ave_r;
        diff_g += (double)delta_g[i] - ave_g;
        diff_b += (double)delta_b[i] - ave_b;
    }
    return diff_r + diff_b + diff_g;
}

/// <summary>
/// Check if a left tile16 and a right tile16 can be joined together.
/// </summary>
/// <returns>
/// return a fitness value of the join result.
/// a small fitness value means they tend to be join-able,
/// a big fitness value means they tend to be not join-able.
/// </returns>
int PCG::GFXUtils::TileUtils::GetFitnessJoinTile16_LR(LevelComponents::TileMap16 *Left_tile16, LevelComponents::TileMap16 *Right_tile16)
{
    QPixmap pixmap(16 * 2, 16);
    pixmap.fill(Qt::transparent);
    Left_tile16->DrawTile(&pixmap, 0, 0);
    Right_tile16->DrawTile(&pixmap, 16, 0);

    // get the color difference between the bottom row of the upper tile16 and the top row of the lower tile16 per pixel
    QColor border_l[16], border_r[16];
    int delta_r[16], delta_g[16], delta_b[16];
    QImage image = pixmap.toImage();
    for (int i = 0; i < 16; i++)
    {
        border_l[i] = image.pixel(15, i);
        border_r[i] = image.pixel(16, i);
        delta_r[i] = border_l[i].red() - border_r[i].red();
        delta_g[i] = border_l[i].green() - border_r[i].green();
        delta_b[i] = border_l[i].blue() - border_r[i].blue();
    }

    // the check if the 16 differences pixel pair looks similar in changes from the upper tile16 to lower tile16
    double ave_r = 0.0, ave_b = 0.0, ave_g = 0.0;
    for (int i = 0; i < 16; i++)
    {
        ave_r += (double)delta_r[i];
        ave_g += (double)delta_g[i];
        ave_b += (double)delta_b[i];
    }
    ave_r = ave_r / 16.0;
    ave_g = ave_g / 16.0;
    ave_b = ave_b / 16.0;
    double diff_r = 0.0, diff_b = 0.0, diff_g = 0.0;
    for (int i = 0; i < 16; i++)
    {
        diff_r += (double)delta_r[i] - ave_r;
        diff_g += (double)delta_g[i] - ave_g;
        diff_b += (double)delta_b[i] - ave_b;
    }
    return diff_r + diff_b + diff_g;
}
