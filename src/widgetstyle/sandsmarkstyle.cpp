/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "sandsmarkstyle.h"
#include <QtWidgets/private/qstylehelper_p.h>
#include <QtWidgets/private/qstyle_p.h>
#include <QPainter>
#include <QPixmapCache>
#include <QStyleOptionSlider>

int SandsmarkStyle::styleHint(QStyle::StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch(stylehint){
    case QStyle::SH_Widget_Animate:
        return false;
    case QStyle::SH_Widget_Animation_Duration:
        return 0;
    case QStyle::SH_Menu_FadeOutOnHide:
        return false;
    case QStyle::SH_Menu_KeyboardSearch:
        return true;
    case QStyle::SH_Menu_SubMenuDontStartSloppyOnLeave:
        return true;
    case QStyle::SH_Menu_SubMenuPopupDelay:
        return 150;
    case QStyle::SH_Menu_SloppySubMenus:
        return true;
    //case QStyle::SH_ToolTip_WakeUpDelay:
    //    return 0;
    default:
        break;
    }

    return QFusionStyle::styleHint(stylehint, opt, widget, returnData);
}

// Not exported from QStyleHelper, so copied in here
static QString uniqueName(const QString &key, const QStyleOption *option, const QSize &size)
{
    const QStyleOptionComplex *complexOption = qstyleoption_cast<const QStyleOptionComplex *>(option);
    QString tmp = key % HexString<uint>(option->state)
            % HexString<uint>(option->direction)
            % HexString<uint>(complexOption ? uint(complexOption->activeSubControls) : 0u)
            % HexString<quint64>(option->palette.cacheKey())
            % HexString<uint>(size.width())
            % HexString<uint>(size.height());
    return tmp;
}

static void qt_fusion_draw_arrow(Qt::ArrowType type, QPainter *painter, const QStyleOption *option, const QRect &rect, const QColor &color)
{
    if (rect.isEmpty())
        return;

    const int arrowWidth = QStyleHelper::dpiScaled(14, option);
    const int arrowHeight = QStyleHelper::dpiScaled(8, option);

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(rect.height(), rect.width());
    const int size = qMin(arrowMax, rectMax);

    QPixmap cachePixmap;
    QString cacheKey = uniqueName(QLatin1String("fusion-arrow"), option, rect.size())
            % HexString<uint>(type)
            % HexString<uint>(color.rgba());
    if (!QPixmapCache::find(cacheKey, &cachePixmap)) {
        cachePixmap = styleCachePixmap(rect.size());
        cachePixmap.fill(Qt::transparent);
        QPainter cachePainter(&cachePixmap);

        QRectF arrowRect;
        arrowRect.setWidth(size);
        arrowRect.setHeight(arrowHeight * size / arrowWidth);
        if (type == Qt::LeftArrow || type == Qt::RightArrow)
            arrowRect = arrowRect.transposed();
        arrowRect.moveTo((rect.width() - arrowRect.width()) / 2.0,
                         (rect.height() - arrowRect.height()) / 2.0);

        QPolygonF triangle;
        triangle.reserve(3);
        switch (type) {
        case Qt::DownArrow:
            triangle << arrowRect.topLeft() << arrowRect.topRight() << QPointF(arrowRect.center().x(), arrowRect.bottom());
            break;
        case Qt::RightArrow:
            triangle << arrowRect.topLeft() << arrowRect.bottomLeft() << QPointF(arrowRect.right(), arrowRect.center().y());
            break;
        case Qt::LeftArrow:
            triangle << arrowRect.topRight() << arrowRect.bottomRight() << QPointF(arrowRect.left(), arrowRect.center().y());
            break;
        default:
            triangle << arrowRect.bottomLeft() << arrowRect.bottomRight() << QPointF(arrowRect.center().x(), arrowRect.top());
            break;
        }

        cachePainter.setPen(Qt::NoPen);
        cachePainter.setBrush(color);
        cachePainter.setRenderHint(QPainter::Antialiasing);
        cachePainter.drawPolygon(triangle);

        QPixmapCache::insert(cacheKey, cachePixmap);
    }

    painter->drawPixmap(rect, cachePixmap);
}

