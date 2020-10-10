#pragma once

#include <memory>
#include <mutex>



template<typename T>
class Singleton {
    static std::unique_ptr<T> instance;
    static std::once_flag onceFlag;
public:
    static T& GetInstance() {
        std::call_once(onceFlag, [] {
                instance = std::make_unique<T>();
            });
        return *instance;
    }
private:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance;
template<typename T>
std::once_flag Singleton<T>::onceFlag;


