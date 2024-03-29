/* This file is part of the KDE libraries
   Copyright (C) 2000, 2006 David Faure <faure@kde.org>
   Copyright 2008 Friedrich W. H. Kossebau <kossebau@kde.org>
   Copyright 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfontsettingsdata.h"
#include <QCoreApplication>
#include <QString>
#include <QVariant>
#include <QApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <qpa/qwindowsysteminterface.h>

#include <ksharedconfig.h>
#include <kconfiggroup.h>

KFontSettingsData::KFontSettingsData()
    : QObject(nullptr),
    mKdeGlobals(KSharedConfig::openConfig())
{
    QMetaObject::invokeMethod(this, "delayedDBusConnects", Qt::QueuedConnection);

    for (int i = 0; i < FontTypesCount; ++i) {
        mFonts[i] = nullptr;
    }
}

KFontSettingsData::~KFontSettingsData()
{
    for (int i = 0; i < FontTypesCount; ++i) {
        delete mFonts[i];
    }
}

// NOTE: keep in sync with plasma-desktop/kcms/fonts/fonts.cpp
static const char GeneralId[] =      "General";
static const char DefaultFont[] =    "Noto Sans";

static const KFontData DefaultFontData[KFontSettingsData::FontTypesCount] = {
    { GeneralId, "font",                 DefaultFont,  10, QFont::Normal, QFont::SansSerif, "Regular" },
    { GeneralId, "fixed",                "Hack",       10, QFont::Normal, QFont::Monospace, "Regular" },
    { GeneralId, "toolBarFont",          DefaultFont,  10, QFont::Normal, QFont::SansSerif, "Regular" },
    { GeneralId, "menuFont",             DefaultFont,  10, QFont::Normal, QFont::SansSerif, "Regular" },
    { "WM",      "activeFont",           DefaultFont,  10, QFont::Normal, QFont::SansSerif, "Regular" },
    { GeneralId, "taskbarFont",          DefaultFont,  10, QFont::Normal, QFont::SansSerif, "Regular" },
    { GeneralId, "smallestReadableFont", DefaultFont,   9, QFont::Normal, QFont::SansSerif, "Regular" }
};

QFont *KFontSettingsData::font(FontTypes fontType)
{
    QFont *cachedFont = mFonts[fontType];

    if (!cachedFont) {
        const KFontData &fontData = DefaultFontData[fontType];
        cachedFont = new QFont(QLatin1String(fontData.FontName), fontData.Size, fontData.Weight);
        cachedFont->setStyleHint(fontData.StyleHint);

        const QString fontInfo = readConfigValue(QLatin1String(fontData.ConfigGroupKey), QLatin1String(fontData.ConfigKey));

        //If we have serialized information for this font, restore it
        //NOTE: We are not using KConfig directly because we can't call QFont::QFont from here
        if (!fontInfo.isEmpty()) {
            cachedFont->fromString(fontInfo);
        }

        mFonts[fontType] = cachedFont;
    }

    return cachedFont;
}

void KFontSettingsData::dropFontSettingsCache()
{
    mKdeGlobals->reparseConfiguration();
    for (int i = 0; i < FontTypesCount; ++i) {
        delete mFonts[i];
        mFonts[i] = nullptr;
    }

    QWindowSystemInterface::handleThemeChange(nullptr);

    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setFont(*font(KFontSettingsData::GeneralFont));
    } else {
        QGuiApplication::setFont(*font(KFontSettingsData::GeneralFont));
    }
}

void KFontSettingsData::delayedDBusConnects()
{
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/KDEPlatformTheme"), QStringLiteral("org.kde.KDEPlatformTheme"),
                                          QStringLiteral("refreshFonts"), this, SLOT(dropFontSettingsCache()));
}

QString KFontSettingsData::readConfigValue(const QString &group, const QString &key, const QString &defaultValue) const
{
    const KConfigGroup configGroup(mKdeGlobals, group);
    return configGroup.readEntry(key, defaultValue);
}
