#ifndef FILEWATCHERGUARD_H
#define FILEWATCHERGUARD_H

#include <QFileSystemWatcher>
#include <QScopedPointer>
#include <QObject>
#include <QStringList>

class FileWatcherGuard : public QObject
{
    Q_OBJECT

public:
    explicit FileWatcherGuard(QObject *parent = nullptr);
    ~FileWatcherGuard();

    // File watching operations
    bool addPath(const QString &path);
    bool addPaths(const QStringList &paths);
    bool removePath(const QString &path);
    bool removePaths(const QStringList &paths);
    QStringList files() const;
    QStringList directories() const;

    // RAII helper methods
    class ScopedWatch {
    public:
        ScopedWatch(FileWatcherGuard *guard, const QString &path)
            : m_guard(guard), m_path(path)
        {
            if (m_guard) {
                m_guard->addPath(m_path);
            }
        }
        
        ~ScopedWatch() {
            if (m_guard) {
                m_guard->removePath(m_path);
            }
        }
        
    private:
        FileWatcherGuard *m_guard;
        QString m_path;
        Q_DISABLE_COPY(ScopedWatch)
    };

    // Factory method for scoped watches
    ScopedWatch *watchPath(const QString &path) {
        return new ScopedWatch(this, path);
    }

Q_SIGNALS:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);
    void watcherError(const QString &path, const QString &error);

private:
    void setupConnections();
    void handleFileError(const QString &path);

    QScopedPointer<QFileSystemWatcher> m_watcher;
    QStringList m_watchedPaths;
};

#endif // FILEWATCHERGUARD_H