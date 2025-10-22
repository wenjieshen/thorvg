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

#include "tvgGlShaderCache.h"
#include "tvgCompressor.h"
#include "tvgEnv.h"

#ifdef _WIN32
    #ifndef PATH_MAX
        #define PATH_MAX MAX_PATH
    #endif
#else
    #include <limits.h>
#endif


bool GlShaderCache::path(const char* vertSrc, const char* fragSrc, char* outPath, size_t outPathSize)
{
#if !defined(THORVG_FILE_IO_SUPPORT) || defined(__EMSCRIPTEN__)
    return false;
#endif
    // Compute hash from shader sources
    auto vertHash = djb2Encode(vertSrc);
    auto fragHash = djb2Encode(fragSrc);
    auto combinedHash = vertHash ^ (fragHash << 1);

    // Get system cache directory
    char cacheDir[PATH_MAX];
    if (!tvg::cachedir(cacheDir, PATH_MAX)) return false;

#ifdef _WIN32
    snprintf(outPath, outPathSize, "%s\\shader_%08lx.bin", cacheDir, combinedHash);
#else
    auto needed = snprintf(outPath, outPathSize, "%s/shader_%08lx.bin", cacheDir, combinedHash);
    if (needed < 0) return false;
    if (needed >= static_cast<int>(outPathSize)) return false;
#endif

    return true;
}


uint32_t GlShaderCache::read(const char* vertSrc, const char* fragSrc)
{

#if !defined(THORVG_FILE_IO_SUPPORT) || defined(__EMSCRIPTEN__)
    return 0;
#endif

    if (!vertSrc || !fragSrc) return 0;

    if (!glProgramBinarySupport()) return 0;

    // Read from cache if exists
    char cachePath[PATH_MAX];
    if (!path(vertSrc, fragSrc, cachePath, PATH_MAX)) return 0;

    auto file = fopen(cachePath, "rb");
    if (!file) return 0;

    GLenum binaryFormat = 0;
    GLsizei length = 0;

    if (fread(&binaryFormat, sizeof(GLenum), 1, file) != 1 || fread(&length, sizeof(GLsizei), 1, file) != 1 || length < 1) {
        TVGLOG("GL_ENGINE", "Failed to read shader cache header: %s", cachePath);
        fclose(file);
        return 0;
    }

    auto binaryData = tvg::malloc<GLubyte*>(length);
    auto bytesRead = fread(binaryData, 1, length, file);
    fclose(file);

    if (bytesRead != static_cast<size_t>(length)) {
        TVGLOG("GL_ENGINE", "Failed to read shader cache data: %s", cachePath);
        tvg::free(binaryData);
        return 0;
    }

    // Load program binary to shader
    uint32_t progObj = glCreateProgram();
    if (!progObj) {
        tvg::free(binaryData);
        return 0;
    }

    glProgramBinary(progObj, binaryFormat, binaryData, length);
    tvg::free(binaryData);

    GLint linked = 0;
    glGetProgramiv(progObj, GL_LINK_STATUS, &linked);

    if (!linked) {
        glDeleteProgram(progObj);
        return 0;
    }

    TVGLOG("GL_ENGINE", "Shader cache loaded: %s (%d bytes)", cachePath, length);
    return progObj;
}

void GlShaderCache::write(uint32_t progObj, const char* vertSrc, const char* fragSrc)
{

#if !defined(THORVG_FILE_IO_SUPPORT) || defined(__EMSCRIPTEN__)
    return;
#endif

    if (!progObj || !vertSrc || !fragSrc) return;

    if (!glProgramBinarySupport()) return;

    char cachePath[PATH_MAX];
    if (!path(vertSrc, fragSrc, cachePath, PATH_MAX)) return;

    GLint binaryLength = 0;
    glGetProgramiv(progObj, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    if (binaryLength < 1) {
        TVGLOG("GL_ENGINE", "Failed to get program binary length");
        return;
    }

    auto binaryData = tvg::malloc<GLubyte*>(binaryLength);

    GLenum binaryFormat = 0;
    GLsizei length = 0;
    glGetProgramBinary(progObj, binaryLength, &length, &binaryFormat, binaryData);

    if (length < 1) {
        TVGLOG("GL_ENGINE", "Failed to retrieve program binary");
        tvg::free(binaryData);
        return;
    }

    // Check if cache file can be written
    auto* file = fopen(cachePath, "wb");
    if (!file) {
        TVGLOG("GL_ENGINE", "Failed to open cache file for writing: %s", cachePath);
        tvg::free(binaryData);
        return;
    }

    fwrite(&binaryFormat, sizeof(GLenum), 1, file);
    fwrite(&length, sizeof(GLsizei), 1, file);

    auto written = fwrite(binaryData, 1, length, file);
    fclose(file);

    if (written == static_cast<size_t>(length)) TVGLOG("GL_ENGINE", "Shader cache written: %s (%d bytes)", cachePath, length);
    else TVGLOG("GL_ENGINE", "Failed to write shader cache: %s", cachePath);

    tvg::free(binaryData);
}
