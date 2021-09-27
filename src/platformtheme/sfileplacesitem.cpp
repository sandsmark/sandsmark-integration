/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sfileplacesitem_p.h"

#include <QDateTime>
#include <QIcon>

#include <KBookmarkManager>
#include <KConfig>
#include <KConfigGroup>
#include <KIconUtils>
#include <KLocalizedString>
#include <QDebug>
#include <kprotocolinfo.h>

static bool isTrash(const KBookmark &bk)
{
    return bk.url().toString() == QLatin1String("trash:/");
}

SFilePlacesItem::SFilePlacesItem(KBookmarkManager *manager, const QString &address, const QString &udi, SFilePlacesModel *parent)
    : QObject(static_cast<QObject *>(parent))
    , m_manager(manager)
    , m_folderIsEmpty(true)
    , m_isCdrom(false)
    , m_isAccessible(false)
{
    updateDeviceInfo(udi);
    setBookmark(m_manager->findByAddress(address));

    if (udi.isEmpty() && m_bookmark.metaDataItem(QStringLiteral("ID")).isEmpty()) {
        m_bookmark.setMetaDataItem(QStringLiteral("ID"), generateNewId());
    } else if (udi.isEmpty()) {
        if (isTrash(m_bookmark)) {
            KConfig cfg(QStringLiteral("trashrc"), KConfig::SimpleConfig);
            const KConfigGroup group = cfg.group("Status");
            m_folderIsEmpty = group.readEntry("Empty", true);
        }
    }
}

SFilePlacesItem::~SFilePlacesItem()
{
}

QString SFilePlacesItem::id() const
{
    if (isDevice()) {
        return bookmark().metaDataItem(QStringLiteral("UDI"));
    } else {
        return bookmark().metaDataItem(QStringLiteral("ID"));
    }
}

bool SFilePlacesItem::hasSupportedScheme(const QStringList &schemes) const
{
    if (schemes.isEmpty()) {
        return true;
    }

    // StorageAccess is always local, doesn't need to be accessible to know this
    //if (m_access && schemes.contains(QLatin1String("file"))) {
    //    return true;
    //}

    //if (m_networkShare && schemes.contains(m_networkShare->url().scheme())) {
    //    return true;
    //}

    //if (m_player) {
    //    const QStringList protocols = m_player->supportedProtocols();
    //    for (const QString &protocol : protocols) {
    //        if (schemes.contains(protocol)) {
    //            return true;
    //        }
    //    }
    //}

    return false;
}

bool SFilePlacesItem::isDevice() const
{
    return !bookmark().metaDataItem(QStringLiteral("UDI")).isEmpty();
}

KBookmark SFilePlacesItem::bookmark() const
{
    return m_bookmark;
}

void SFilePlacesItem::setBookmark(const KBookmark &bookmark)
{
    m_bookmark = bookmark;

    updateDeviceInfo(m_bookmark.metaDataItem(QStringLiteral("UDI")));

    if (bookmark.metaDataItem(QStringLiteral("isSystemItem")) == QLatin1String("true")) {
        // This context must stay as it is - the translated system bookmark names
        // are created with 'KFile System Bookmarks' as their context, so this
        // ensures the right string is picked from the catalog.
        // (coles, 13th May 2009)

        m_text = i18nc("KFile System Bookmarks", bookmark.text().toUtf8().data());
    } else {
        m_text = bookmark.text();
    }

    const SFilePlacesModel::GroupType type = groupType();
    switch (type) {
    case SFilePlacesModel::PlacesType:
        m_groupName = i18nc("@item", "Places");
        break;
    case SFilePlacesModel::RemoteType:
        m_groupName = i18nc("@item", "Remote");
        break;
    case SFilePlacesModel::RecentlySavedType:
        m_groupName = i18nc("@item The place group section name for recent dynamic lists", "Recent");
        break;
    case SFilePlacesModel::SearchForType:
        m_groupName = i18nc("@item", "Search For");
        break;
    case SFilePlacesModel::DevicesType:
        m_groupName = i18nc("@item", "Devices");
        break;
    case SFilePlacesModel::RemovableDevicesType:
        m_groupName = i18nc("@item", "Removable Devices");
        break;
    case SFilePlacesModel::TagsType:
        m_groupName = i18nc("@item", "Tags");
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

QVariant SFilePlacesItem::data(int role) const
{
    if (role == SFilePlacesModel::GroupRole) {
        return QVariant(m_groupName);
    } else if (role != SFilePlacesModel::HiddenRole && role != Qt::BackgroundRole && isDevice()) {
        return deviceData(role);
    } else {
        return bookmarkData(role);
    }
}

SFilePlacesModel::GroupType SFilePlacesItem::groupType() const
{
    if (!isDevice()) {
        const QString protocol = bookmark().url().scheme();
        if (protocol == QLatin1String("timeline") || protocol == QLatin1String("recentlyused")) {
            return SFilePlacesModel::RecentlySavedType;
        }

        if (protocol.contains(QLatin1String("search"))) {
            return SFilePlacesModel::SearchForType;
        }

        if (protocol == QLatin1String("bluetooth") || protocol == QLatin1String("obexftp") || protocol == QLatin1String("kdeconnect")) {
            return SFilePlacesModel::DevicesType;
        }

        if (protocol == QLatin1String("tags")) {
            return SFilePlacesModel::TagsType;
        }

        if (protocol == QLatin1String("remote") || KProtocolInfo::protocolClass(protocol) != QLatin1String(":local")) {
            return SFilePlacesModel::RemoteType;
        } else {
            return SFilePlacesModel::PlacesType;
        }
    }

    //if (m_drive && (m_drive->isHotpluggable() || m_drive->isRemovable())) {
    //    return SFilePlacesModel::RemovableDevicesType;
    ////} else if (m_networkShare) {
    ////    return SFilePlacesModel::RemoteType;
    //} else {
        return SFilePlacesModel::DevicesType;
    //}
}

bool SFilePlacesItem::isHidden() const
{
    return m_bookmark.metaDataItem(QStringLiteral("IsHidden")) == QLatin1String("true");
}

void SFilePlacesItem::setHidden(bool hide)
{
    if (m_bookmark.isNull() || isHidden() == hide) {
        return;
    }
    m_bookmark.setMetaDataItem(QStringLiteral("IsHidden"), hide ? QStringLiteral("true") : QStringLiteral("false"));
}

QVariant SFilePlacesItem::bookmarkData(int role) const
{
    KBookmark b = bookmark();

    if (b.isNull()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return m_text;
    case Qt::DecorationRole:
        return QIcon::fromTheme(iconNameForBookmark(b));
    case Qt::BackgroundRole:
        if (isHidden()) {
            return QColor(Qt::lightGray);
        } else {
            return QVariant();
        }
    case SFilePlacesModel::UrlRole:
        return b.url();
    case SFilePlacesModel::SetupNeededRole:
        return false;
    case SFilePlacesModel::HiddenRole:
        return isHidden();
    case SFilePlacesModel::IconNameRole:
        return iconNameForBookmark(b);
    default:
        return QVariant();
    }
}

QVariant SFilePlacesItem::deviceData(int role) const
{
    if (m_device.isValid()) {
        switch (role) {
        case Qt::DisplayRole:
            return m_device.displayName();
        case Qt::DecorationRole:
            // qDebug() << "adding emblems" << m_emblems << "to device icon" << m_deviceIconName;
            return QIcon::fromTheme("drive-harddisk");
        case SFilePlacesModel::UrlRole:
            return QUrl::fromLocalFile(m_device.rootPath());
        case SFilePlacesModel::SetupNeededRole:
            return !m_device.isReady();

        case SFilePlacesModel::FixedDeviceRole:
            return true;

        case SFilePlacesModel::CapacityBarRecommendedRole:
            return m_device.isReady();

        case SFilePlacesModel::IconNameRole:
            return "drive-harddisk";

        default:
            return QVariant();
        }
    } else {
        return QVariant();
    }
}

KBookmark SFilePlacesItem::createBookmark(KBookmarkManager *manager, const QString &label, const QUrl &url, const QString &iconName, SFilePlacesItem *after)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull()) {
        return KBookmark();
    }
    QString empty_icon = iconName;
    if (url.toString() == QLatin1String("trash:/")) {
        if (empty_icon.endsWith(QLatin1String("-full"))) {
            empty_icon.chop(5);
        } else if (empty_icon.isEmpty()) {
            empty_icon = QStringLiteral("user-trash");
        }
    }
    KBookmark bookmark = root.addBookmark(label, url, empty_icon);
    bookmark.setMetaDataItem(QStringLiteral("ID"), generateNewId());

    if (after) {
        root.moveBookmark(bookmark, after->bookmark());
    }

    return bookmark;
}

