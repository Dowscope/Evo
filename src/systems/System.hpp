#pragma once

#include <string>

class System {
public:
    explicit System(std::string name);
    virtual ~System();

    System(const System&) = delete;
    System& operator=(const System&) = delete;

    virtual void init();

private:
    std::string name_;
};
