#pragma once

#include "ecs/Entity.hpp"

#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class Registry final {
public:
    [[nodiscard]] Entity create() {
        return _nextEntity++;
    }

    template <typename Component, typename... Arguments>
    Component& emplace(Entity entity, Arguments&&... arguments) {
        return _pool<Component>().emplace(
            entity,
            Component{std::forward<Arguments>(arguments)...}
        );
    }

    template <typename Component>
    [[nodiscard]] Component& get(Entity entity) {
        return _pool<Component>().get(entity);
    }

    template <typename Component>
    [[nodiscard]] const Component& get(Entity entity) const {
        return _pool<Component>().get(entity);
    }

private:
    class PoolBase {
    public:
        virtual ~PoolBase() = default;
    };

    template <typename Component>
    class Pool final : public PoolBase {
    public:
        Component& emplace(Entity entity, Component component) {
            if (_positions.contains(entity)) {
                throw std::runtime_error("Entity already has this component");
            }
            _positions.emplace(entity, _components.size());
            _entities.push_back(entity);
            _components.push_back(std::move(component));
            return _components.back();
        }

        Component& get(Entity entity) {
            return _components.at(_positions.at(entity));
        }

        const Component& get(Entity entity) const {
            return _components.at(_positions.at(entity));
        }

    private:
        std::vector<Entity> _entities;
        std::vector<Component> _components;
        std::unordered_map<Entity, std::size_t> _positions;
    };

    template <typename Component>
    Pool<Component>& _pool() const {
        const std::type_index type = typeid(Component);
        const auto existing = _pools.find(type);
        if (existing != _pools.end()) {
            return *static_cast<Pool<Component>*>(existing->second.get());
        }
        auto pool = std::make_unique<Pool<Component>>();
        Pool<Component>* result = pool.get();
        _pools.emplace(type, std::move(pool));
        return *result;
    }

    Entity _nextEntity = 1;
    mutable std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> _pools;
};
