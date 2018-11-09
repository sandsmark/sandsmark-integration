#pragma once

#include <QStylePlugin>

class SandsmarkStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "sandsmarkstyle.json")

public:
    SandsmarkStylePlugin() {}

    QStringList keys() const;
    QStyle *create(const QString &key) override;
};
