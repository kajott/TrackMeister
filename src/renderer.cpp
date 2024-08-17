// SPDX-FileCopyrightText: 2023-2024 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <new>
#include <algorithm>

#include <glad/glad.h>
#include "lodepng.h"

#include "util.h"

#include "renderer.h"
#include "font_data.h"

constexpr int BatchSize = 16384;  // must be 16384 or less

///////////////////////////////////////////////////////////////////////////////

unsigned TextBoxRenderer::loadTexture(const void* pngData, size_t pngSize, int channels, bool mipmap, TextureDimensions* dims) {
    LodePNGColorType pngFormat;  GLenum glIntFormat, glInFormat;
    switch (channels) {
        case 1: pngFormat = LCT_GREY;       glIntFormat = GL_R8;    glInFormat = GL_RED;  break;
        case 2: pngFormat = LCT_GREY_ALPHA; glIntFormat = GL_RG8;   glInFormat = GL_RG;   break;
        case 3: pngFormat = LCT_RGB;        glIntFormat = GL_RGB8;  glInFormat = GL_RGB;  break;
        case 4: pngFormat = LCT_RGBA;       glIntFormat = GL_RGBA8; glInFormat = GL_RGBA; break;
        default: return 0;
    }

    uint8_t *img = nullptr;
    unsigned width = 0, height = 0;
    if (lodepng_decode_memory(&img, &width, &height, static_cast<const unsigned char*>(pngData), pngSize, pngFormat, 8)
    || !img || !width || !height)
        { free((void*)img); return 0; }
    if (dims) { dims->width = int(width); dims->height = int(height); }

    unsigned texID = 0;
    glGenTextures(1, &texID);
    if (!texID) { return 0; }
    while (glGetError());
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    if (mipmap) { glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.0f); }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if ((width * channels) & 3) { glPixelStorei(GL_UNPACK_ALIGNMENT, 1); }
    glTexImage2D(GL_TEXTURE_2D, 0, glIntFormat, width, height, 0, glInFormat, GL_UNSIGNED_BYTE, static_cast<const void*>(img));
    if (mipmap) { glGenerateMipmap(GL_TEXTURE_2D); }
    glBindTexture(GL_TEXTURE_2D, 0);
    glFlush(); glFinish();
    free((void*)img);
    if (glGetError()) { glDeleteTextures(1, &texID); texID = 0; }
    return texID;
}

unsigned TextBoxRenderer::loadTexture(const char* filename, int channels, bool mipmap, TextureDimensions* dims) {
    if (!filename || !filename[0]) { return 0; }
    FILE *f = fopen(filename, "rb");
    if (!f) { return 0; }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    if (fsize > (64u << 20)) { fclose(f); return 0; }  // size sanity check: max. 64 MiB
    void *buf = malloc(fsize);
    if (!buf) { fclose(f); return 0; }
    fseek(f, 0, SEEK_SET);
    if (fread(buf, fsize, 1, f) != 1) { free(buf); fclose(f); return 0; }
    fclose(f);
    unsigned texID = loadTexture(buf, fsize, channels, mipmap, dims);
    free(buf);
    return texID;
}

void TextBoxRenderer::freeTexture(unsigned &texID) {
    if (!texID) { return; }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &texID);
    texID = 0;
}

///////////////////////////////////////////////////////////////////////////////

static const char* vsSrc =
     "#version 330"
"\n" "layout(location=0) in vec2 aPos;"
"\n" "layout(location=1) in vec2 aTC;         out vec2 vTC;"
"\n" "layout(location=2) in vec3 aSize;  flat out vec3 vSize;"
"\n" "layout(location=3) in vec2 aBR;    flat out vec2 vBR;"
"\n" "layout(location=4) in vec4 aColor;      out vec4 vColor;"
"\n" "layout(location=5) in uint aMode;  flat out uint vMode;"
"\n" "void main() {"
"\n" "    gl_Position = vec4(aPos, 0., 1.);"
"\n" "    vTC    = aTC;"
"\n" "    vSize  = aSize;"
"\n" "    vBR    = aBR;"
"\n" "    vColor = aColor;"
"\n" "    vMode  = aMode;"
"\n" "}"
"\n";

static const char* fsSrc =
     "#version 330"
