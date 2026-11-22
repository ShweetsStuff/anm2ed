#pragma once

#include "canvas.h"
#include "document.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

  class GenerateAnimationFromGrid : public Canvas
  {
    float time{};

  public:
    bool isEnd{};

    GenerateAnimationFromGrid();
    void update(Manager&, Document&, Resources&, Settings&);
  };
