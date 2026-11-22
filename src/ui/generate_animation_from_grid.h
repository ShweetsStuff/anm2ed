#pragma once

#include "canvas.h"
#include "document.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

  class GenerateAnimationFromGrid : public Canvas
  {
    float time = 0;

  public:
    bool isEnd = false;

    GenerateAnimationFromGrid();
    void update(Manager&, Document&, Resources&, Settings&);
  };
