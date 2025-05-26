#pragma once

#include <vector>

namespace observer 
{
    /**
     * Interface for the Observer
     */
    class Observer
    {
    public:
        /**
         * Update the state of this observer
         */
        virtual void on_notify(void* data) = 0;

    };

    /**
     * Interface for the Subject
     */
    class Subject {

    public:

        /**
         * Register an observer
         * @param observer the observer object to be registered
         */
        virtual void register_observer(Observer *observer) = 0;

        /**
         * Unregister an observer
         * @param observer the observer object to be unregistered
         */
        virtual void remove_observer(Observer *observer) = 0;

        /**
         * Notify all the registered observers when a change happens
         */
        virtual void notify_observers() = 0;

    protected:
        std::vector<Observer *> m_observers; // observers

    };
}
