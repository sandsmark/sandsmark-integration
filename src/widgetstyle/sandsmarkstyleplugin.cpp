#include "sandsmarkstyleplugin.h"
#include "sandsmarkstyle.h"

QStringList SandsmarkStylePlugin::keys() const
{
    return {"SandsmarkStyle"};
}

QStyle *SandsmarkStylePlugin::create(const QString &key)
{
    if (key.toLower() == "sandsmarkstyle")
        return new SandsmarkStyle;
    return nullptr;
}
