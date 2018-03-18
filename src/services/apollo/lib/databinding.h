/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <functional>

namespace Apollo {
    
    /*
    Observable is a wrapper for a type that sends a notification
    each time the value it wraps changes.
    */
    template<class T>
    class Observable {
    public:

        std::function<T(void)> subscribe(std::function<void()> s) {
            onChange = s;
            return [&]() {return value;};
        }

        void setValue(T newValue) {
            value = newValue;

            if (onChange) {
                onChange();
            }
        }

    private:

        /*
        The event handler to call whenever value changes
        */
        std::function<void()> onChange;
        T value;
    };

    /*
    An observer for a type that notifies on other changes besides
    assignment, such as adding/removing/modifying elements in
    the collection.
    */
    template<class T>
    class ObservableCollection
        : Observable<T> {
    public:

        void add() {}
    };

    /*
    A Bindable is the link between an Observable and a
    UI Element. An Application can own any number of
    observable values. UI Elements can expose certain
    values that an application might want to have access
    to. Bindable can subscribe to notification changes
    from an Observable, and then notify its parent
    UI Element about the change, and allows the element
    to access the value.

    The template parameter Binding is expected to be
    an Enum Class that identifies a UI Element-specific
    property that can be bound. For example, 
    TextBox::Bindings holds all of the properties that
    can be bound, such as its Text value. This is an
    easy way to identify which property changed when
    notifying the parent UI object.
    */
    template<class Owner, class Binding, class Value>
    class Bindable {
    public:

        using ValueType = Value;

        Bindable(Owner* owner, Binding binding)
            : owner {owner}, binding {binding} {}

        /*
        Indicates to the parent UI element that the
        Observable this binding subscribed to changed
        its value.
        */
        void notifyChange() {
            if (owner) {
                owner->onChange(binding);
            }
        }

        /*
        Gets the current value from the Observable
        */
        Value getValue() {
            if (getter) {
                return getter();
            }
            else {
                return {};
            }
        }

        /*
        Subscribes to change events from the Observable,
        and gets a getter function from it.
        */
        void bindTo(Observable<Value>& o) {
            getter = o.subscribe([&]() {
                notifyChange();
            });
        }

    private:

        /*
        The parent UI object
        */
        Owner* owner;

        Binding binding;
        std::function<Value(void)> getter;
    };

}
