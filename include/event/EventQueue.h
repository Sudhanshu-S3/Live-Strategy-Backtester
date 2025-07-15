#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "Event.h"

/**
 * A thread-safe queue for passing events between different components of the trading system.
 * It uses a std::mutex to protect access to the queue and a std::condition_variable
 * to allow consumer threads to wait efficiently for new events to arrive.
 */
class EventQueue {
public:
    EventQueue() = default;
    // Delete copy constructor and assignment operator to prevent copying
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    /**
     * @brief Pushes a new event onto the queue.
     * 
     * @param event A shared pointer to the event to be added.
     */
    void push(std::shared_ptr<Event> event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(event));
        m_cond.notify_one(); // Notify one waiting thread that an item is available
    }

    /**
     * @brief Waits until an event is available and then pops it from the queue.
     * 
     * @return A shared pointer to the event from the front of the queue.
     */
    std::shared_ptr<Event> wait_and_pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        // Wait for the condition that the queue is not empty
        m_cond.wait(lock, [this] { return !m_queue.empty(); });
        
        std::shared_ptr<Event> event = m_queue.front();
        m_queue.pop();
        return event;
    }

    /**
     * @brief Checks if the queue is empty.
     * 
     * @return true if the queue is empty, false otherwise.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    mutable std::mutex m_mutex;
    std::queue<std::shared_ptr<Event>> m_queue;
    std::condition_variable m_cond;
};

#endif // EVENT_QUEUE_H