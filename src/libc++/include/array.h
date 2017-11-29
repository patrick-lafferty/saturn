#pragma once

/*
Note: this should be std::vector, but since I don't have
allocators and what-not, it doesn't feel right to have a
half implemented vector, this is a temp replacement
*/
template<typename T>
class Array {
public:
    
    Array() {
        maxItems = 10;
        data = new T[maxItems];
    }
    
    void add(T item) {
        if (length >= maxItems) {
            auto oldSize = maxItems;
            maxItems *= 2;
            auto newData = new T[maxItems];
            memcpy(newData, data, sizeof(T) * oldSize);
            delete data;
            data = newData;
        }

        data[length] = item;
        length++;
    }

    T* begin() {
        return data;
    }

    T* end() {
        return data + length;
    }

    T& operator[](size_t pos) {
        return data[pos];
    }

    bool empty() const {
        return length == 0;
    }

    uint32_t size() const {
        return length;
    }

private:
    uint32_t length {0};
    uint32_t maxItems {0};
    T* data {nullptr};
};