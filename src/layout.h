// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

struct Config;
class TextBoxRenderer;

struct Layout {
    float screenSizeX, screenSizeY;

    float emptyTextSize;

    void update(const Config& config, const TextBoxRenderer& renderer);
};
