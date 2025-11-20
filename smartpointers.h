#ifndef SMARTPOINTERS_H
#define SMARTPOINTERS_H

#include <QPointer>
#include <QObject>
#include <memory>

namespace QuteNote {

// For QObject-derived classes, use QPointer for weak references
template<typename T>
using WeakPtr = QPointer<T>;

// For QObject-derived classes that need ownership semantics
template<typename T>
using OwnedPtr = std::unique_ptr<T, std::function<void(T*)>>;

// Factory function for creating owned QObject pointers
template<typename T, typename... Args>
OwnedPtr<T> makeOwned(Args&&... args) {
    return OwnedPtr<T>(new T(std::forward<Args>(args)...), 
                       [](T* ptr) { ptr->deleteLater(); });
}

// For non-QObject classes
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// For shared ownership
template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// Helper for managing singleton instances
template<typename T>
class Singleton {
public:
    static T* instance() {
        static OwnedPtr<T> s_instance{
            new T(),
            [](T* ptr) { ptr->deleteLater(); }
        };
        return s_instance.get();
    }
    
protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    
private:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

} // namespace QuteNote

#endif // SMARTPOINTERS_H