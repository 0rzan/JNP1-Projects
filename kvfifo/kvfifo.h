#ifndef KVFIFO_H
#define KVFIFO_H

#include <concepts>
#include <list>
#include <map>
#include <memory>
#include <utility>

namespace
{
template <typename K, typename V> using list_t = std::list<std::pair<K, V>>;

template <typename K, typename V>
using map_t = std::map<K, std::list<typename list_t<K, V>::iterator>>;
} // namespace

template <typename K, typename V>
requires std::totally_ordered<K> && std::semiregular<K>
    && std::copy_constructible<V> struct k_iterator
{
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = K;
    using pointer = K *;
    using reference = K &;

    k_iterator() = default;
    k_iterator(map_t<K, V>::iterator const &initial_iterator)
        : map_iterator(initial_iterator)
    {}
    k_iterator(k_iterator<K, V> const &other) : map_iterator(other.map_iterator)
    {}
    ~k_iterator() = default;

    k_iterator<K, V>& operator= (k_iterator<K, V> const &other)
    {
        map_iterator = other.map_iterator;
        return *this;
    }

    K const &operator*() const noexcept
    {
        return map_iterator->first;
    }

    k_iterator<K, V> &operator++() noexcept
    {
        map_iterator++;
        return *this;
    }

    k_iterator<K, V> operator++(int)
    {
        k_iterator<K, V> tmp = *this;
        ++(*this);
        return tmp;
    }

    k_iterator<K, V> &operator--() noexcept
    {
        map_iterator--;
        return *this;
    }

    k_iterator<K, V> operator--(int)
    {
        k_iterator<K, V> tmp = *this;
        --(*this);
        return tmp;
    }

    bool operator==(k_iterator<K, V> const &other) const = default;

  private:
    map_t<K, V>::iterator map_iterator;
};

