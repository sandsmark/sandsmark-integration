/*
 * This file is part of the KDE project.
 * Copyright (C) 2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *
 * You can Freely distribute this program under the GNU Library General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#pragma once

#include <QHash>
#include <QStackedWidget>
#include <kpreviewwidgetbase.h>
#include <qmimetype.h>

// Internal, but exported for KDirOperator (kfile) and KPreviewProps (kdelibs4support)
class SFileMetaPreview : public KPreviewWidgetBase
{
    Q_OBJECT

public:
    explicit SFileMetaPreview(QWidget *parent);
    ~SFileMetaPreview();

    virtual void addPreviewProvider(const QString &mimeType,
                                    KPreviewWidgetBase *provider);
    virtual void clearPreviewProviders();

public Q_SLOTS:
    void showPreview(const QUrl &url) override;
    void clearPreview() override;

protected:
    virtual KPreviewWidgetBase *previewProviderFor(const QString &mimeType);

private:
    void initPreviewProviders();
    KPreviewWidgetBase *findExistingProvider(const QString &mimeType, const QMimeType &mimeInfo) const;

    QStackedWidget *m_stack;
    QWidget *m_blankWidget;
    QHash<QString, KPreviewWidgetBase *> m_previewProviders;

private:
    class SFileMetaPreviewPrivate;
    SFileMetaPreviewPrivate *d;
};

