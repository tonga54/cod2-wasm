#!/usr/bin/env python3
"""Fix texture calls and indexed client arrays in Emscripten 3.1.64.

Apply only to this target's linked JS, leaving the SDK and its cache untouched.
The SDK's duplicate-load regex stops at the texture-matrix subexpression's
closing parenthesis, producing invalid GLSL. Keep CSE, with balanced calls.
"""

import argparse
from pathlib import Path


ORIGINAL = r'''      var texLoadLines = "";
      var texLoadRegex = /(texture.*?\(.*?\))/g;
      var loadCounter = 0;
      var load;
      // As an optimization, merge duplicate identical texture loads to one var.
      while (load = texLoadRegex.exec(lines)) {
        var texLoadExpr = load[1];
        var secondOccurrence = lines.slice(load.index + 1).indexOf(texLoadExpr);
        if (secondOccurrence != -1) {
          // And also has a second occurrence of same load expression..
          // Create new var to store the common load.
          var prefix = TEXENVJIT_NAMESPACE_PREFIX + "env" + texUnitID + "_";
          var texLoadVar = prefix + "texload" + loadCounter++;
          var texLoadLine = "vec4 " + texLoadVar + " = " + texLoadExpr + ";\n";
          texLoadLines += texLoadLine + "\n";
          // Store the generated texture load statements in a temp string to not confuse regex search in progress.
          lines = lines.split(texLoadExpr).join(texLoadVar);
          // Reset regex search, since we modified the string.
          texLoadRegex = /(texture.*\(.*\))/g;
        }
      }
      return [ texLoadLines + lines ];'''

REPLACEMENT = r'''      // COD2_BALANCED_TEXTURE_LOADS: local Emscripten 3.1.64 compatibility fix.
      var texLoadLines = "";
      var texLoadRegex = /\btexture[A-Za-z0-9_]*\s*\(/g;
      var loadCounter = 0;
      var load;
      while (load = texLoadRegex.exec(lines)) {
        var end = texLoadRegex.lastIndex;
        var depth = 1;
        while (end < lines.length && depth) {
          var character = lines.charAt(end++);
          if (character === "(") ++depth;
          else if (character === ")") --depth;
        }
        if (depth) throw new Error("Unbalanced generated GLSL texture call");
        var texLoadExpr = lines.slice(load.index, end);
        texLoadRegex.lastIndex = end;
        if (lines.indexOf(texLoadExpr, end) !== -1) {
          var prefix = TEXENVJIT_NAMESPACE_PREFIX + "env" + texUnitID + "_";
          var texLoadVar = prefix + "texload" + loadCounter++;
          texLoadLines += "vec4 " + texLoadVar + " = " + texLoadExpr + ";\n";
          lines = lines.split(texLoadExpr).join(texLoadVar);
          texLoadRegex.lastIndex = 0;
        }
      }
      return [ texLoadLines + lines ];'''

DRAW_ORIGINAL = '''  // We can only emulate buffers of this kind, for now
  out("DrawElements doesn't actually prepareClientAttributes properly.");
  GLImmediate.prepareClientAttributes(count, false);
  GLImmediate.mode = mode;
  if (!GLctx.currentArrayBufferBinding) {
    GLImmediate.firstVertex = end ? start : HEAP8.length;
    // if we don't know the start, set an invalid value and we will calculate it later from the indices
    GLImmediate.lastVertex = end ? end + 1 : 0;
    start = GLImmediate.vertexPointer;
    // TODO(sbc): Combine these two subarray calls back into a single one if
    // we ever fix https://github.com/emscripten-core/emscripten/issues/21250.
    if (end) {
      end = GLImmediate.vertexPointer + (end + 1) * GLImmediate.stride;
      GLImmediate.vertexData = HEAPF32.subarray(((start) >>> 2) >>> 0, ((end) >>> 2) >>> 0);
    } else {
      GLImmediate.vertexData = HEAPF32.subarray(((start) >>> 2) >>> 0);
    }
  }'''

