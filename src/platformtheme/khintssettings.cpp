/*  This file is part of the KDE libraries
 *  Copyright 2013 Kevin Ottens <ervin+bluesystems@kde.org>
 *  Copyright 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *  Copyright 2013 Alejandro Fiestas Olivares <afiestas@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License or ( at
 *  your option ) version 3 or, at the discretion of KDE e.V. ( which shall
 *  act as a proxy as in section 14 of the GPLv3 ), any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "khintssettings.h"

#include <QDebug>
#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QToolBar>
#include <QPalette>
#include <QToolButton>
#include <QMainWindow>
#include <QApplication>
#include <QGuiApplication>
#include <QDialogButtonBox>
#include <QScreen>
#include <QStandardPaths>

#include <QDBusConnection>
#include <QDBusInterface>

#include <kiconloader.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <kcolorscheme.h>

#include <config-platformtheme.h>
#ifdef UNIT_TEST
#undef HAVE_X11
#define HAVE_X11 0
#endif
#if HAVE_X11
#include <QX11Info>
#include <X11/Xcursor/Xcursor.h>
#endif

static const QString defaultLookAndFeelPackage = QStringLiteral("org.kde.breeze.desktop");

KHintsSettings::KHintsSettings(KSharedConfig::Ptr kdeglobals)
    : QObject(nullptr)
    , mKdeGlobals(kdeglobals)
{
    if (!mKdeGlobals) {
        mKdeGlobals = KSharedConfig::openConfig();
    }
    KConfigGroup cg(mKdeGlobals, "KDE");

    // try to extract the proper defaults file from a lookandfeel package
    const QString looknfeel = readConfigValue(cg, QStringLiteral("LookAndFeelPackage"), defaultLookAndFeelPackage).toString();
    mDefaultLnfConfig = KSharedConfig::openConfig(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/look-and-feel/") + defaultLookAndFeelPackage + QStringLiteral("/contents/defaults")));
    if (looknfeel != defaultLookAndFeelPackage) {
        mLnfConfig = KSharedConfig::openConfig(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/look-and-feel/") + looknfeel + QStringLiteral("/contents/defaults")));
    }

    if (qApp->applicationName() == QLatin1String("okteta")) { // okteta is bugged, and too many assumptions, so easier to fix here
        m_hints[QPlatformTheme::CursorFlashTime] = 500;
    } else {
        const int cursorBlinkRate = readConfigValue(cg, QStringLiteral("CursorBlinkRate"), -1).toInt();
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkRate > 0 ? qBound(200, cursorBlinkRate, 2000) : -1;
    }

    m_hints[QPlatformTheme::MouseDoubleClickInterval] = readConfigValue(cg, QStringLiteral("DoubleClickInterval"), 400);
    m_hints[QPlatformTheme::StartDragDistance] = readConfigValue(cg, QStringLiteral("StartDragDist"), 10);
    m_hints[QPlatformTheme::StartDragTime] = readConfigValue(cg, QStringLiteral("StartDragTime"), 500);

    KConfigGroup cgToolbar(mKdeGlobals, "Toolbar style");
    m_hints[QPlatformTheme::ToolButtonStyle] = toolButtonStyle(cgToolbar);

    KConfigGroup cgToolbarIcon(mKdeGlobals, "MainToolbarIcons");
    m_hints[QPlatformTheme::ToolBarIconSize] = readConfigValue(cgToolbarIcon, QStringLiteral("Size"), 22);

    m_hints[QPlatformTheme::ItemViewActivateItemOnSingleClick] = readConfigValue(cg, QStringLiteral("SingleClick"), true);

    m_hints[QPlatformTheme::SystemIconThemeName] = readConfigValue(QStringLiteral("Icons"), QStringLiteral("Theme"), QStringLiteral("breeze"));

    m_hints[QPlatformTheme::SystemIconFallbackThemeName] = QStringLiteral("hicolor");
    m_hints[QPlatformTheme::IconThemeSearchPaths] = xdgIconThemePaths();

    QStringList styleNames{
        QStringLiteral("sandsmarkstyle"),
        QStringLiteral("fusion"),
        QStringLiteral("breeze"),
        QStringLiteral("oxygen"),
        QStringLiteral("windows")
    };
    const QString configuredStyle = readConfigValue(cg, QStringLiteral("widgetStyle"), QString()).toString();
    if (!configuredStyle.isEmpty()) {
        styleNames.removeOne(configuredStyle);
        styleNames.prepend(configuredStyle);
    }
    const QString lnfStyle = readConfigValue(QStringLiteral("KDE"), QStringLiteral("widgetStyle"), QString()).toString();
    if (!lnfStyle.isEmpty()) {
        styleNames.removeOne(lnfStyle);
        styleNames.prepend(lnfStyle);
    }
    m_hints[QPlatformTheme::StyleNames] = styleNames;

    m_hints[QPlatformTheme::DialogButtonBoxLayout] = QDialogButtonBox::KdeLayout;
    m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = readConfigValue(cg, QStringLiteral("ShowIconsOnPushButtons"), true);
    m_hints[QPlatformTheme::UseFullScreenForPopupMenu] = true;
    m_hints[QPlatformTheme::KeyboardScheme] = QPlatformTheme::KdeKeyboardScheme;
    m_hints[QPlatformTheme::UiEffects] = readConfigValue(cg, QStringLiteral("GraphicEffectsLevel"), 0) != 0 ? QPlatformTheme::GeneralUiEffect : 0;
    m_hints[QPlatformTheme::IconPixmapSizes] = QVariant::fromValue(QList<int>() << 512 << 256 << 128 << 64 << 32 << 22 << 16 << 8);

    m_hints[QPlatformTheme::WheelScrollLines] = readConfigValue(cg, QStringLiteral("WheelScrollLines"), 3);
    if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
        QApplication::setWheelScrollLines(readConfigValue(cg, QStringLiteral("WheelScrollLines"), 3).toInt());
    }

    updateShowIconsInMenuItems(cg);

    m_hints[QPlatformTheme::ShowShortcutsInContextMenus] = true;

    QMetaObject::invokeMethod(this, "delayedDBusConnects", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "setupIconLoader", Qt::QueuedConnection);

    loadPalettes();
}

KHintsSettings::~KHintsSettings()
{
    qDeleteAll(m_palettes);
}

QVariant KHintsSettings::readConfigValue(const QString &group, const QString &key, const QVariant &defaultValue)
{
    KConfigGroup userCg(mKdeGlobals, group);
    QVariant value = readConfigValue(userCg, key, QString());

    if (!value.isNull()) {
        return value;
    }

    if (mLnfConfig) {
        KConfigGroup lnfCg(mLnfConfig, "kdeglobals");
        lnfCg = KConfigGroup(&lnfCg, group);
        if (lnfCg.isValid()) {
            value = lnfCg.readEntry(key, defaultValue);

            if (!value.isNull()) {
                return value;
            }
        }
    }

    KConfigGroup lnfCg(mDefaultLnfConfig, "kdeglobals");
    lnfCg = KConfigGroup(&lnfCg, group);
    if (lnfCg.isValid()) {
        return  lnfCg.readEntry(key, defaultValue);
    }

    return defaultValue;
}

QVariant KHintsSettings::readConfigValue(const KConfigGroup &cg, const QString &key, const QVariant &defaultValue) const
{
    return cg.readEntry(key, defaultValue);
}

QStringList KHintsSettings::xdgIconThemePaths() const
{
    QStringList paths;

    // make sure we have ~/.local/share/icons in paths if it exists
    paths << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);

    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir()) {
        paths << homeIconDir.absoluteFilePath();
    }

    return paths;
}

void KHintsSettings::delayedDBusConnects()
{
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/KToolBar"), QStringLiteral("org.kde.KToolBar"),
                                          QStringLiteral("styleChanged"), this, SLOT(toolbarStyleChanged()));
    QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/KGlobalSettings"), QStringLiteral("org.kde.KGlobalSettings"),
                                          QStringLiteral("notifyChange"), this, SLOT(slotNotifyChange(int,int)));
}

void KHintsSettings::setupIconLoader()
{
    connect(KIconLoader::global(), &KIconLoader::iconChanged, this, &KHintsSettings::iconChanged);
}

void KHintsSettings::toolbarStyleChanged()
{
    mKdeGlobals->reparseConfiguration();
    KConfigGroup cg(mKdeGlobals, "Toolbar style");

    m_hints[QPlatformTheme::ToolButtonStyle] = toolButtonStyle(cg);
    //from gtksymbol.cpp
    QWidgetList widgets = QApplication::allWidgets();
    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *widget = widgets.at(i);
        if (qobject_cast<QToolButton *>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

void KHintsSettings::slotNotifyChange(int type, int arg)
{
    mKdeGlobals->reparseConfiguration();
    KConfigGroup cg(mKdeGlobals, "KDE");

    switch (type) {
    case PaletteChanged: {
        // Don't change the palette if the application has a custom one set
        if (!qApp->property("KDE_COLOR_SCHEME_PATH").toString().isEmpty()) {
            break;
        }
        loadPalettes();

        //QApplication::setPalette and QGuiApplication::setPalette are different functions
        //and non virtual. Call the correct one
        if (qobject_cast<QApplication *>(QCoreApplication::instance())) {
            if (!m_palettes.contains(QPlatformTheme::SystemPalette)) {
                qWarning() << "Missing system palette!" << type << arg;
                return;
            }
            QPalette palette = *m_palettes[QPlatformTheme::SystemPalette];
            QApplication::setPalette(palette);
            // QTBUG QGuiApplication::paletteChanged() signal is only emitted by QGuiApplication
            // so things like SystemPalette QtQuick item that use it won't notice a palette
            // change when a QApplication which causes e.g. QML System Settings modules to not update
            emit qApp->paletteChanged(palette);
        } else if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
            QGuiApplication::setPalette(*m_palettes[QPlatformTheme::SystemPalette]);
        }
        break;
    }
    case SettingsChanged: {

        SettingsCategory category = static_cast<SettingsCategory>(arg);
        if (category == SETTINGS_QT || category == SETTINGS_MOUSE) {
            updateQtSettings(cg);
        } else if (category == SETTINGS_STYLE) {
            m_hints[QPlatformTheme::DialogButtonBoxButtonsHaveIcons] = cg.readEntry("ShowIconsOnPushButtons", true);
            m_hints[QPlatformTheme::UiEffects] = cg.readEntry("GraphicEffectsLevel", 0) != 0 ? QPlatformTheme::GeneralUiEffect : 0;

            updateShowIconsInMenuItems(cg);
        }
        break;
    }
    case ToolbarStyleChanged: {
        toolbarStyleChanged();
        break;
    }
    case IconChanged:
        iconChanged(arg); //Once the KCM is ported to use IconChanged, this should not be needed
        break;
    case CursorChanged:
        updateCursorTheme();
        break;
    case StyleChanged: {
        QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
        if (!app) {
            return;
        }

        const QString theme = cg.readEntry("widgetStyle", QString());
        if (theme.isEmpty()) {
            return;
        }

        QStringList styleNames;
        styleNames << cg.readEntry("widgetStyle", QString())
                << QStringLiteral("sandsmarkstyle")
                << QStringLiteral("fusion")
                << QStringLiteral("breeze")
                << QStringLiteral("oxygen")
                << QStringLiteral("windows");
        const QString lnfStyle = readConfigValue(QStringLiteral("KDE"), QStringLiteral("widgetStyle"), QString()).toString();
        if (!lnfStyle.isEmpty() && !styleNames.contains(lnfStyle)) {
            styleNames.prepend(lnfStyle);
        }
        m_hints[QPlatformTheme::StyleNames] = styleNames;

        app->setStyle(theme);
        loadPalettes();
        break;
    }
    default:
        qWarning() << "Unknown type of change in KGlobalSettings::slotNotifyChange: " << type;
    }
}

void KHintsSettings::iconChanged(int group)
{
    KIconLoader::Group iconGroup = (KIconLoader::Group) group;
    if (iconGroup != KIconLoader::MainToolbar) {
        m_hints[QPlatformTheme::SystemIconThemeName] = readConfigValue(QStringLiteral("Icons"), QStringLiteral("Theme"), QStringLiteral("breeze"));
        return;
    }

    const int currentSize = KIconLoader::global()->currentSize(KIconLoader::MainToolbar);
    if (m_hints[QPlatformTheme::ToolBarIconSize] == currentSize) {
        return;
    }

    m_hints[QPlatformTheme::ToolBarIconSize] = currentSize;

    //If we are not a QApplication, means that we are a QGuiApplication, then we do nothing.
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
        return;
    }

    QWidgetList widgets = QApplication::allWidgets();
    Q_FOREACH (QWidget *widget, widgets) {
        if (qobject_cast<QToolBar *>(widget) || qobject_cast<QMainWindow *>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

void KHintsSettings::updateQtSettings(KConfigGroup &cg)
{
    if (qApp->applicationName() == QLatin1String("okteta")) { // okteta is bugged, and too many assumptions, so easier to fix here
        m_hints[QPlatformTheme::CursorFlashTime] = 500;
    } else {
        const int cursorBlinkRate = readConfigValue(cg, QStringLiteral("CursorBlinkRate"), -1).toInt();
        m_hints[QPlatformTheme::CursorFlashTime] = cursorBlinkRate > 0 ? qBound(200, cursorBlinkRate, 2000) : -1;
    }

    int doubleClickInterval = cg.readEntry("DoubleClickInterval", 400);
    m_hints[QPlatformTheme::MouseDoubleClickInterval] = doubleClickInterval;

    int startDragDistance = cg.readEntry("StartDragDist", 10);
    m_hints[QPlatformTheme::StartDragDistance] = startDragDistance;

    int startDragTime = cg.readEntry("StartDragTime", 500);
    m_hints[QPlatformTheme::StartDragTime] = startDragTime;

    m_hints[QPlatformTheme::ItemViewActivateItemOnSingleClick] = cg.readEntry("SingleClick", true);

    updateShowIconsInMenuItems(cg);

    int wheelScrollLines = cg.readEntry("WheelScrollLines", 3);
    m_hints[QPlatformTheme::WheelScrollLines] = wheelScrollLines;
    QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (app) {
        QApplication::setWheelScrollLines(cg.readEntry("WheelScrollLines", 3));
    }
}

void KHintsSettings::updateShowIconsInMenuItems(KConfigGroup &cg)
{
    bool showIcons = readConfigValue(cg, QStringLiteral("ShowIconsInMenuItems"), true).toBool();
    QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus, !showIcons);
}

Qt::ToolButtonStyle KHintsSettings::toolButtonStyle(const KConfigGroup &cg)
{
    const QString buttonStyle = readConfigValue(cg, QStringLiteral("ToolButtonStyle"), QStringLiteral("TextBesideIcon")).toString().toLower();
    return buttonStyle == QLatin1String("textbesideicon") ? Qt::ToolButtonTextBesideIcon
           : buttonStyle == QLatin1String("icontextright") ? Qt::ToolButtonTextBesideIcon
           : buttonStyle == QLatin1String("textundericon") ? Qt::ToolButtonTextUnderIcon
           : buttonStyle == QLatin1String("icontextbottom") ? Qt::ToolButtonTextUnderIcon
           : buttonStyle == QLatin1String("textonly") ? Qt::ToolButtonTextOnly
           : Qt::ToolButtonIconOnly;
}

void KHintsSettings::loadPalettes()
{
    qDeleteAll(m_palettes);
    m_palettes.clear();

    if (mKdeGlobals->hasGroup("Colors:View")) {
        m_palettes[QPlatformTheme::SystemPalette] = new QPalette(KColorScheme::createApplicationPalette(mKdeGlobals));
        return;
    }

    KConfigGroup cg(mKdeGlobals, "KDE");
    const QString looknfeel = readConfigValue(cg, QStringLiteral("LookAndFeelPackage"), defaultLookAndFeelPackage).toString();
    QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/look-and-feel/") + looknfeel + QStringLiteral("/contents/colors"));
    if (!path.isEmpty()) {
        m_palettes[QPlatformTheme::SystemPalette] = new QPalette(KColorScheme::createApplicationPalette(KSharedConfig::openConfig(path)));
        return;
    }

    const QString scheme = readConfigValue(QStringLiteral("General"), QStringLiteral("ColorScheme"), QStringLiteral("Breeze")).toString();
    path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/") + scheme + QStringLiteral(".colors"));

    KSharedConfig::Ptr config = mKdeGlobals;

    if (!path.isEmpty()) {
        config = KSharedConfig::openConfig(path);
        return;
    }

    m_palettes[QPlatformTheme::SystemPalette] = new QPalette(KColorScheme::createApplicationPalette(config));
}

void KHintsSettings::updateCursorTheme()
{
    KConfig config(QStringLiteral("kcminputrc"));
    KConfigGroup g(&config, "Mouse");

    int size      = g.readEntry("cursorSize", 24);

#if HAVE_X11
    if (QX11Info::isPlatformX11()) {
        const QString theme = g.readEntry("cursorTheme", QString());
        // Note that in X11R7.1 and earlier, calling XcursorSetTheme()
        // with a NULL theme would cause Xcursor to use "default", but
        // in 7.2 and later it will cause it to revert to the theme that
        // was configured when the application was started.
        XcursorSetTheme(QX11Info::display(), theme.isNull() ?
                        "default" : QFile::encodeName(theme).constData());
        XcursorSetDefaultSize(QX11Info::display(), size);
    }
#endif
}
