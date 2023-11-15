// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include "config.h"
#include "renderer.h"
#include "layout.h"

void Layout::update(const Config& config, const TextBoxRenderer& renderer) {
    screenSizeX = float(renderer.viewportWidth());
    screenSizeY = float(renderer.viewportHeight());

    emptyTextSize = screenSizeY * float(config.emptyTextSize) * .001f;
}
