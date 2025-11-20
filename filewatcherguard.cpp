#include "filewatcherguard.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

FileWatcherGuard::FileWatcherGuard(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
{
    setupConnections();
}

FileWatcherGuard::~FileWatcherGuard()
{
    // QScopedPointer will handle m_watcher cleanup
    if (!m_watchedPaths.isEmpty()) {
        m_watcher->removePaths(m_watchedPaths);
    }
}

void FileWatcherGuard::setupConnections()
{
    connect(m_watcher.data(), &QFileSystemWatcher::fileChanged,
            this, [this](const QString &path) {
        if (!QFileInfo::exists(path)) {
            handleFileError(path);
        } else {
            emit fileChanged(path);
        }
    });

    connect(m_watcher.data(), &QFileSystemWatcher::directoryChanged,
            this, &FileWatcherGuard::directoryChanged);
}

bool FileWatcherGuard::addPath(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        emit watcherError(path, "File does not exist");
        return false;
    }

    if (m_watchedPaths.contains(path)) {
        return true; // Already watching
    }

    if (m_watcher->addPath(path)) {
        m_watchedPaths.append(path);
        return true;
    }

    emit watcherError(path, "Failed to add watch");
    return false;
}

bool FileWatcherGuard::addPaths(const QStringList &paths)
{
    bool allSuccess = true;
    for (const QString &path : paths) {
        if (!addPath(path)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool FileWatcherGuard::removePath(const QString &path)
{
    if (m_watcher->removePath(path)) {
        m_watchedPaths.removeOne(path);
        return true;
    }
    return false;
}

bool FileWatcherGuard::removePaths(const QStringList &paths)
{
    bool allSuccess = true;
    for (const QString &path : paths) {
        if (!removePath(path)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

QStringList FileWatcherGuard::files() const
{
    return m_watcher->files();
}

QStringList FileWatcherGuard::directories() const
{
    return m_watcher->directories();
}

void FileWatcherGuard::handleFileError(const QString &path)
{
    // File was deleted or moved
    removePath(path);
    emit watcherError(path, "File no longer exists");
}