KBookmark SFilePlacesItem::createSystemBookmark(KBookmarkManager *manager,
                                                const char *translationContext,
                                                const QByteArray &untranslatedLabel,
                                                const QUrl &url,
                                                const QString &iconName,
                                                const KBookmark &after)
{
    Q_UNUSED(translationContext); // parameter is only necessary to force the caller
    // to provide a marked-for-translation string for the label, with context

    KBookmark bookmark = createBookmark(manager, QString::fromUtf8(untranslatedLabel), url, iconName);
    if (!bookmark.isNull()) {
        bookmark.setMetaDataItem(QStringLiteral("isSystemItem"), QStringLiteral("true"));
    }
    if (!after.isNull()) {
        manager->root().moveBookmark(bookmark, after);
    }
    return bookmark;
}

KBookmark SFilePlacesItem::createDeviceBookmark(KBookmarkManager *manager, const QString &udi)
{
    KBookmarkGroup root = manager->root();
    if (root.isNull()) {
        return KBookmark();
    }
    KBookmark bookmark = root.createNewSeparator();
    bookmark.setMetaDataItem(QStringLiteral("UDI"), udi);
    bookmark.setMetaDataItem(QStringLiteral("isSystemItem"), QStringLiteral("true"));
    return bookmark;
}

KBookmark SFilePlacesItem::createTagBookmark(KBookmarkManager *manager, const QString &tag)
{
    KBookmark bookmark = createSystemBookmark(manager, tag.toUtf8().data(), tag.toUtf8(), QUrl(QLatin1String("tags:/") + tag), QLatin1String("tag"));
    bookmark.setMetaDataItem(QStringLiteral("tag"), tag);
    bookmark.setMetaDataItem(QStringLiteral("isSystemItem"), QStringLiteral("true"));

    return bookmark;
}

QString SFilePlacesItem::generateNewId()
{
    static int count = 0;

    //    return QString::number(count++);

    return QString::number(QDateTime::currentSecsSinceEpoch()) + QLatin1Char('/') + QString::number(count++);

    //    return QString::number(QDateTime::currentSecsSinceEpoch())
    //         + '/' + QString::number(qrand());
}

void SFilePlacesItem::onAccessibilityChanged(bool isAccessible)
{
    m_isAccessible = isAccessible;
    m_isCdrom = m_device.fileSystemType() == "iso9660";

    Q_EMIT itemChanged(id());
}

QString SFilePlacesItem::iconNameForBookmark(const KBookmark &bookmark) const
{
    if (!m_folderIsEmpty && isTrash(bookmark)) {
        return bookmark.icon() + QLatin1String("-full");
    } else {
        return bookmark.icon();
    }
}

#include "moc_sfileplacesitem_p.cpp"
