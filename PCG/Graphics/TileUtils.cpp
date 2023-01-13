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
    int tmp_result = GetFitnessJoinTile16_UL(tile16s[upper_tile16_id], tile16s[lower_tile16_id]);
    if (tmp_result == max_Tile16_border_pixels_color_channel_diff_value && (upper_tile16_id == 0 || lower_tile16_id == 0))
        return 0;
    return tmp_result;
}

int PCG::GFXUtils::TileUtils::GetFitness_CurTilesetJoinTile16_LR(unsigned int left_tile16_id, unsigned int right_tile16_id)
{
    int tileset_id = singleton->GetCurrentRoom()->GetTilesetID();
    LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[tileset_id];
    auto tile16s = tileset->GetMap16arrayPtr();
    int tmp_result = GetFitnessJoinTile16_LR(tile16s[left_tile16_id], tile16s[right_tile16_id]);
    if (tmp_result == max_Tile16_border_pixels_color_channel_diff_value && (left_tile16_id == 0 || right_tile16_id == 0))
        return 0;
    return tmp_result;
}

bool PCG::GFXUtils::TileUtils::IsBlankTile_CurTilesetTile16(unsigned int tile16_id)
{
    int tileset_id = singleton->GetCurrentRoom()->GetTilesetID();
    LevelComponents::Tileset *tileset = ROMUtils::singletonTilesets[tileset_id];
    auto tile16s = tileset->GetMap16arrayPtr();
    for (int pos = 0; pos < 4; pos++)
    {
        char *data = tile16s[tile16_id]->GetTile8X8(pos)->CreateGraphicsData().data();
        for (int i = 0; i < 32; i++)
        {
            if (data[i] != 0) return false;
        }
    }
    return true;
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
    double diff_r = 0.0, diff_b = 0.0, diff_g = 0.0;
    QImage image = pixmap.toImage();
    char l_black_pixel = 0;
    char u_black_pixel = 0;
    for (int i = 0; i < 16; i++)
    {
        border_u[i] = image.pixel(i, 15);
        border_l[i] = image.pixel(i, 16);
        if (border_l[i].red() < 8 && border_l[i].green() < 8 && border_l[i].blue() < 8)
        {
            l_black_pixel += 1;
        }
        if (border_u[i].red() < 8 && border_u[i].green() < 8 && border_u[i].blue() < 8)
        {
            u_black_pixel += 1;
        }
        if (l_black_pixel > 8 || u_black_pixel > 8)
            return max_Tile16_border_pixels_color_channel_diff_value;
        delta_r[i] = border_u[i].red() - border_l[i].red();
        delta_g[i] = border_u[i].green() - border_l[i].green();
        delta_b[i] = border_u[i].blue() - border_l[i].blue();
        diff_r += (double)delta_r[i];
        diff_g += (double)delta_g[i];
        diff_b += (double)delta_b[i];
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
    double diff_r = 0.0, diff_b = 0.0, diff_g = 0.0;
    QImage image = pixmap.toImage();
    char l_black_pixel = 0;
    char r_black_pixel = 0;
    for (int i = 0; i < 16; i++)
    {
        border_l[i] = image.pixel(15, i);
        border_r[i] = image.pixel(16, i);
        if (border_l[i].red() < 8 && border_l[i].green() < 8 && border_l[i].blue() < 8)
        {
            l_black_pixel += 1;
        }
        if (border_r[i].red() < 8 && border_r[i].green() < 8 && border_r[i].blue() < 8)
        {
            r_black_pixel += 1;
        }
        if (l_black_pixel > 8 || r_black_pixel > 8)
            return max_Tile16_border_pixels_color_channel_diff_value;
        delta_r[i] = border_l[i].red() - border_r[i].red();
        delta_g[i] = border_l[i].green() - border_r[i].green();
        delta_b[i] = border_l[i].blue() - border_r[i].blue();
        diff_r += (double)delta_r[i];
        diff_g += (double)delta_g[i];
        diff_b += (double)delta_b[i];
    }
    return diff_r + diff_b + diff_g;
}
