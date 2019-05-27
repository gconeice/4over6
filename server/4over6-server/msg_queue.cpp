//
// Created by root on 17-5-19.
//

#include "msg_queue.h"
template <typename T>
void MsgQueue<T>::pop(T& elem) {
    std::unique_lock<std::mutex> lck(mutex_);
    cond_.wait(lck, [this]() {return !queue_.empty();});
    elem = std::move(queue_.front());
    queue_.pop();
}

template <typename T>
bool MsgQueue<T>::empty() const {
    std::lock_guard<std::mutex> lck(mutex_);
    return queue_.empty();
}

template <typename T>
void MsgQueue<T>::push(const T& elem) {
    {
        std::lock_guard<std::mutex> lck(mutex_);
        queue_.push(elem);
    }
    cond_.notify_one();
}

template <typename T>
void MsgQueue<T>::push(T& elem) {
    {
        std::lock_guard<std::mutex> lck(mutex_);
        queue_.push(std::move(elem));
    }
    cond_.notify_one();
}