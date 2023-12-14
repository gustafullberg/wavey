#ifndef LABEL_H_
#define LABEL_H_

#include <optional>
#include <string>

struct Label {
    float start;
    std::optional<float> stop;
    std::optional<std::string> label;
};

#endif
