#ifndef LABEL_H_
#define LABEL_H_

#include <optional>
#include <string>

struct Label {
    float begin;
    std::optional<float> end;
    std::optional<std::string> label;
};

#endif
