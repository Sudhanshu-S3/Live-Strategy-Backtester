#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <atomic>
#include <memory>
#include <optional>

template <typename T>
class ThreadSafeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;
        Node(std::shared_ptr<T> data) : data(std::move(data)), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    ThreadSafeQueue() : head(new Node(nullptr)), tail(head.load()) {}
    ~ThreadSafeQueue() {
        while (Node* const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(std::shared_ptr<T> new_value) {
        auto new_data = std::make_shared<T>(std::move(*new_value));
        Node* new_node = new Node(new_data);
        Node* old_tail = tail.load();
        while (!tail.compare_exchange_weak(old_tail, new_node)) {}
        old_tail->next = new_node;
    }

    std::optional<std::shared_ptr<T>> try_pop() {
        Node* old_head = head.load();
        while (old_head != tail.load() && !head.compare_exchange_weak(old_head, old_head->next)) {}
        if (old_head == tail.load()) {
            return std::nullopt;
        }
        return old_head->next.load()->data;
    }
};

#endif // THREAD_SAFE_QUEUE_H