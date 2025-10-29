/*
 * Copyright (c) 2025 the ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_GL_SHADER_CACHE_H_
#define _TVG_GL_SHADER_CACHE_H_

#include "tvgGlCommon.h"
#include <unordered_map>
#include <vector>

struct ShaderBinaryData {
    GLenum binaryFormat;
    GLsizei length;
    GLubyte* data;
};

struct GlShaderCache
{
    static void write(uint32_t progObj, const char* vertSrc, const char* fragSrc);
    static uint32_t read(const char* vertSrc, const char* fragSrc);
    static bool path(const char* vertSrc, const char* fragSrc, char* outPath, size_t outPathSize);

    std::unordered_map<unsigned long, ShaderBinaryData> cache;
    bool loaded = false;
    bool dirty = false;

    bool load();
    bool flush();
    bool fetchEntry(unsigned long entryHash, GLenum* binaryFormat, GLsizei* length, GLubyte** binaryData);
    bool writeEntry(unsigned long entryHash, GLenum binaryFormat, GLsizei length, const GLubyte* binaryData);
    void clear();
};

#endif /* _TVG_GL_SHADER_CACHE_H_ */

