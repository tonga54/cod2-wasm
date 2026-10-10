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

INDEX_BIND_ORIGINAL = '''        var indexBuffer = GL.getTempIndexBuffer(numProvidedIndexes << 1);
        GLctx.bindBuffer(GLctx.ELEMENT_ARRAY_BUFFER, indexBuffer);'''
INDEX_BIND_REPLACEMENT = '''        var indexBuffer = GL.getTempIndexBuffer(numProvidedIndexes << 1);
        GLctx.bindBuffer(GLctx.ELEMENT_ARRAY_BUFFER, indexBuffer);
        // COD2_STREAM_INDEX_STORAGE: the SDK reuses one IBO per size, even
        // while earlier draws still use it. Orphan its storage before writing
        // to avoid a CPU wait for the GPU. Keep the SDK's power-of-two capacity.
        GLctx.bufferData(GLctx.ELEMENT_ARRAY_BUFFER,
          1 << GL.log2ceilLookup(numProvidedIndexes << 1), GLctx.STREAM_DRAW);'''


POINT_DEFS_ORIGINAL = '          var vsSource = ['
POINT_DEFS_REPLACEMENT = r'''          // COD2_POINT_LIGHT_SHADER: compile once, including frames without flashes.
          // Uniform count zero skips lighting; firing never creates a shader variant.
          var pointLights = this.usedTexUnitList.indexOf(0) !== -1 &&
            GLImmediate.TexEnvJIT.getTexUnitType(0) == 0x0DE1;
          var pointNormal = pointLights && GLEmulation.lightingEnabled;
          var pointVarying = pointLights ? "varying highp vec3 v_cod2Eye;" : "";
          if (pointNormal) pointVarying += "varying mediump vec3 v_cod2Normal;";
          var pointDefs = pointLights ? pointVarying +
            "uniform int u_cod2LightCount; uniform highp vec4 u_cod2LightEye[4]; uniform vec4 u_cod2LightColor[4];" : "";
          var pointPass = "";
          if (pointLights) {
            var pointUV = GLImmediate.useTextureMatrix ? "(u_textureMatrix0 * v_texCoord0).xy" : "v_texCoord0.xy";
            pointPass = "if (u_cod2LightCount > 0) {\n" +
              (pointNormal ? "highp vec3 n = v_cod2Normal;\n" :
                "highp vec3 n = cross(dFdx(v_cod2Eye), dFdy(v_cod2Eye));\n") +
              "n *= inversesqrt(max(dot(n,n), 0.00000001));\nvec3 lightSum = vec3(0.0);\n";
            for (var light = 0; light < 4; ++light) {
              pointPass += "if (u_cod2LightCount > " + light + ") {\n" +
                "highp vec3 delta = u_cod2LightEye[" + light + "].xyz - v_cod2Eye;\n" +
                "highp float distance2 = dot(delta,delta);\n" +
                "float attenuation = max(1.0 - distance2 * u_cod2LightEye[" + light + "].w, 0.0);\n" +
                "float incidence = abs(dot(n,delta)) * inversesqrt(max(distance2, 0.0001));\n" +
                "lightSum += u_cod2LightColor[" + light + "].rgb * attenuation * attenuation * incidence;\n}\n";
            }
            pointPass += "gl_FragColor.rgb += texture2D(u_texUnit0, " + pointUV + ").rgb * lightSum;\n}\n";
          }
          var vsSource = ['''
POINT_VERTEX_ORIGINAL = 'vsPointSizeDefs, vsClipPlaneDefs, vsLightingDefs, "void main()"'
POINT_VERTEX_REPLACEMENT = 'vsPointSizeDefs, vsClipPlaneDefs, vsLightingDefs, pointVarying, "void main()"'
POINT_POSITION_ORIGINAL = '"  gl_Position = u_projection * ecPosition;", "  v_color = a_color;"'
POINT_POSITION_REPLACEMENT = '"  gl_Position = u_projection * ecPosition;", (pointLights ? "v_cod2Eye = ecPosition.xyz;" : ""), (pointNormal ? "v_cod2Normal = u_normalMatrix * a_normal;" : ""), "  v_color = a_color;"'
POINT_FRAGMENT_ORIGINAL = 'fogHeaderIfNeeded, fsClipPlaneDefs, fsAlphaTestDefs, "void main()"'
POINT_FRAGMENT_REPLACEMENT = 'fogHeaderIfNeeded, fsClipPlaneDefs, fsAlphaTestDefs, pointDefs, "void main()"'
POINT_PASS_ORIGINAL = 'fsClipPlanePass, fsTexEnvPass, fogPass, fsAlphaTestPass'
POINT_PASS_REPLACEMENT = 'fsClipPlanePass, fsTexEnvPass, pointPass, fogPass, fsAlphaTestPass'
POINT_UNIFORMS_ORIGINAL = '        this.normalMatrixLocation = GLctx.getUniformLocation(this.program, "u_normalMatrix");'
POINT_UNIFORMS_REPLACEMENT = r'''        this.normalMatrixLocation = GLctx.getUniformLocation(this.program, "u_normalMatrix");
        this.cod2LightCountLocation = GLctx.getUniformLocation(this.program, "u_cod2LightCount");
        this.cod2LightEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2LightEye[0]");
        this.cod2LightColorLocation = GLctx.getUniformLocation(this.program, "u_cod2LightColor[0]");
        this.cod2LightCount = -1;
        this.cod2LightRevision = -1;'''
