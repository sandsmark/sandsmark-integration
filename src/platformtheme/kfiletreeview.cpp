/*
   This file is part of the KDE project

   Copyright (C) 2007 Tobias Koenig <tokoe@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfiletreeview_p.h"

#include <QtCore/QDir>
#include <QContextMenuEvent>
#include <QMenu>
#include <QUrl>

#include <kdirlister.h>
#include <kdirmodel.h>
#include <kdirsortfilterproxymodel.h>
#include <kfileitemdelegate.h>
#include <klocalizedstring.h>
#include <ktoggleaction.h>

class KFileTreeView::Private
{
public:
    Private(KFileTreeView *parent)
        : q(parent)
    {
    }

    QUrl urlForProxyIndex(const QModelIndex &index) const;

    void _k_activated(const QModelIndex &);
    void _k_currentChanged(const QModelIndex &, const QModelIndex &);
    void _k_expanded(const QModelIndex &);

    KFileTreeView *q;
    KDirModel *mSourceModel;
    KDirSortFilterProxyModel *mProxyModel;
};

QUrl KFileTreeView::Private::urlForProxyIndex(const QModelIndex &index) const
{
    const KFileItem item = mSourceModel->itemForIndex(mProxyModel->mapToSource(index));

    return !item.isNull() ? item.url() : QUrl();
}

void KFileTreeView::Private::_k_activated(const QModelIndex &index)
{
    const QUrl url = urlForProxyIndex(index);
    if (url.isValid()) {
        emit q->activated(url);
    }
}

void KFileTreeView::Private::_k_currentChanged(const QModelIndex &currentIndex, const QModelIndex &)
{
    const QUrl url = urlForProxyIndex(currentIndex);
    if (url.isValid()) {
        emit q->currentChanged(url);
    }
}

void KFileTreeView::Private::_k_expanded(const QModelIndex &baseIndex)
{
    QModelIndex index = mProxyModel->mapFromSource(baseIndex);

    q->selectionModel()->clearSelection();
    q->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
    q->scrollTo(index);
}

KFileTreeView::KFileTreeView(QWidget *parent)
    : QTreeView(parent), d(new Private(this))
{
    d->mSourceModel = new KDirModel(this);
    d->mProxyModel = new KDirSortFilterProxyModel(this);
    d->mProxyModel->setSourceModel(d->mSourceModel);

    setModel(d->mProxyModel);
    setItemDelegate(new KFileItemDelegate(this));
    setLayoutDirection(Qt::LeftToRight);

    d->mSourceModel->dirLister()->openUrl(QUrl::fromLocalFile(QDir::root().absolutePath()), KDirLister::Keep);

    connect(this, SIGNAL(activated(QModelIndex)),
            this, SLOT(_k_activated(QModelIndex)));
    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(_k_currentChanged(QModelIndex,QModelIndex)));

    connect(d->mSourceModel, SIGNAL(expand(QModelIndex)),
            this, SLOT(_k_expanded(QModelIndex)));
}

KFileTreeView::~KFileTreeView()
{
    delete d;
}

QUrl KFileTreeView::currentUrl() const
{
    return d->urlForProxyIndex(currentIndex());
}

QUrl KFileTreeView::selectedUrl() const
{
    if (!selectionModel()->hasSelection()) {
        return QUrl();
    }

    const QItemSelection selection = selectionModel()->selection();
    const QModelIndex firstIndex = selection.indexes().first();

    return d->urlForProxyIndex(firstIndex);
}

QList<QUrl> KFileTreeView::selectedUrls() const
{
    QList<QUrl> urls;

    if (!selectionModel()->hasSelection()) {
        return urls;
    }

    const QModelIndexList indexes = selectionModel()->selection().indexes();
    foreach (const QModelIndex &index, indexes) {
        const QUrl url = d->urlForProxyIndex(index);
        if (url.isValid()) {
            urls.append(url);
        }
    }

    return urls;
}

QUrl KFileTreeView::rootUrl() const
{
    return d->mSourceModel->dirLister()->url();
}

void KFileTreeView::setDirOnlyMode(bool enabled)
{
    d->mSourceModel->dirLister()->setDirOnlyMode(enabled);
    d->mSourceModel->dirLister()->openUrl(d->mSourceModel->dirLister()->url());
}

void KFileTreeView::setShowHiddenFiles(bool enabled)
{
    QUrl url = currentUrl();
    d->mSourceModel->dirLister()->setShowingDotFiles(enabled);
    d->mSourceModel->dirLister()->openUrl(d->mSourceModel->dirLister()->url());
    setCurrentUrl(url);
}

void KFileTreeView::setCurrentUrl(const QUrl &url)
{
    QModelIndex baseIndex = d->mSourceModel->indexForUrl(url);

    if (!baseIndex.isValid()) {
        d->mSourceModel->expandToUrl(url);
        return;
    }

    QModelIndex proxyIndex = d->mProxyModel->mapFromSource(baseIndex);
    selectionModel()->clearSelection();
    selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::SelectCurrent);
    scrollTo(proxyIndex);
}

void KFileTreeView::setRootUrl(const QUrl &url)
{
    d->mSourceModel->dirLister()->openUrl(url);
}

void KFileTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    KToggleAction *showHiddenAction = new KToggleAction(i18n("Show Hidden Folders"), &menu);
    showHiddenAction->setChecked(d->mSourceModel->dirLister()->showingDotFiles());
    connect(showHiddenAction, SIGNAL(toggled(bool)), this, SLOT(setShowHiddenFiles(bool)));

    menu.addAction(showHiddenAction);
    menu.exec(event->globalPos());
}

bool KFileTreeView::showHiddenFiles() const
{
    return d->mSourceModel->dirLister()->showingDotFiles();
}

QSize KFileTreeView::sizeHint() const
{
    // This size makes KDirSelectDialog pop up just under 800x600 by default :-)
    return QSize(680, 500);
}

#include "moc_kfiletreeview_p.cpp"