"\n" "     in vec2 vTC;"
"\n" "flat in vec3 vSize;"
"\n" "flat in vec2 vBR;"
"\n" "     in vec4 vColor;"
"\n" "flat in uint vMode;"
"\n" "uniform sampler2D uTex;"
"\n" "uniform sampler2D uBitmap;"
"\n" "uniform float uInvAlphaGamma;"
"\n" "layout(location=0) out vec4 outColor;"
"\n" "float sampleMSDF(in vec2 tc) {"
"\n" "    vec3 s = texture(uTex, tc).rgb;"
"\n" "    float d = max(min(s.r, s.g), min(max(s.r, s.g), s.b)) - 0.5;"
"\n" "    return clamp(d / fwidth(d), -0.5, 0.5);"
"\n" "}"
"\n" "void main() {"
"\n" "    float d = 0.;"
"\n" "    if (vMode == 1u) {  // MSDF text mode"
"\n" "        d = 0.25 * (sampleMSDF(vTC + vSize.xy * vec2(-0.375, -0.125))"
"\n" "                 +  sampleMSDF(vTC + vSize.xy * vec2(+0.125, -0.375))"
"\n" "                 +  sampleMSDF(vTC + vSize.xy * vec2(-0.125, +0.375))"
"\n" "                 +  sampleMSDF(vTC + vSize.xy * vec2(+0.375, +0.125)));"
"\n" "    } else if (vMode == 0u) {  // box mode"
"\n" "        vec2 p = abs(vTC) - vSize.xy;"
"\n" "        d = (min(p.x, p.y) > (-vSize.z))"
"\n" "          ? (vSize.z - length(p + vec2(vSize.z)))"
"\n" "          : min(-p.x, -p.y);"
"\n" "    } else if (vMode == 3u) {  // bitmap text mode"
"\n" "        d = texture(uBitmap, vTC).r;"
"\n" "    } else if (vMode == 2u) {  // logo mode"
"\n" "        d = texture(uTex, vTC).r;"
"\n" "    } else {  // normal texture mode"
"\n" "        outColor = texture(uTex, vTC);  return;"
"\n" "    }"
"\n" "    outColor = vec4(vColor.rgb, vColor.a * pow(clamp((d - vBR.x) * vBR.y + 0.5, 0.0, 1.0), uInvAlphaGamma));"
"\n" "}"
"\n";

namespace RenderMode {
    constexpr uint8_t Texture    = 4;
    constexpr uint8_t Logo       = 2;
    constexpr uint8_t Box        = 0;
    constexpr uint8_t MSDFText   = 1;
    constexpr uint8_t BitmapText = 3;
};

bool TextBoxRenderer::init() {
    GLint res;
    m_error = "unknown error";

    viewportChanged();

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, BatchSize * 4 * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
    m_vertices = nullptr;
    m_quadCount = 0;
    m_tex = 0;

    // GL_ARRAY_BUFFER is still bound
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer (0, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->pos[0]));
    glVertexAttribPointer (1, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->tc[0]));
    glVertexAttribPointer (2, 3, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->size[0]));
    glVertexAttribPointer (3, 2, GL_FLOAT,        GL_FALSE, sizeof(Vertex), &(static_cast<Vertex*>(0)->br[0]));
    glVertexAttribPointer (4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &(static_cast<Vertex*>(0)->color));
    glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT,           sizeof(Vertex), &(static_cast<Vertex*>(0)->mode));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    auto iboData = new(std::nothrow) uint16_t[BatchSize * 6];
    if (!iboData) { return false; }
    auto iboPtr = iboData;
    for (int base = 0;  base < (BatchSize * 4);  base += 4) {
        *iboPtr++ = uint16_t(base + 0);
        *iboPtr++ = uint16_t(base + 2);
        *iboPtr++ = uint16_t(base + 1);
        *iboPtr++ = uint16_t(base + 1);
        *iboPtr++ = uint16_t(base + 2);
        *iboPtr++ = uint16_t(base + 3);
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BatchSize * 6 * sizeof(uint16_t), static_cast<const void*>(iboData), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glFlush(); glFinish();
    delete[] iboData;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &res);
    if (res != GL_TRUE) {
        m_error = "Vertex Shader compilation failed";
        #ifndef NDEBUG
            ::puts(vsSrc);
            printf("%s.\n", m_error);
            glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetShaderInfoLog(vs, res, nullptr, msg);
                puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &res);
    if (res != GL_TRUE) {
        m_error = "Fragment Shader compilation failed";
        #ifndef NDEBUG
            ::puts(fsSrc);
            printf("%s.\n", m_error);
            glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetShaderInfoLog(fs, res, nullptr, msg);
                ::puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }

    m_prog = glCreateProgram();
    glAttachShader(m_prog, vs);
    glAttachShader(m_prog, fs);
    glLinkProgram(m_prog);
    glGetProgramiv(m_prog, GL_LINK_STATUS, &res);
    if (res != GL_TRUE) {
        m_error = "Shader Program linking failed";
        #ifndef NDEBUG
            printf("%s.\n", m_error);
            glGetProgramiv(m_prog, GL_INFO_LOG_LENGTH, &res);
            char* msg = new(std::nothrow) char[res];
            if (msg) {
                glGetProgramInfoLog(m_prog, res, nullptr, msg);
                ::puts(msg);
                delete[] msg;
            }
        #endif
        return false;
    }
    glDeleteShader(fs);
    glDeleteShader(vs);
    glUseProgram(m_prog);
    glUniform1i(glGetUniformLocation(m_prog, "uBitmap"), 1);
    m_locInvAlphaGamma = glGetUniformLocation(m_prog, "uInvAlphaGamma");
    glUniform1f(m_locInvAlphaGamma, 1.0f);

    m_fontTex = loadTexture(static_cast<const void*>(FontData::TexData), FontData::TexDataSize, 3, true);
    if (!m_fontTex) { m_error = "failed to load and decode the font texture"; return false; }

    glGenSamplers(1, &m_sampler);
    glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_fontTex);
    glBindSampler(1, m_sampler);
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_currentFont = &FontData::Fonts[0];
    m_error = "success";
    return true;
}