POINT_UPLOAD_ORIGINAL = '        if (GLImmediate.mode == GLctx.POINTS) {\n          if (this.pointSizeLocation) {'
POINT_UPLOAD_REPLACEMENT = r'''        // COD2_POINT_LIGHT_UNIFORMS: cache per program; no allocation per draw.
        if (this.cod2LightCountLocation !== null) {
          var lights = Module.cod2PointLights;
          var count = lights ? lights.drawCount : 0;
          if (this.cod2LightCount !== count) {
            GLctx.uniform1i(this.cod2LightCountLocation, count);
            this.cod2LightCount = count;
          }
          if (count && this.cod2LightRevision !== lights.eyeRevision) {
            GLctx.uniform4fv(this.cod2LightEyeLocation, lights.eye);
            GLctx.uniform4fv(this.cod2LightColorLocation, lights.color);
            this.cod2LightRevision = lights.eyeRevision;
          }
        }
        if (GLImmediate.mode == GLctx.POINTS) {
          if (this.pointSizeLocation) {'''

SHADER_CHECK_ORIGINAL = '          GLctx.linkProgram(this.program);'
SHADER_CHECK_REPLACEMENT = r'''          GLctx.linkProgram(this.program);
          // COD2_SHADER_DIAGNOSTICS: fail visibly instead of drawing a black frame.
          if (!GLctx.getProgramParameter(this.program, GLctx.LINK_STATUS)) {
            throw new Error("Fixed-function shader failed: " + GLctx.getProgramInfoLog(this.program) +
              "\nVertex: " + GLctx.getShaderInfoLog(this.vertexShader) +
              "\nFragment: " + GLctx.getShaderInfoLog(this.fragmentShader));
          }'''

POINT_GLSL_ORIGINAL = '          this.vertexShader = GLctx.createShader(GLctx.VERTEX_SHADER);'
POINT_GLSL_REPLACEMENT = r'''          // WebGL2 implements derivatives in GLSL ES 3.00, not its ES 1.00
          // compatibility shaders. Keep the generated texture/fog/alpha logic.
          function cod2Glsl300(source, vertex) {
            source = source.replace(/\battribute\b/g, "in");
            source = source.replace(/\bvarying (?!(?:lowp|mediump|highp)\b)/g,
              vertex ? "out mediump " : "in mediump ");
            source = source.replace(/\bvarying\b/g, vertex ? "out" : "in");
            source = source.replace(/\btexture(?:2D|Cube)\b/g, "texture");
            if (!vertex) source = "out highp vec4 cod2FragColor;\n" + source.replace(/\bgl_FragColor\b/g, "cod2FragColor");
            return "#version 300 es\n" + source;
          }
          this.vertexShader = GLctx.createShader(GLctx.VERTEX_SHADER);'''
POINT_VS_UPLOAD_ORIGINAL = '          GLctx.shaderSource(this.vertexShader, vsSource);'
POINT_VS_UPLOAD_REPLACEMENT = '          GLctx.shaderSource(this.vertexShader, pointLights ? cod2Glsl300(vsSource, true) : vsSource);'
POINT_FS_UPLOAD_ORIGINAL = '          GLctx.shaderSource(this.fragmentShader, fsSource);'
POINT_FS_UPLOAD_REPLACEMENT = '          GLctx.shaderSource(this.fragmentShader, pointLights ? cod2Glsl300(fsSource, false) : fsSource);'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("javascript", type=Path)
    path = parser.parse_args().javascript
    source = path.read_text()
    for original, replacement in ((ORIGINAL, REPLACEMENT), (DRAW_ORIGINAL, DRAW_REPLACEMENT),
                                  (CUBE_SAMPLE_ORIGINAL, CUBE_SAMPLE_REPLACEMENT),
                                  (CUBE_UNIFORM_ORIGINAL, CUBE_UNIFORM_REPLACEMENT),
                                  (LIGHT_COLOR_ORIGINAL, LIGHT_COLOR_REPLACEMENT),
                                  (INDEX_BIND_ORIGINAL, INDEX_BIND_REPLACEMENT),
                                  (POINT_DEFS_ORIGINAL, POINT_DEFS_REPLACEMENT),
                                  (POINT_VERTEX_ORIGINAL, POINT_VERTEX_REPLACEMENT),
                                  (POINT_POSITION_ORIGINAL, POINT_POSITION_REPLACEMENT),
                                  (POINT_FRAGMENT_ORIGINAL, POINT_FRAGMENT_REPLACEMENT),
                                  (POINT_PASS_ORIGINAL, POINT_PASS_REPLACEMENT),
                                  (POINT_UNIFORMS_ORIGINAL, POINT_UNIFORMS_REPLACEMENT),
                                  (POINT_UPLOAD_ORIGINAL, POINT_UPLOAD_REPLACEMENT),
                                  (SHADER_CHECK_ORIGINAL, SHADER_CHECK_REPLACEMENT),
                                  (POINT_GLSL_ORIGINAL, POINT_GLSL_REPLACEMENT),
                                  (POINT_VS_UPLOAD_ORIGINAL, POINT_VS_UPLOAD_REPLACEMENT),
                                  (POINT_FS_UPLOAD_ORIGINAL, POINT_FS_UPLOAD_REPLACEMENT)):
        if source.count(replacement) == 1:
            continue
        if source.count(original) != 1:
            raise SystemExit("Unexpected Emscripten GL generator; refusing to patch")
        source = source.replace(original, replacement, 1)
    path.write_text(source)
    print("Fixed Emscripten GL texture calls, cube samplers, model colors, point lights and indexed client-array ranges")


if __name__ == "__main__":
    main()
