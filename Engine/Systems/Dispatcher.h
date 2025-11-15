#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include "../Components/InputEvent.h"
#include <functional>

using InputEventCallback = std::function<bool(const InputEvent&)>;

using SubscribersID = unsigned int;

struct Dispatcher
{
    public:
        struct Subscriber
        {
            SubscribersID id;
            int priority;
            InputEventCallback callback;
            bool once;
        };
    
        Dispatcher() : nextSubscriberID(1) {}

        SubscribersID subscribe(InputEventCallback callback, int priority = 0, bool once = false)
        {
            Subscriber subscriber{ nextSubscriberID++, priority, callback, once };
            subscribers.push_back(subscriber);
            // Sort subscribers by priority (higher priority first)
            std::sort(subscribers.begin(), subscribers.end(), [](const Subscriber& a, const Subscriber& b) {
                return a.priority > b.priority;
            });
            return subscriber.id;
        }

        void unsubscribe(SubscribersID id)
        {
            subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                [id](const Subscriber& subscriber) { return subscriber.id == id; }),
                subscribers.end());
        }

        void dispatch(const InputEvent& event)
        {
            std::vector<SubscribersID> toRemove;
            for (const auto& subscriber : subscribers)
            {
                bool handled = subscriber.callback(event);
                if (handled && subscriber.once)
                {
                    toRemove.push_back(subscriber.id);
                }
                if (handled)
                {
                    break; // Stop propagation if event is handled
                }
            }
            // Remove one-time subscribers that have handled the event
            for (SubscribersID id : toRemove)
            {
                unsubscribe(id);
            }
        }
    private :
        SubscribersID nextSubscriberID;
        std::vector<Subscriber> subscribers;
};

#endif // DISPATCHER_H