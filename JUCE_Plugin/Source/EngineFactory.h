#pragma once

#include <memory>
#include "EngineBase.h"
#include "EngineTypes.h"

class EngineFactory {
public:
    static std::unique_ptr<EngineBase> createEngine(int engineID);
};