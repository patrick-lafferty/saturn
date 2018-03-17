(grid
    (margin 5)

    (items 
        (label (caption "Menu"))
        (textbox (bind text commandLine))
        (grid 
            (bind items availableCommands)
            (auto-columns 1 1)
            (column-gap 3))))


createUI(SExp* root, [&](auto binding, Element string_view name) {
    if (name.compare("commandLine") == 0) {
        //binding.getter([&]{return commandLine;});
        //binding.setter([&]({})
        //commandLine = binding;
        
        binding.bindTo(commandLine);

        commandLine.subscribe([=](auto value) {
            binding->setValue(value);
        });
    }
    else if (name.compare("availableCommands") == 0) {

    }
});

application code:

Observable commandLine;

onInput(c) {
    commandLine.setValue(input);
}

template<class T>
class Observable {
    void setValue(T value) {
        propagateValue(value);
    }

    void subscribe(std::function<void(T)> subscriber) {
        propagateValue = subscriber;
    }
}

class MultiObservable {

}

needs to call onChange to re-render text

class TextBox {

    enum class Bindings {
        Text
    };

    Bindable<TextBox, Bindings> text;

    TextBox() {
        text.bind(this, Bindings::Text);
    }

    void onChange(Bindings b) {
        switch (b) {
            case Bindings::Text: {
                break;
            }
        }
    }
}

template<class T, class B>
Bindable {
    void setValue() {
        if (owner) {
            owner->onChange(binding);
        }
    }

    T owner;
    B binding;
}