template <typename K, typename V>
requires std::totally_ordered<K> && std::semiregular<K>
    && std::copy_constructible<V> class kvfifo
{
  public:
    kvfifo()
        : elements(std::make_shared<list_t<K, V>>()),
          elements_by_key(std::make_shared<map_t<K, V>>())
    {}

    kvfifo(kvfifo const &other)
        : elements(other.elements), elements_by_key(other.elements_by_key)
    {
        if (other.cannot_share)
            detach();
    }

    kvfifo(kvfifo &&other) noexcept
        : elements(std::move(other.elements)),
          elements_by_key(std::move(other.elements_by_key))
    {
        other.elements = empty_list();
        other.elements_by_key = empty_map();
        other.cannot_share = false;
    }

    ~kvfifo() = default;

    kvfifo &operator=(kvfifo other) noexcept
    {
        other.swap(*this);
        return *this;
    }

    void push(K const &k, V const &v)
    {
        throw_exception_if_moved();

        try_detach();

        elements->push_back({k, v});
        try
        {
            auto new_element_iter = std::prev(elements->end());
            auto map_iter = elements_by_key->find(k);
            if (map_iter == elements_by_key->end())
                elements_by_key->insert({k, {new_element_iter}});
            else
                map_iter->second.push_back(new_element_iter);
        }
        catch (...)
        {
            elements->pop_back();
            throw;
        }
    }

    void pop()
    {
        throw_exception_if_moved();

        try_detach();

        if (elements->empty())
            throw std::invalid_argument("Queue is empty");

        auto &elements_of_same_key = elements_by_key->find(elements->front().first)->second;
        elements_of_same_key.pop_front();

        if (elements_of_same_key.empty())
            elements_by_key->erase(elements->front().first);

        elements->pop_front();
    }

    void pop(K const &k)
    {
        throw_exception_if_moved();

        try_detach();

        auto map_iter = elements_by_key->find(k);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        elements->erase(elements_of_same_key.front());
        elements_of_same_key.pop_front();

        if (elements_of_same_key.empty())
            elements_by_key->erase(k);
    }

    void move_to_back(K const &k)
    {
        throw_exception_if_moved();

        try_detach();

        auto map_iter = elements_by_key->find(k);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        for (const auto &elements_iter : elements_of_same_key)
            elements->splice(elements->end(), *elements, elements_iter);
    }

    std::pair<K const &, V &> front()
    {
        throw_exception_if_moved();

        try_detach();
        cannot_share = true;

        if (elements->empty())
            throw std::invalid_argument("Queue is empty");

        return {elements->front().first, elements->front().second};
    }

    std::pair<K const &, V const &> front() const
    {
        throw_exception_if_moved();

        if (elements->empty())
            throw std::invalid_argument("Queue is empty");

        return {elements->front().first, elements->front().second};
    }

    std::pair<K const &, V &> back()
    {
        throw_exception_if_moved();

        try_detach();
        cannot_share = true;

        if (elements->empty())
            throw std::invalid_argument("Queue is empty");

        return {elements->back().first, elements->back().second};
    }

    std::pair<K const &, V const &> back() const
    {
        throw_exception_if_moved();

        if (elements->empty())
            throw std::invalid_argument("Queue is empty");

        return {elements->back().first, elements->back().second};
    }

    std::pair<K const &, V &> first(K const &key)
    {
        throw_exception_if_moved();

        try_detach();
        cannot_share = true;

        auto map_iter = elements_by_key->find(key);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        return {elements_of_same_key.front()->first, elements_of_same_key.front()->second};
    }

    std::pair<K const &, V const &> first(K const &key) const
    {
        throw_exception_if_moved();

        auto map_iter = elements_by_key->find(key);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        return {elements_of_same_key.front()->first, elements_of_same_key.front()->second};
    }

    std::pair<K const &, V &> last(K const &key)
    {
        throw_exception_if_moved();

        try_detach();
        cannot_share = true;

        auto map_iter = elements_by_key->find(key);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        return {elements_of_same_key.back()->first, elements_of_same_key.back()->second};
    }

    std::pair<K const &, V const &> last(K const &key) const
    {
        throw_exception_if_moved();

        auto map_iter = elements_by_key->find(key);
        if (map_iter == elements_by_key->end())
            throw std::invalid_argument("No element with given key");

        auto &elements_of_same_key = map_iter->second;
        return {elements_of_same_key.back()->first, elements_of_same_key.back()->second};
    }

    size_t size() const noexcept
    {
        if (!elements.get())
            return 0;
        return elements->size();
    }

    bool empty() const noexcept
    {
        if (!elements)
            return true;
        return elements->empty();
    }

    size_t count(K const &k) const
    {
        if (!elements)
            return 0;

        auto map_iter = elements_by_key->find(k);
        if (map_iter == elements_by_key->end())
            return 0;

        return map_iter->second.size();
    }

    void clear()
    {
        if (!elements)
            return;

        try_detach();

        cannot_share = false;
        elements->clear();
        elements_by_key->clear();
    }

    k_iterator<K, V> k_begin() const
    {
        return k_iterator<K, V>(elements_by_key->begin());
    }

    k_iterator<K, V> k_end() const
    {
        return k_iterator<K, V>(elements_by_key->end());
    }

private:
    bool cannot_share = false;
    std::shared_ptr<list_t<K, V>> elements;
    std::shared_ptr<map_t<K, V>> elements_by_key;

    static std::shared_ptr<list_t<K, V>> empty_list() 
    {
        static const auto empty_l = std::make_shared<list_t<K, V>>();
        return empty_l;
    }
    static std::shared_ptr<map_t<K, V>> empty_map() 
    {
        static const auto empty_m = std::make_shared<map_t<K, V>>();
        return empty_m;
    }

    void swap(kvfifo &other) noexcept
    {
        elements.swap(other.elements);
        elements_by_key.swap(other.elements_by_key);
        std::swap(cannot_share, other.cannot_share);
    }

    // Calls detach() if current object shares data with any other kvfifo object
    void try_detach()
    {
        if (elements.use_count() <= 1)
            return;

        detach();
    }

    // Makes copy of stored data
    void detach()
    {
        auto tmp_elements(std::make_shared<list_t<K, V>>(*elements));
        auto tmp_elements_by_key(std::make_shared<map_t<K, V>>());

        for (auto list_iter = tmp_elements->begin(); list_iter != tmp_elements->end();
             ++list_iter)
        {
            auto map_iter = tmp_elements_by_key->find(list_iter->first);
            if (map_iter == tmp_elements_by_key->end())
                map_iter = tmp_elements_by_key->insert({list_iter->first, {}}).first;

            map_iter->second.push_back(list_iter);
        }

        elements.swap(tmp_elements);
        elements_by_key.swap(tmp_elements_by_key);
        cannot_share = false;
    }

    // Throws exception if std:move() was executed on *this
    void throw_exception_if_moved() const
    {
        if (!elements)
            throw std::invalid_argument("Cannot execute method on object that was moved from");
    }
};

#endif