#ifndef COMPONENTBASE_H
#define COMPONENTBASE_H

#include <QWidget>
#include <QString>
#include "smartpointers.h"
#include "resourcemanager.h"

namespace QuteNote {

class ComponentBase : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ComponentBase)

public:
    explicit ComponentBase(QWidget* parent = nullptr);
    ~ComponentBase() override;

    // Component lifecycle
    virtual void initializeComponent();
    virtual void setupComponent();
    virtual void cleanupComponent();
    virtual void refreshComponent();
    virtual void setupConnections();
    virtual void cleanupResources();

    // Resource management
    void registerResource(const QString& id, qint64 size);
    void unregisterResource(const QString& id);
    
    // State management
    bool isInitialized() const { return m_initialized; }
    QString componentName() const { return m_componentName; }

protected:
    // Resource tracking helpers
    void trackMemoryUsage(qint64 bytes);
    void untrackMemoryUsage(qint64 bytes);
    
    // Initialization helpers
    void setComponentName(const QString& name);
    void markInitialized() { m_initialized = true; }
    
    // Event handling
    void customEvent(QEvent* event) override;
    
    // Resource monitoring
    virtual void handleMemoryWarning();
    virtual void handleResourceLimit();

private:
    bool m_initialized;
    QString m_componentName;
    qint64 m_memoryUsage;

Q_SIGNALS:
    void componentInitialized();
    void componentCleanupStarted();
    void componentCleanupFinished();
    void memoryUsageChanged(qint64 bytes);
};

} // namespace QuteNote

#endif // COMPONENTBASE_H