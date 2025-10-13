#pragma once
#include "EngineBase.h"
#include <memory>

namespace EngineFactory {
    std::unique_ptr<EngineBase> createEngine(int engineID);
}
