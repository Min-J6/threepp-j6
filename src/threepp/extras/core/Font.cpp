
#include "threepp/extras/core/Font.hpp"

#include <iostream>

#include "threepp/extras/core/ShapePath.hpp"
#include "threepp/utils/StringUtils.hpp"

using namespace threepp;

namespace {

    struct FontPath {

        float offsetX{};
        ShapePath path;
    };

    FontPath createPath(char32_t c, float scale, float offsetX, float offsetY, const Font& data) {

        const auto glyph = data.glyphs.contains(c) ? data.glyphs.at(c) : data.glyphs.at('?');

        ShapePath path;

        float x, y, cpx, cpy, cpx1, cpy1, cpx2, cpy2;

        if (!glyph.o.empty()) {

            const auto outline = glyph.o;

            for (unsigned i = 0, l = outline.size(); i < l;) {

                const auto& action = outline[i++];

                if (action == "m") {// moveTo
                    x = utils::parseFloat(outline[i++]) * scale + offsetX;
                    y = utils::parseFloat(outline[i++]) * scale + offsetY;

                    path.moveTo(x, y);
                } else if (action == "l") {// lineTo

                    x = utils::parseFloat(outline[i++]) * scale + offsetX;
                    y = utils::parseFloat(outline[i++]) * scale + offsetY;

                    path.lineTo(x, y);

                } else if (action == "q") {// quadraticCurveTo

                    cpx = utils::parseFloat(outline[i++]) * scale + offsetX;
                    cpy = utils::parseFloat(outline[i++]) * scale + offsetY;
                    cpx1 = utils::parseFloat(outline[i++]) * scale + offsetX;
                    cpy1 = utils::parseFloat(outline[i++]) * scale + offsetY;

                    path.quadraticCurveTo(cpx1, cpy1, cpx, cpy);

                } else if (action == "b") {// bezierCurveTo

                    cpx = utils::parseFloat(outline[i++]) * scale + offsetX;
                    cpy = utils::parseFloat(outline[i++]) * scale + offsetY;
                    cpx1 = utils::parseFloat(outline[i++]) * scale + offsetX;
                    cpy1 = utils::parseFloat(outline[i++]) * scale + offsetY;
                    cpx2 = utils::parseFloat(outline[i++]) * scale + offsetX;
                    cpy2 = utils::parseFloat(outline[i++]) * scale + offsetY;

                    path.bezierCurveTo(cpx1, cpy1, cpx2, cpy2, cpx, cpy);
                }
            }
        }

        return FontPath{static_cast<float>(glyph.ha) * scale, path};
    }

    std::vector<ShapePath> createPaths(const std::string& text, float size, const Font& data) {
        const auto scale = size / static_cast<float>(data.resolution);
        const auto line_height = (data.boundingBox.yMax - data.boundingBox.yMin + static_cast<float>(data.underlineThickness)) * scale;

        std::vector<ShapePath> paths;
        float offsetX = 0, offsetY = 0;

        // UTF-8 문자열을 순회
        for (size_t i = 0; i < text.length();) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            char32_t codepoint;

            if (c < 0x80) {  // ASCII
                codepoint = c;
                i += 1;
            } else if (c < 0xE0) {  // 2-byte UTF-8
                codepoint = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F);
                i += 2;
            } else if (c < 0xF0) {  // 3-byte UTF-8 (한글 포함)
                codepoint = ((c & 0x0F) << 12) |
                           ((text[i + 1] & 0x3F) << 6) |
                           (text[i + 2] & 0x3F);
                i += 3;
            } else {  // 4-byte UTF-8
                codepoint = ((c & 0x07) << 18) |
                           ((text[i + 1] & 0x3F) << 12) |
                           ((text[i + 2] & 0x3F) << 6) |
                           (text[i + 3] & 0x3F);
                i += 4;
            }

            if (codepoint == '\n') {
                offsetX = 0;
                offsetY -= line_height;
            } else {
                const auto ret = createPath(codepoint, scale, offsetX, offsetY, data);
                offsetX += ret.offsetX;
                paths.emplace_back(ret.path);
            }
        }

        return paths;
    }

}// namespace

std::vector<Shape> Font::generateShapes(const std::string& text, float size) const {

    std::vector<Shape> shapes;
    const auto paths = createPaths(text, size, *this);

    for (const auto& path : paths) {

        const auto pathShapes = path.toShapes();
        shapes.insert(shapes.end(), pathShapes.begin(), pathShapes.end());
    }

    return shapes;
}
