#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include "../Components/InputEvent.h"
#include <functional>

using InputEventCallback = std::function<bool(const InputEvent&)>;

using SubscribersID = unsigned int;

class Dispatcher
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

        void unsubscribe(Subscriber* subscriberPtr)
        {
            subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                [subscriberPtr](const Subscriber& subscriber) { return &subscriber == subscriberPtr; }),
                subscribers.end());
        }

        void unsubscribeAll()
        {
            subscribers.clear();
        }

        void dispatch(const InputEvent& event)
        {
            std::vector<SubscribersID> toRemove;
            for (const auto& subscriber : subscribers) {
                try {
                    std::cout << "Dispatcher: Notifying subscriber ID " << subscriber.id << std::endl;
                    bool handled = subscriber.callback(event);
                    std::cout << "Dispatcher: Subscriber ID " << subscriber.id << " handled event: " << (handled ? "yes" : "no") << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Exception in subscriber ID " << subscriber.id << ": " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown exception in subscriber ID " << subscriber.id << std::endl;
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