void TextBoxRenderer::setAlphaGamma(float gamma) {
    glUseProgram(m_prog);
    glUniform1f(m_locInvAlphaGamma, 1.0f / gamma);
}

void TextBoxRenderer::viewportChanged() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    m_vpWidth  = vp[2];
    m_vpHeight = vp[3];
    m_vpScaleX =  2.0f / float(m_vpWidth);
    m_vpScaleY = -2.0f / float(m_vpHeight);
}

void TextBoxRenderer::flush() {
    if (m_quadCount < 1) { return; }

    if (m_vertices) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_vertices = nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, m_tex);
    glBindVertexArray(m_vao);
    glUseProgram(m_prog);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glDrawElements(GL_TRIANGLES, m_quadCount * 6, GL_UNSIGNED_SHORT, nullptr);
    glFinish();
    m_quadCount = 0;
}

void TextBoxRenderer::shutdown() {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    freeTexture(m_fontTex);
    glBindSampler(1, 0);                       glDeleteSamplers(1, &m_sampler);
    glBindBuffer(GL_ARRAY_BUFFER, 0);          glDeleteBuffers(1, &m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  glDeleteBuffers(1, &m_ibo);
    glUseProgram(0);                           glDeleteProgram(m_prog);
    glBindVertexArray(0);                      glDeleteVertexArrays(1, &m_vao);
    glActiveTexture(GL_TEXTURE0);
}

///////////////////////////////////////////////////////////////////////////////

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices() {
    if (m_quadCount >= BatchSize) { flush(); }
    if (!m_vertices) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        m_vertices = (Vertex*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    return &m_vertices[4 * (m_quadCount++)];
}

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices(uint8_t mode, float x0, float y0, float x1, float y1) {
    x0 = x0 * m_vpScaleX - 1.0f;
    y0 = y0 * m_vpScaleY + 1.0f;
    x1 = x1 * m_vpScaleX - 1.0f;
    y1 = y1 * m_vpScaleY + 1.0f;
    Vertex* v = newVertices();
    v[0].pos[0] = x0;  v[0].pos[1] = y0;  v[0].mode = mode;
    v[1].pos[0] = x1;  v[1].pos[1] = y0;  v[1].mode = mode;
    v[2].pos[0] = x0;  v[2].pos[1] = y1;  v[2].mode = mode;
    v[3].pos[0] = x1;  v[3].pos[1] = y1;  v[3].mode = mode;
    return v;
}

TextBoxRenderer::Vertex* TextBoxRenderer::newVertices(uint8_t mode, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1) {
    Vertex* v = newVertices(mode, x0, y0, x1, y1);
    v[0].tc[0] = u0;  v[0].tc[1] = v0;  v[0].mode = mode;
    v[1].tc[0] = u1;  v[1].tc[1] = v0;  v[1].mode = mode;
    v[2].tc[0] = u0;  v[2].tc[1] = v1;  v[2].mode = mode;
    v[3].tc[0] = u1;  v[3].tc[1] = v1;  v[3].mode = mode;
    return v;
}

void TextBoxRenderer::box(int x0, int y0, int x1, int y1, uint32_t colorUpperLeft, uint32_t colorLowerRight, bool horizontalGradient, int borderRadius, float blur, float offset) {
    float w = 0.5f * (float(x1) - float(x0));
    float h = 0.5f * (float(y1) - float(y0));
    Vertex* v = newVertices(RenderMode::Box, float(x0), float(y0), float(x1), float(y1), -w, -h, w, h);
    v[0].color = colorUpperLeft;
    v[1].color = horizontalGradient ? colorLowerRight : colorUpperLeft;
    v[2].color = horizontalGradient ? colorUpperLeft : colorLowerRight;
    v[3].color = colorLowerRight;
    for (int i = 4;  i;  --i, ++v) {
        v->size[0] = w;  v->size[1] = h;
        v->size[2] = std::min(std::min(w, h), float(borderRadius));  // clamp border radius to half size
        v->br[0] = offset;
        v->br[1] = 1.0f / std::max(blur, 1.0f/256);
    }
}

void TextBoxRenderer::outlineBox(int x0, int y0, int x1, int y1, uint32_t colorUpper, uint32_t colorLower, uint32_t colorOutline, int outlineWidth, int borderRadius, int shadowOffset, float shadowBlur, float shadowAlpha, int shadowGrow) {
    int cOuter = std::max(0,  outlineWidth);
    int cInner = std::max(0, -outlineWidth);
    if ((shadowOffset || shadowGrow) && (shadowAlpha > 0.0f)) {
        uint32_t shadowColor = makeAlpha(shadowAlpha);
        box(x0 - cOuter + shadowOffset - shadowGrow,
            y0 - cOuter + shadowOffset - shadowGrow,
            x1 + cOuter + shadowOffset + shadowGrow,
            y1 + cOuter + shadowOffset + shadowGrow,
            shadowColor, shadowColor, false,
            borderRadius + cOuter + shadowGrow,
            shadowBlur + 1.0f, shadowBlur);
    }
    if (outlineWidth) {
        box(x0 - cOuter, y0 - cOuter, x1 + cOuter, y1 + cOuter,
            colorOutline | 0xFF000000u, colorOutline | 0xFF000000u, false, borderRadius + cOuter);
    }
    box(x0 + cInner, y0 + cInner, x1 - cInner, y1 - cInner,
        colorUpper | 0xFF000000u, colorLower | 0xFF000000u, false, borderRadius - cInner);
}

void TextBoxRenderer::texturedRect(uint8_t mode, int x0, int y0, int x1, int y1, uint32_t color, unsigned texID) {
    if (!texID || !(color & 0xFF000000u)) { return; }
    useTexture(texID);
    Vertex* v = newVertices(mode, float(x0), float(y0), float(x1), float(y1), 0.f, 0.f, 1.f, 1.f);
    v[0].color = v[1].color = v[2].color = v[3].color = color;
    v[0].br[0] = v[1].br[0] = v[2].br[0] = v[3].br[0] = 0.5f;
    v[0].br[1] = v[1].br[1] = v[2].br[1] = v[3].br[1] = -1.0f;
}

void TextBoxRenderer::logo(int x0, int y0, int x1, int y1, uint32_t color, unsigned texID) {
    texturedRect(RenderMode::Logo, x0, y0, x1, y1, color, texID);
}

void TextBoxRenderer::bitmap(int x0, int y0, int x1, int y1, unsigned texID) {
    texturedRect(RenderMode::Texture, x0, y0, x1, y1, uint32_t(-1), texID);
}

///////////////////////////////////////////////////////////////////////////////

const char* TextBoxRenderer::setFont(const char* name) {
    if (!name) { name = ""; }
    int matchLen = -1;
    for (const auto* font = FontData::Fonts;  font->name;  ++font) {
        int l = 0;
        while (name[l] && font->name[l] && (toLower(name[l]) == toLower(font->name[l]))) { ++l; }
        if (l > matchLen) {
            m_currentFont = font;
            matchLen = l;
        }
    }
    return m_currentFont->name;
}

const FontData::Glyph* TextBoxRenderer::getGlyph(uint32_t codepoint) const {
    if (!codepoint) { return nullptr; }

    // resolve fallback character
    if ((codepoint < 32u) || (codepoint == 0xFFFD)) { return &m_currentFont->glyphs[m_currentFont->fallbackIndex]; }

    // most characters are ASCII, and those are usually the first ones in the
    // glyph list anyway, so we may have a direct hit
    int quickIndex = codepoint - m_currentFont->glyphs[0].codepoint;
    if ((quickIndex < m_currentFont->numGlyphs)
    && (m_currentFont->glyphs[quickIndex].codepoint == codepoint)) {
        return &m_currentFont->glyphs[quickIndex];
    }

    // binary search in glyph list
    int foundIndex = -1;
    int a = 0, b = m_currentFont->numGlyphs;
    while (b > (a + 1)) {
        int c = (a + b) >> 1;
        if (m_currentFont->glyphs[c].codepoint == codepoint) { foundIndex = c; break; }
        if (m_currentFont->glyphs[c].codepoint > codepoint) { b = c; } else { a = c; }
    }
    if (m_currentFont->glyphs[a].codepoint == codepoint) { foundIndex = a; }
    return &m_currentFont->glyphs[(foundIndex < 0) ? m_currentFont->fallbackIndex : foundIndex];
}

uint32_t TextBoxRenderer::nextCodepoint(const char* &utf8string) {
    if (!utf8string || !utf8string[0]) { return 0u; }
    uint32_t cp = uint8_t(*utf8string++);
    if      (cp < 0x80) { return cp; }      // 7-bit ASCII byte
    else if (cp < 0xC0) { return 0xFFFD; }  // unexpected continuation byte
    int ecb = 0;  // ECB = "expected continuation bytes"
    if      (cp < 0xE0) { ecb = 1;  cp &= 0x1F; }
    else if (cp < 0xF0) { ecb = 2;  cp &= 0x0F; }
    else if (cp < 0xF8) { ecb = 3;  cp &= 0x07; }
    else { return 0xFFFD; }  // invalid UTF-8 sequence
    while (ecb--) {
        uint8_t byte = uint8_t(*utf8string);
        if ((byte & 0xC0) != 0x80) { return 0xFFFD; }  // truncated UTF-8 sequence; keep the offending byte
        ++utf8string;  // *now* consume the byte
        cp = (cp << 6) | byte;
    }
    return cp;
}

int   TextBoxRenderer::textSizeGranularity() const { return m_currentFont->bitmapHeight; }
float TextBoxRenderer::textBaseline()        const { return m_currentFont->baseline; }
float TextBoxRenderer::textNumberHeight()    const { return m_currentFont->numberHeight; }

float TextBoxRenderer::textWidth(const char* text) const {
    float w = 0.0f;
    const FontData::Glyph* g;
    while ((g = getGlyph(nextCodepoint(text))) != 0u) { w += g->advance; }
    return w;
}

void TextBoxRenderer::alignText(float &x, float &y, float size, const char* text, uint8_t align) {
    switch (align & Align::HMask) {
        case Align::Center:   x -= size * textWidth(text) * 0.5f; break;
        case Align::Right:    x -= size * textWidth(text);        break;
        default: break;
    }
    switch (align & Align::VMask) {
        case Align::Middle:   y -= size * 0.5f;                    break;
        case Align::Bottom:   y -= size;                           break;
        case Align::Baseline: y -= size * m_currentFont->baseline; break;
        default: break;
    }
    if (m_currentFont->bitmapHeight) {
        x = std::round(x);
        y = std::round(y);
    }
}

float TextBoxRenderer::text(float x, float y, float size, const char* text, uint8_t align, uint32_t colorUpper, uint32_t colorLower, float blur, float offset) {
    useTexture(m_fontTex);
    alignText(x, y, size, text, align);
    const FontData::Glyph* g;
    bool msdf = !m_currentFont->bitmapHeight;
    while ((g = getGlyph(nextCodepoint(text))) != 0u) {
        if (!g->space) {
            float aaSizeFactor = std::min(1.0f, 10.0f / size) / size;
            Vertex* v = newVertices(msdf ? RenderMode::MSDFText : RenderMode::BitmapText,
                            x + g->pos.x0 * size, y + g->pos.y0 * size,
                            x + g->pos.x1 * size, y + g->pos.y1 * size);
            v[0].color = v[1].color = colorUpper;
            v[2].color = v[3].color = colorLower;
            v[0].br[0] = v[1].br[0] = v[2].br[0] = v[3].br[0] = msdf ? offset        : 0.5f;
            v[0].br[1] = v[1].br[1] = v[2].br[1] = v[3].br[1] = msdf ? (1.0f / blur) : 1.0f;
            v[0].size[0] = v[1].size[0] = v[2].size[0] = v[3].size[0] = (g->tc.x1 - g->tc.x0) / (g->pos.x1 - g->pos.x0) * aaSizeFactor;
            v[0].size[1] = v[1].size[1] = v[2].size[1] = v[3].size[1] = (g->tc.y1 - g->tc.y0) / (g->pos.y1 - g->pos.y0) * aaSizeFactor;
            v[0].tc[0] = g->tc.x0;  v[0].tc[1] = g->tc.y0;
            v[1].tc[0] = g->tc.x1;  v[1].tc[1] = g->tc.y0;
            v[2].tc[0] = g->tc.x0;  v[2].tc[1] = g->tc.y1;
            v[3].tc[0] = g->tc.x1;  v[3].tc[1] = g->tc.y1;
        }
        x += g->advance * size;
    }
    return x;
}

float TextBoxRenderer::outlineText(float x, float y, float size, const char* text, uint8_t align, uint32_t colorUpper, uint32_t colorLower, uint32_t colorOutline, float outlineWidth, int shadowOffset, float shadowBlur, float shadowAlpha, float shadowGrow) {
    alignText(x, y, size, text, align);
    if ((shadowOffset || (shadowGrow >= 0.0f)) && (shadowAlpha > 0.0f)) {
        uint32_t shadowColor = makeAlpha(shadowAlpha);
        this->text(x + float(shadowOffset), y + float(shadowOffset), size, text, 0, shadowColor, shadowColor, shadowBlur + 1.0f, -shadowGrow);
    }
    if (outlineWidth >= 0.0f) {
        this->text(x, y, size, text, 0, colorOutline, colorOutline, 1.0f, -outlineWidth);
    }
    return this->text(x, y, size, text, 0, colorUpper, colorLower);
}

///////////////////////////////////////////////////////////////////////////////

int TextBoxRenderer::control(int x, int y, int size, uint8_t vAlign, bool keyboard, const char* control, const char* label, uint32_t textColor, uint32_t backgroundColor) {
    switch (vAlign & Align::VMask) {
        case Align::Middle:   y -= size >> 1;  break;
        case Align::Bottom:   y -= size;  break;
        case Align::Baseline: y -= int(float(size) * m_currentFont->baseline + 0.5f); break;
        default: break;
    }
    if (keyboard) {
        int border = std::max(1, size >> 3);
        int cheight = size - 2 * border;
        if (cheight <= 0) { return x; }
        float cwidth = float(cheight) * textWidth(control);
        int w = int(std::ceil(cwidth)) + 4 * border;
        box(x, y, x + w, y + size, textColor, textColor, false, 2 * border);
        box(x + border, y + border, x + w - border, y + size - border, backgroundColor, backgroundColor, false, border);
        text((float(2 * x + w) - cwidth) * 0.5f, float(y + border), float(cheight), control, 0, textColor);
        x += w;
    } else {
        const char *check = control;
        nextCodepoint(check);  // ignore first glyph, select font size based on whether another is following
        float cheight = float(size) * (nextCodepoint(check) ? 0.707f : 1.0f);
        float cwidth = cheight * textWidth(control);
        int w = std::max(size, int(std::ceil(cwidth + float(size) - cheight)));
        box(x, y, x + w, y + size, textColor, textColor, false, size);
        text((float(2 * x + w) - cwidth) * 0.5f, (float(2 * y + size) - cheight) * 0.5f, float(cheight), control, 0, backgroundColor);
        x += w;
    }
    x += size / 3;
    if (label) {
        x = int(std::ceil(text(float(x), float(y), float(size), label, 0, textColor))) + size;
    }
    return x;
}
