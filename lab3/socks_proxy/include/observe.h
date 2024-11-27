#include <vector>
#include <iostream>

#pragma once

namespace observe {
    class Event {
        public:
            enum class EvType {
                ADD,
                DEL,
                INV
            };

        public:
            virtual int get_data1() noexcept {
                return -1;
            }

            virtual int get_data2() noexcept {
                return -1;
            }

            virtual EvType get_type() noexcept {
                return EvType::INV;
            }
    };

    class Observer {
        public:
            virtual ~Observer() = default;
            virtual void update(Event &e) noexcept = 0;
    };

    class Observable {
        protected:
            std::vector<Observer*> observers_;
        
        public:
            void add(Observer *observer) {
                observers_.push_back(observer);
            }

            void update(Event &event) noexcept {
                for (Observer *ob: observers_)
                    ob->update(event);
            }
    };
}