DRAW_REPLACEMENT = '''  // COD2_INDEXED_CLIENT_RANGE: copy vertices referenced by the indices.
  // Index count is not vertex count (e.g. one triangle may reference vertex 255).
  // Establish the range before restriding separate color/UV/position arrays.
  if (!count) return;
  var vertexCount = count;
  if (!GLctx.currentArrayBufferBinding) {
    assert(!GLctx.currentElementArrayBufferBinding);
    if (end === undefined) {
      start = 65535;
      end = 0;
      for (var index = 0; index < count; ++index) {
        var vertex = HEAPU16[(indices >>> 1) + index];
        start = Math.min(start, vertex);
        end = Math.max(end, vertex);
      }
    }
    GLImmediate.firstVertex = start;
    GLImmediate.lastVertex = end + 1;
    vertexCount = end + 1;
  }
  GLImmediate.prepareClientAttributes(vertexCount, false);
  GLImmediate.mode = mode;
  if (!GLctx.currentArrayBufferBinding) {
    var vertexStart = GLImmediate.vertexPointer;
    var vertexEnd = vertexStart + vertexCount * GLImmediate.stride;
    GLImmediate.vertexData = HEAPF32.subarray(vertexStart >>> 2, vertexEnd >>> 2);
  }'''

CUBE_SAMPLE_ORIGINAL = '''      return `${func}(${TEX_UNIT_UNIFORM_PREFIX}${texUnitID}, ${texCoordExpr}.xy)`;'''
CUBE_SAMPLE_REPLACEMENT = '''      // COD2_CUBE_TEXTURE_COORDS: cube sampling requires a three-dimensional direction.
      var components = texType == GL_TEXTURE_CUBE_MAP ? "xyz" : "xy";
      return `${func}(${TEX_UNIT_UNIFORM_PREFIX}${texUnitID}, ${texCoordExpr}.${components})`;'''

CUBE_UNIFORM_ORIGINAL = r'''            texUnitUniformList += "uniform sampler2D " + uTexUnitPrefix + texUnit + ";\n";'''
CUBE_UNIFORM_REPLACEMENT = r'''            // COD2_CUBE_TEXTURE_SAMPLER: match the sampler to the enabled texture target.
            var samplerType = GLImmediate.TexEnvJIT.getTexUnitType(texUnit) == 0x8513 ? "samplerCube" : "sampler2D";
            texUnitUniformList += "uniform " + samplerType + " " + uTexUnitPrefix + texUnit + ";\n";'''

LIGHT_COLOR_ORIGINAL = '''            vsLightingPass += "  v_color = clamp(v_color, 0.0, 1.0);";'''
LIGHT_COLOR_REPLACEMENT = '''            // COD2_LIT_VERTEX_COLOR: DX7 uses COLOR1 for ambient and diffuse.
            // Its material is white, with zero emission/specular. Preserve
            // the original per-vertex tint/alpha before clamping the light sum.
            vsLightingPass += "  v_color *= a_color;";
            vsLightingPass += "  v_color = clamp(v_color, 0.0, 1.0);";'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("javascript", type=Path)
    path = parser.parse_args().javascript
    source = path.read_text()
    for original, replacement in ((ORIGINAL, REPLACEMENT), (DRAW_ORIGINAL, DRAW_REPLACEMENT),
                                  (CUBE_SAMPLE_ORIGINAL, CUBE_SAMPLE_REPLACEMENT),
                                  (CUBE_UNIFORM_ORIGINAL, CUBE_UNIFORM_REPLACEMENT),
                                  (LIGHT_COLOR_ORIGINAL, LIGHT_COLOR_REPLACEMENT)):
        if source.count(replacement) == 1:
            continue
        if source.count(original) != 1:
            raise SystemExit("Unexpected Emscripten GL generator; refusing to patch")
        source = source.replace(original, replacement, 1)
    path.write_text(source)
    print("Fixed Emscripten GL texture calls, cube samplers, model colors and indexed client-array ranges")


if __name__ == "__main__":
    main()
