/*
 * This file is part of the KDE project.
 * Copyright (C) 2003 Carsten Pfeiffer <pfeiffer@kde.org>
 *
 * You can Freely distribute this program under the GNU Library General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include "sfilemetapreview.h"

#include <QLayout>
#include <qmimedatabase.h>

#include <QDebug>
#include <kio/previewjob.h>
#include <QPluginLoader>
#include <KPluginFactory>
#include <kimagefilepreview.h>

bool SFileMetaPreview::s_tryAudioPreview = false;

SFileMetaPreview::SFileMetaPreview(QWidget *parent)
    : KPreviewWidgetBase(parent),
      haveAudioPreview(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);
    m_blankWidget = new QWidget(this);
    m_stack->addWidget(m_blankWidget);

    // ###
//     m_previewProviders.setAutoDelete( true );
    initPreviewProviders();
}

SFileMetaPreview::~SFileMetaPreview()
{
}

void SFileMetaPreview::initPreviewProviders()
{
    qDeleteAll(m_previewProviders);
    m_previewProviders.clear();
    // hardcoded so far

    // image previews
    KImageFilePreview *imagePreview = new KImageFilePreview(m_stack);
    (void) m_stack->addWidget(imagePreview);
    m_stack->setCurrentWidget(imagePreview);
    resize(imagePreview->sizeHint());

    const QStringList mimeTypes = imagePreview->supportedMimeTypes();
    QStringList::ConstIterator it = mimeTypes.begin();
    for (; it != mimeTypes.end(); ++it) {
//         qDebug(".... %s", (*it).toLatin1().constData());
        m_previewProviders.insert(*it, imagePreview);
    }
}

KPreviewWidgetBase *SFileMetaPreview::findExistingProvider(const QString &mimeType, const QMimeType &mimeInfo) const
{
    KPreviewWidgetBase *provider = m_previewProviders.value(mimeType);
    if (provider) {
        return provider;
    }

    if (mimeInfo.isValid()) {
        // check mime type inheritance
        const QStringList parentMimeTypes = mimeInfo.allAncestors();
        for (const QString &parentMimeType : parentMimeTypes) {
            provider = m_previewProviders.value(parentMimeType);
            if (provider) {
                return provider;
            }
        }
    }

    // ### mimetype may be image/* for example, try that
    const int index = mimeType.indexOf(QLatin1Char('/'));
    if (index > 0) {
        provider = m_previewProviders.value(mimeType.leftRef(index + 1) + QLatin1Char('*'));
        if (provider) {
            return provider;
        }
    }

    return nullptr;
}

KPreviewWidgetBase *SFileMetaPreview::previewProviderFor(const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType mimeInfo = db.mimeTypeForName(mimeType);

    //     qDebug("### looking for: %s", mimeType.toLatin1().constData());
    // often the first highlighted item, where we can be sure, there is no plugin
    // (this "folders reflect icons" is a konq-specific thing, right?)
    //if (mimeInfo.inherits(QStringLiteral("inode/directory"))) {
    //    return nullptr;
    //}

    KPreviewWidgetBase *provider = findExistingProvider(mimeType, mimeInfo);
    if (provider) {
        return provider;
    }

//qDebug("#### didn't find anything for: %s", mimeType.toLatin1().constData());

    if (s_tryAudioPreview &&
            !mimeType.startsWith(QLatin1String("text/")) &&
            !mimeType.startsWith(QLatin1String("image/"))) {
        if (!haveAudioPreview) {
            KPreviewWidgetBase *audioPreview = createAudioPreview(m_stack);
            if (audioPreview) {
                haveAudioPreview = true;
                (void) m_stack->addWidget(audioPreview);
                const QStringList mimeTypes = audioPreview->supportedMimeTypes();
                QStringList::ConstIterator it = mimeTypes.begin();
                for (; it != mimeTypes.end(); ++it) {
                    // only add non already handled mimetypes
                    if (m_previewProviders.constFind(*it) == m_previewProviders.constEnd()) {
                        m_previewProviders.insert(*it, audioPreview);
                    }
                }
            }
        }
    }

    // with the new mimetypes from the audio-preview, try again
    provider = findExistingProvider(mimeType, mimeInfo);
    if (provider) {
        return provider;
    }

    // The logic in this code duplicates the logic in PreviewJob.
    // But why do we need multiple KPreviewWidgetBase instances anyway?

    return nullptr;
}

void SFileMetaPreview::showPreview(const QUrl &url)
{
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForUrl(url);
    KPreviewWidgetBase *provider = previewProviderFor(mt.name());
    if (provider) {
        if (provider != m_stack->currentWidget()) { // stop the previous preview
            clearPreview();
        }

        m_stack->setEnabled(true);
        m_stack->setCurrentWidget(provider);
        provider->showPreview(url);
    } else {
        qWarning() << "Failed to find provider";
        clearPreview();
        m_stack->setEnabled(false);
        m_stack->setCurrentWidget(m_blankWidget);
    }
}

void SFileMetaPreview::clearPreview()
{
    KPreviewWidgetBase *previewWidget = qobject_cast<KPreviewWidgetBase *>(m_stack->currentWidget());
    if (previewWidget && !qobject_cast<KImageFilePreview*>(previewWidget)) {
        previewWidget->clearPreview();
    } else {
        qWarning() << "No current preview";
    }
    m_stack->setCurrentWidget(m_blankWidget);
}

void SFileMetaPreview::addPreviewProvider(const QString &mimeType,
        KPreviewWidgetBase *provider)
{
    m_previewProviders.insert(mimeType, provider);
}

void SFileMetaPreview::clearPreviewProviders()
{
    QHash<QString, KPreviewWidgetBase *>::const_iterator i = m_previewProviders.constBegin();
    while (i != m_previewProviders.constEnd()) {
        m_stack->removeWidget(i.value());
        ++i;
    }
    qDeleteAll(m_previewProviders);
    m_previewProviders.clear();
}

// static
KPreviewWidgetBase *SFileMetaPreview::createAudioPreview(QWidget *parent)
{
    if (!s_tryAudioPreview) {
        return nullptr;
    }
    QPluginLoader loader(QStringLiteral("kfileaudiopreview"));
    KPluginFactory *factory = qobject_cast<KPluginFactory*>(loader.instance());
    if (!factory) {
        qWarning() << "Couldn't load kfileaudiopreview" << loader.errorString();
        s_tryAudioPreview = false;
        return nullptr;
    }
    KPreviewWidgetBase *w = factory->create<KPreviewWidgetBase>(parent);
    if (w) {
        w->setObjectName(QStringLiteral("kfileaudiopreview"));
    }
    return w;
}
