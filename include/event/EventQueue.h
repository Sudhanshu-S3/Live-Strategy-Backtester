#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <memory>
#include "Event.h"

// Define EventQueue as a type alias for a queue of shared pointers to Event objects.
// This makes the code cleaner and easier to read, as components can
// simply use EventQueue instead of specifying the full type every time.

using EventQueue = std::queue<std::shared_ptr<Event>>;

#endif // EVENT_QUEUE_H