//// Used for grip handles
static inline QColor lightShade() {
    return QColor(255, 255, 255, 90);
}
static inline QColor darkShade() {
    return QColor(0, 0, 0, 60);
}

static inline QColor innerContrastLine() {
    return QColor(255, 255, 255, 30);
}

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

static QColor getOutline(const QPalette &pal) {
    if (pal.window().style() == Qt::TexturePattern)
        return QColor(0, 0, 0, 160);
    return pal.window().color().darker(140);
}


static QColor getButtonColor(const QPalette &pal) {
    QColor buttonColor = pal.button().color();
    int val = qGray(buttonColor.rgb());
    buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
    buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
    return buttonColor;
}

void SandsmarkStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                      QPainter *painter, const QWidget *widget) const
{
    if (control != CC_ScrollBar) {
        QFusionStyle::drawComplexControl(control, option, painter, widget);
        return;
    }

    const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option);
    if (!scrollBar) {
        return;
    }

    painter->save();
    const QColor outline = getOutline(option->palette);
    const QColor buttonColor = getButtonColor(option->palette);
    const QColor gradientStartColor = buttonColor.lighter(118);
    const QColor gradientStopColor = buttonColor;

    bool wasActive = false;
    qreal expandScale = 1.0;
    qreal expandOffset = -1.0;
    QObject *styleObject = option->styleObject;
    if (styleObject && proxy()->styleHint(SH_ScrollBar_Transient, option, widget)) {
        qreal opacity = 0.0;
        bool shouldExpand = false;
        const qreal maxExpandScale = 13.0 / 9.0;

        int oldPos = styleObject->property("_q_stylepos").toInt();
        int oldMin = styleObject->property("_q_stylemin").toInt();
        int oldMax = styleObject->property("_q_stylemax").toInt();
        QRect oldRect = styleObject->property("_q_stylerect").toRect();
        QStyle::State oldState = static_cast<QStyle::State>(styleObject->property("_q_stylestate").value<QStyle::State::Int>());
        uint oldActiveControls = styleObject->property("_q_stylecontrols").toUInt();

        // a scrollbar is transient when the the scrollbar itself and
        // its sibling are both inactive (ie. not pressed/hovered/moved)
        bool transient = !option->activeSubControls && !(option->state & State_On);

        if (!transient ||
                oldPos != scrollBar->sliderPosition ||
                oldMin != scrollBar->minimum ||
                oldMax != scrollBar->maximum ||
                oldRect != scrollBar->rect ||
                oldState != scrollBar->state ||
                oldActiveControls != scrollBar->activeSubControls) {

            styleObject->setProperty("_q_stylepos", scrollBar->sliderPosition);
            styleObject->setProperty("_q_stylemin", scrollBar->minimum);
            styleObject->setProperty("_q_stylemax", scrollBar->maximum);
            styleObject->setProperty("_q_stylerect", scrollBar->rect);
            styleObject->setProperty("_q_stylestate", static_cast<QStyle::State::Int>(scrollBar->state));
            styleObject->setProperty("_q_stylecontrols", static_cast<uint>(scrollBar->activeSubControls));

            // if the scrollbar is transient or its attributes, geometry or
            // state has changed, the opacity is reset back to 100% opaque
            opacity = 1.0;
        }

        shouldExpand = (option->activeSubControls || wasActive);
        if (shouldExpand) {
            // Keep expanded state after the animation ends, and when fading out
            expandScale = maxExpandScale;
            expandOffset = 4.5;
        }
        painter->setOpacity(opacity);
    }

    bool transient = proxy()->styleHint(SH_ScrollBar_Transient, option, widget);
    bool horizontal = scrollBar->orientation == Qt::Horizontal;
    bool sunken = scrollBar->state & State_Sunken;

    QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
    QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
    QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
    QRect scrollBarGroove = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);

    QRect rect = option->rect;
    QColor alphaOutline = outline;
    alphaOutline.setAlpha(180);

    QColor arrowColor = option->palette.windowText().color();
    arrowColor.setAlpha(160);

    QColor subtleEdge = alphaOutline;
    subtleEdge.setAlpha(40);

    const QColor bgColor = QStyleHelper::backgroundColor(option->palette, widget);
    const bool isDarkBg = bgColor.red() < 128 && bgColor.green() < 128 && bgColor.blue() < 128;

    if (transient) {
        if (horizontal) {
            rect.setY(rect.y() + 4.5 - expandOffset);
            scrollBarSlider.setY(scrollBarSlider.y() + 4.5 - expandOffset);
            scrollBarGroove.setY(scrollBarGroove.y() + 4.5 - expandOffset);

            rect.setHeight(rect.height() * expandScale);
            scrollBarGroove.setHeight(scrollBarGroove.height() * expandScale);
        } else {
            rect.setX(rect.x() + 4.5 - expandOffset);
            scrollBarSlider.setX(scrollBarSlider.x() + 4.5 - expandOffset);
            scrollBarGroove.setX(scrollBarGroove.x() + 4.5 - expandOffset);

            rect.setWidth(rect.width() * expandScale);
            scrollBarGroove.setWidth(scrollBarGroove.width() * expandScale);
        }
    }

    // Paint groove
    if ((!transient || scrollBar->activeSubControls || wasActive) && scrollBar->subControls & SC_ScrollBarGroove) {
        QLinearGradient gradient(rect.center().x(), rect.top(),
                rect.center().x(), rect.bottom());
        if (!horizontal)
            gradient = QLinearGradient(rect.left(), rect.center().y(),
                    rect.right(), rect.center().y());
        if (!isDarkBg) {
            gradient.setColorAt(0.0, buttonColor.darker(357));
            gradient.setColorAt(0.1, buttonColor.darker(355));
            gradient.setColorAt(0.9, buttonColor.darker(355));
            gradient.setColorAt(1.0, buttonColor.darker(357));
        } else if (!transient) {
            gradient.setColorAt(0.0, buttonColor.darker(157));
            gradient.setColorAt(0.1, buttonColor.darker(155));
            gradient.setColorAt(0.9, buttonColor.darker(155));
            gradient.setColorAt(1.0, buttonColor.darker(157));
        } else {
            gradient.setColorAt(0.0, bgColor.lighter(157));
            gradient.setColorAt(0.1, bgColor.lighter(155));
            gradient.setColorAt(0.9, bgColor.lighter(155));
            gradient.setColorAt(1.0, bgColor.lighter(157));
        }

        painter->save();
        if (transient)
            painter->setOpacity(0.8);
        painter->fillRect(rect, gradient);
        painter->setPen(Qt::NoPen);
        if (transient)
            painter->setOpacity(0.4);
        painter->setPen(alphaOutline);
        if (horizontal)
            painter->drawLine(rect.topLeft(), rect.topRight());
        else
            painter->drawLine(rect.topLeft(), rect.bottomLeft());

        painter->setPen(subtleEdge);
        painter->setBrush(Qt::NoBrush);
        painter->setClipRect(scrollBarGroove.adjusted(1, 0, -1, -3));
        painter->drawRect(scrollBarGroove.adjusted(1, 0, -1, -1));
        painter->restore();
    }

    QRect pixmapRect = scrollBarSlider;
    QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
            pixmapRect.center().x(), pixmapRect.bottom());
    if (!horizontal)
        gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                pixmapRect.right(), pixmapRect.center().y());

    QLinearGradient highlightedGradient = gradient;

    QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
    gradient.setColorAt(0, getButtonColor(option->palette).lighter(108));
    gradient.setColorAt(1, getButtonColor(option->palette));

    highlightedGradient.setColorAt(0, gradientStartColor.darker(102));
    highlightedGradient.setColorAt(1, gradientStopColor.lighter(102));

    // Paint slider
    if (scrollBar->subControls & SC_ScrollBarSlider) {
        if (transient) {
            QRect rect = scrollBarSlider.adjusted(horizontal ? 1 : 2, horizontal ? 2 : 1, -1, -1);
            painter->setPen(Qt::NoPen);
            painter->setBrush(isDarkBg ? lightShade() : darkShade());
            int r = qMin(rect.width(), rect.height()) / 2;

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawRoundedRect(rect, r, r);
            painter->restore();
        } else {
            QRect pixmapRect = scrollBarSlider;
            painter->setPen(QPen(alphaOutline));
            if (option->state & State_Sunken && scrollBar->activeSubControls & SC_ScrollBarSlider)
                painter->setBrush(midColor2);
            else if (option->state & State_MouseOver && scrollBar->activeSubControls & SC_ScrollBarSlider)
                painter->setBrush(highlightedGradient);
            else
                painter->setBrush(gradient);

            painter->drawRect(pixmapRect.adjusted(horizontal ? -1 : 0, horizontal ? 0 : -1, horizontal ? 0 : 1, horizontal ? 1 : 0));

            painter->setPen(innerContrastLine());
            painter->drawRect(scrollBarSlider.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));

            // Outer shadow
            painter->setPen(subtleEdge);
            if (horizontal) {
                painter->drawLine(scrollBarSlider.topLeft() + QPoint(-2, 0), scrollBarSlider.bottomLeft() + QPoint(2, 0));
                painter->drawLine(scrollBarSlider.topRight() + QPoint(-2, 0), scrollBarSlider.bottomRight() + QPoint(2, 0));
            } else {
                painter->drawLine(pixmapRect.topLeft() + QPoint(0, -2), pixmapRect.bottomLeft() + QPoint(0, -2));
                painter->drawLine(pixmapRect.topRight() + QPoint(0, 2), pixmapRect.bottomRight() + QPoint(0, 2));
            }
        }
    }

    // The SubLine (up/left) buttons
    if (!transient && scrollBar->subControls & SC_ScrollBarSubLine) {
        if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
            painter->setBrush(gradientStopColor);
        else if ((scrollBar->activeSubControls & SC_ScrollBarSubLine))
            painter->setBrush(highlightedGradient);
        else
            painter->setBrush(gradient);

        painter->setPen(Qt::NoPen);
        painter->drawRect(scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
        painter->setPen(QPen(alphaOutline));
        if (option->state & State_Horizontal) {
            if (option->direction == Qt::RightToLeft) {
                pixmapRect.setLeft(scrollBarSubLine.left());
                painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
            } else {
                pixmapRect.setRight(scrollBarSubLine.right());
                painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
            }
        } else {
            pixmapRect.setBottom(scrollBarSubLine.bottom());
            painter->drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());
        }

        QRect upRect = scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, horizontal ? -2 : -1, horizontal ? -1 : -2);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(innerContrastLine());
        painter->drawRect(upRect);

        // Arrows
        Qt::ArrowType arrowType = Qt::UpArrow;
        if (option->state & State_Horizontal)
            arrowType = option->direction == Qt::LeftToRight ? Qt::LeftArrow : Qt::RightArrow;
        qt_fusion_draw_arrow(arrowType, painter, option, upRect, arrowColor);
    }

    // The AddLine (down/right) button
    if (!transient && scrollBar->subControls & SC_ScrollBarAddLine) {
        if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
            painter->setBrush(gradientStopColor);
        else if ((scrollBar->activeSubControls & SC_ScrollBarAddLine))
            painter->setBrush(midColor2);
        else
            painter->setBrush(gradient);

        painter->setPen(Qt::NoPen);
        painter->drawRect(scrollBarAddLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
        painter->setPen(QPen(alphaOutline, 1));
        if (option->state & State_Horizontal) {
            if (option->direction == Qt::LeftToRight) {
                pixmapRect.setLeft(scrollBarAddLine.left());
                painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
            } else {
                pixmapRect.setRight(scrollBarAddLine.right());
                painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
            }
        } else {
            pixmapRect.setTop(scrollBarAddLine.top());
            painter->drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
        }

        QRect downRect = scrollBarAddLine.adjusted(1, 1, -1, -1);
        painter->setPen(innerContrastLine());
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(downRect);

        Qt::ArrowType arrowType = Qt::DownArrow;
        if (option->state & State_Horizontal)
            arrowType = option->direction == Qt::LeftToRight ? Qt::RightArrow : Qt::LeftArrow;
        qt_fusion_draw_arrow(arrowType, painter, option, downRect, arrowColor);
    }

    painter->restore();
}
