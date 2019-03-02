#pragma once

#include <QtWidgets/private/qfusionstyle_p.h>
#include <QtWidgets/private/qfusionstyle_p_p.h>

class SandsmarkStyle : public QFusionStyle
{
    Q_OBJECT

public:
    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
            QPainter *painter, const QWidget *widget) const override;
};

