#ifndef THEMES_H
#define THEMES_H

#include "phantomstyle.h"
#include <QMainWindow>

enum ThemeColorType : int
{
    Light = 0,
    Dark = 1,
};

QPalette namedColorSchemePalette(ThemeColorType color)
{
    struct ThemeColors
    {
        QColor window;
        QColor text;
        QColor disabledText;
        QColor brightText;
        QColor highlight;
        QColor highlightedText;
        QColor base;
        QColor alternateBase;
        QColor shadow;
        QColor button;
        QColor disabledButton;
        QColor unreadBadge;
        QColor unreadBadgeText;
        QColor icon;
        QColor disabledIcon;
        QColor chatTimestampText;
    };

    auto themeColorsToPalette = [](const ThemeColors &x) -> QPalette {
        QPalette pal;
        pal.setColor(QPalette::Window, x.window);
        pal.setColor(QPalette::WindowText, x.text);
        pal.setColor(QPalette::Text, x.text);
        pal.setColor(QPalette::ButtonText, x.text);
        if (x.brightText.isValid())
            pal.setColor(QPalette::BrightText, x.brightText);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, x.disabledText);
        pal.setColor(QPalette::Disabled, QPalette::Text, x.disabledText);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, x.disabledText);
        pal.setColor(QPalette::Base, x.base);
        pal.setColor(QPalette::AlternateBase, x.alternateBase);
        if (x.shadow.isValid())
            pal.setColor(QPalette::Shadow, x.shadow);
        pal.setColor(QPalette::Button, x.button);
        pal.setColor(QPalette::Highlight, x.highlight);
        pal.setColor(QPalette::HighlightedText, x.highlightedText);
        if (x.disabledButton.isValid())
            pal.setColor(QPalette::Disabled, QPalette::Button, x.disabledButton);
        // Used as the shadow text color on disabled menu items
        pal.setColor(QPalette::Disabled, QPalette::Light, Qt::transparent);
        return pal;
    };

    ThemeColors c;

    // clang-format off
    switch (color)
    {
        case Light: 
        {
            QColor snow(251, 252, 254);
            QColor callout(90, 97, 111);
            QColor bright(237, 236, 241);
            QColor lessBright(234, 234, 238);
            QColor dimmer(221, 221, 226);
            QColor text(18, 18, 24);
            QColor disabledText(140, 140, 145);
            c.window = bright;
            c.highlight = callout;
            c.highlightedText = QColor(255, 255, 255);
            c.base = snow;
            c.alternateBase = lessBright;
            c.button = bright;
            c.text = text;
            c.disabledText = disabledText;
            c.icon = QColor(105, 107, 113);
            c.disabledIcon = c.disabledText.lighter(125);
            c.unreadBadge = c.highlight;
            c.unreadBadgeText = c.highlightedText;
            c.chatTimestampText = c.base.darker(130);
            break;
        }
        case Dark: 
        {
            QColor window(22, 21, 20);
            QColor button(90, 89, 88);
            QColor base(53, 54, 55);
            QColor alternateBase(51, 51, 52);
            QColor text(240, 240, 240);
            QColor highlight(83, 114, 142);
            QColor highlightedText(240, 240, 240);
            QColor disabledText(120, 120, 120);
            c.window = window;
            c.text = text;
            c.disabledText = disabledText;
            c.base = base;
            c.alternateBase = alternateBase;
            c.shadow = base;
            c.button = button;
            c.disabledButton = button.darker(50);
            c.brightText = Qt::white;
            c.highlight = highlight;
            c.highlightedText = highlightedText;
            c.icon = text;
            c.disabledIcon = c.disabledText;
            c.unreadBadge = c.text;
            c.unreadBadgeText = c.highlightedText;
            c.chatTimestampText = c.base.lighter(160);
            break;
        }
    }
    return themeColorsToPalette(c);
    // clang-format on
}
#endif
