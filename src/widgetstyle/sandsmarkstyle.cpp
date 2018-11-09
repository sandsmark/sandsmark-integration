#include "sandsmarkstyle.h"

int SandsmarkStyle::styleHint(QStyle::StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch(stylehint){
    case QStyle::SH_Widget_Animate:
        return false;
    case QStyle::SH_Widget_Animation_Duration:
        return 0;
    default:
        break;
    }

    return QFusionStyle::styleHint(stylehint, opt, widget, returnData);
}
