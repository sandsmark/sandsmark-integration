#pragma once

#include <QtWidgets/private/qfusionstyle_p.h>

class SandsmarkStyle : public QFusionStyle
{
    Q_OBJECT

public:
    SandsmarkStyle() {}


    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const override;
};

