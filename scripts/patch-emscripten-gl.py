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
LIGHT_COLOR_LEGACY = '''            // COD2_LIT_VERTEX_COLOR: DX7 uses COLOR1 for ambient and diffuse.
            // Its material is white, with zero emission/specular. Preserve
            // the original per-vertex tint/alpha before clamping the light sum.
            vsLightingPass += "  v_color *= a_color;";
            vsLightingPass += "  v_color = clamp(v_color, 0.0, 1.0);";'''

LIGHT_COLOR_REPLACEMENT = LIGHT_COLOR_LEGACY.replace("v_color *= a_color;", "v_color *= cod2VertexColor;")

INDEX_BIND_ORIGINAL = '''        var indexBuffer = GL.getTempIndexBuffer(numProvidedIndexes << 1);
        GLctx.bindBuffer(GLctx.ELEMENT_ARRAY_BUFFER, indexBuffer);'''
INDEX_BIND_REPLACEMENT = '''        var indexBuffer = GL.getTempIndexBuffer(numProvidedIndexes << 1);
        GLctx.bindBuffer(GLctx.ELEMENT_ARRAY_BUFFER, indexBuffer);
        // COD2_STREAM_INDEX_STORAGE: the SDK reuses one IBO per size, even
        // while earlier draws still use it. Orphan its storage before writing
        // to avoid a CPU wait for the GPU. Keep the SDK's power-of-two capacity.
        GLctx.bufferData(GLctx.ELEMENT_ARRAY_BUFFER,
          1 << GL.log2ceilLookup(numProvidedIndexes << 1), GLctx.STREAM_DRAW);'''


POINT_DEFS_LEGACY = r'''          // COD2_POINT_LIGHT_SHADER: compile once, including frames without flashes.
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
            // Reuse the material's base texel for illumination. This removes
            // the second texture lookup on flashes, including matrix UVs.
            var pointSample = "texture2D(u_texUnit0, " + pointUV + ")";
            var pointAlbedo = pointSample;
            if (fsTexEnvPass.indexOf(pointSample) !== -1) {
              fsTexEnvPass = "vec4 cod2BaseTexel = " + pointSample + ";\n" +
                fsTexEnvPass.split(pointSample).join("cod2BaseTexel");
              pointAlbedo = "cod2BaseTexel";
            }
            pointPass = "if (u_cod2LightCount > 0) {\n" +
              (pointNormal ? "highp vec3 n = v_cod2Normal;\n" :
                "highp vec3 n = cross(dFdx(v_cod2Eye), dFdy(v_cod2Eye));\n") +
              "n *= inversesqrt(max(dot(n,n), 0.00000001));\nvec3 lightSum = vec3(0.0);\n";
            for (var light = 0; light < 4; ++light) {
              pointPass += "if (u_cod2LightCount > " + light + ") {\n" +
                "highp vec3 delta = u_cod2LightEye[" + light + "].xyz - v_cod2Eye;\n" +
                "highp float distance2 = dot(delta,delta);\n" +
                "float attenuation = max(1.0 - distance2 * u_cod2LightEye[" + light + "].w, 0.0);\n" +
                // Models have authored outward normals; flashes must not
                // light their back side. Derivative world normals lack a
                // guaranteed orientation, so retain two-sided world lighting.
                "float incidence = " + (pointNormal ? "max(dot(n,delta), 0.0)" : "abs(dot(n,delta))") +
                " * inversesqrt(max(distance2, 0.0001));\n" +
                "lightSum += u_cod2LightColor[" + light + "].rgb * attenuation * attenuation * incidence;\n}\n";
            }
            pointPass += "gl_FragColor.rgb += " + pointAlbedo + ".rgb * lightSum;\n}\n";
          }
          var vsSource = ['''
POINT_DEFS_ORIGINAL = '          var vsSource = ['
POINT_DEFS_REPLACEMENT = r'''          // COD2_POINT_LIGHT_SHADER: compile once, including frames without flashes.
          // Uniform count zero skips lighting; firing never creates a shader variant.
          var pointLights = this.usedTexUnitList.indexOf(0) !== -1 &&
            GLImmediate.TexEnvJIT.getTexUnitType(0) == 0x0DE1;
          var pointNormal = pointLights && GLEmulation.lightingEnabled;
          var pointVarying = pointLights ? "varying highp vec3 v_cod2Eye;" : "";
          if (pointNormal) pointVarying += "varying mediump vec3 v_cod2Normal;";
          var pointDefs = pointLights ? pointVarying +
            "uniform int u_cod2LightCount; uniform highp vec4 u_cod2LightEye[4]; uniform vec4 u_cod2LightColor[4];" +
            (pointNormal ? "" : "uniform int u_cod2ShadowCount; uniform highp vec4 u_cod2ShadowEye[4];") : "";
          var pointPass = "";
          // COD2_EFFECTS: the feather and heat of effects (web_effects.c),
          // after the material's light and before its fog.
          var cod2EffectPass = "";
          if (pointLights) {
            var pointUV = GLImmediate.useTextureMatrix ? "(u_textureMatrix0 * v_texCoord0).xy" : "v_texCoord0.xy";
            // Reuse the material's base texel for illumination. This removes
            // the second texture lookup on flashes, including matrix UVs.
            var pointSample = "texture2D(u_texUnit0, " + pointUV + ")";
            var pointAlbedo = pointSample;
            var hasBaseTexel = fsTexEnvPass.indexOf(pointSample) !== -1;
            // COD2_WEAPON_CAMO: a camouflaged gun's pattern and paint mask,
            // bound by the device on units 16-17. Lit models paint the base
            // texel before its lighting; a uniform flag turns it on per draw.
            var camoPaint = pointNormal && hasBaseTexel;
            if (hasBaseTexel) {
              // Preserve the authored texture footprint. A positive mip bias
              // erases weapon detail; it cannot reproduce COD4's resolved-scene
              // depth of field, which filters silhouettes and lighting too.
              // COD2_TEXTURE_LOD_BIAS: a scene rendered below the canvas's
              // lines samples a finer mip by the scale's octaves (u_cod2Detail.w,
              // web_scene_fx.c), so its color maps keep the canvas's detail.
              fsTexEnvPass = "vec4 cod2BaseTexel = texture2D(u_texUnit0, " + pointUV + ", u_cod2Detail.w);\n" +
                (camoPaint ? "float cod2Paint = 0.0;\nif (u_cod2Camo.x > 0.5) cod2BaseTexel.rgb = cod2Camouflage(" +
                  "cod2BaseTexel.rgb, " + pointUV + ", cod2Paint);\n" : "") +
                fsTexEnvPass.split(pointSample).join("cod2BaseTexel");
              pointAlbedo = "cod2BaseTexel";
            }
            if (camoPaint)
              pointDefs += "uniform sampler2D u_cod2CamoMap; uniform sampler2D u_cod2CamoMask; uniform vec4 u_cod2Camo;\n" +
                // The pattern where the mask paints, shaded by the texel against
                // the material's painted median: its grain, edges and wear.
                "vec3 cod2Camouflage(vec3 base, highp vec2 uv, out float paint) {\n" +
                "  paint = texture2D(u_cod2CamoMask, uv).r;\n" +
                "  vec3 camo = texture2D(u_cod2CamoMap, uv * u_cod2Camo.y).rgb;\n" +
                "  float wear = clamp(dot(base, vec3(0.299, 0.587, 0.114)) / u_cod2Camo.z, 0.5, 1.5);\n" +
                "  return mix(base, camo * (1.0 + 0.4 * (wear - 1.0)), paint);\n}\n";
            pointPass = "if (u_cod2LightCount > 0) {\n" +
              (pointNormal ? "highp vec3 n = v_cod2Normal;\n" :
                "highp vec3 n = cross(dFdx(v_cod2Eye), dFdy(v_cod2Eye));\n") +
              "n *= inversesqrt(max(dot(n,n), 0.00000001));\nvec3 lightSum = vec3(0.0);\n";
            for (var light = 0; light < 4; ++light) {
              pointPass += "if (u_cod2LightCount > " + light + ") {\n" +
                "highp vec3 delta = u_cod2LightEye[" + light + "].xyz - v_cod2Eye;\n" +
                "highp float distance2 = dot(delta,delta);\n" +
                "float attenuation = max(1.0 - distance2 * u_cod2LightEye[" + light + "].w, 0.0);\n" +
                // Models have authored outward normals; flashes must not
                // light their back side. Derivative world normals lack a
                // guaranteed orientation, so retain two-sided world lighting.
                "float incidence = " + (pointNormal ? "max(dot(n,delta), 0.0)" : "abs(dot(n,delta))") +
                " * inversesqrt(max(distance2, 0.0001));\n" +
                "lightSum += u_cod2LightColor[" + light + "].rgb * attenuation * attenuation * incidence;\n}\n";
            }
            pointPass += "gl_FragColor.rgb += " + pointAlbedo + ".rgb * lightSum;\n}\n";
            if (!pointNormal) {
              pointPass += "if (u_cod2ShadowCount > 0) {\nfloat cod2Shadow = 1.0;\n";
              for (var shadow = 0; shadow < 4; ++shadow) {
                pointPass += "if (u_cod2ShadowCount > " + shadow + ") {\n" +
                  "highp vec3 sDelta = v_cod2Eye - u_cod2ShadowEye[" + shadow + "].xyz;\n" +
                  "float sAtten = max(1.0 - dot(sDelta, sDelta) * u_cod2ShadowEye[" + shadow + "].w, 0.0);\n" +
                  "cod2Shadow *= 1.0 - 0.42 * sAtten * sAtten;\n}\n";
              }
              pointPass += "gl_FragColor.rgb *= cod2Shadow;\n}\n";
            }
            // COD2_MATERIAL_LIGHTING: the original normal, specular and detail
            // maps, bound by the device on units 5-7. Uniform flags select them
            // per draw, so materials never add shader variants.
            var materialLightmap = !pointNormal && this.usedTexUnitList.indexOf(1) !== -1 &&
              GLImmediate.TexEnvJIT.getTexUnitType(1) == 0x0DE1;
            // Derivatives of fp16 UVs are noise on GPUs that honor mediump.
            texUnitVaryingList = texUnitVaryingList.replace(/varying vec4 v_texCoord([01]);/g,
              "varying highp vec4 v_texCoord$1;");
            // Effects: multisample coverage, the signed feather scale (negative
            // fades the color), the heat and the near clip the depth needs.
            pointDefs += "uniform sampler2D u_cod2DetailMap; uniform sampler2D u_cod2NormalMap; uniform sampler2D u_cod2SpecMap;\n" +
              "uniform vec4 u_cod2Surface; uniform vec4 u_cod2Detail; uniform vec4 u_cod2Model; uniform highp vec4 u_cod2Effect;\n" +
              // x: how much a lightmapped surface's folds shade it (r_cavity).
              "uniform vec4 u_cod2Relief;\n" +
              // The sky's direction in eye space; w is 1 while surfaces with a
              // specular map mirror their surroundings (r_reflections).
              "uniform highp vec3 u_cod2SunEye; uniform highp vec4 u_cod2SkyEye; uniform vec4 u_cod2SunColor;\n" +
              // Tangent frame from screen derivatives: DX7 vertices have no tangents.
              // The map decodes as CoD2's DX9 shaders do, N + x T + y B, where the
              // BSP's tangents follow +u and +v. Relief flattens as the map
              // minifies: a pixel averaging several texels of it would otherwise
              // sparkle in the sun.
              "highp vec3 cod2Bump(highp vec3 n, highp vec2 uv, float strength) {\n" +
              "  highp vec2 du1 = dFdx(uv), du2 = dFdy(uv);\n" +
              "  highp vec2 size = vec2(textureSize(u_cod2NormalMap, 0));\n" +
              "  highp float texels = sqrt(max(dot(du1 * size, du1 * size), dot(du2 * size, du2 * size)));\n" +
              "  vec2 t = (texture2D(u_cod2NormalMap, uv).ag * 2.0 - 1.0) * (strength * clamp(1.5 - 0.5 * texels, 0.15, 1.0));\n" +
              "  highp vec3 dp1 = dFdx(v_cod2Eye), dp2 = dFdy(v_cod2Eye);\n" +
              "  highp vec3 a = cross(dp2, n), b = cross(n, dp1);\n" +
              "  highp vec3 tu = a * du1.x + b * du2.x, tv = a * du1.y + b * du2.y;\n" +
              "  highp float s = inversesqrt(max(max(dot(tu, tu), dot(tv, tv)), 1e-30));\n" +
              "  highp vec3 r = mat3(tu * s, tv * s, n) * vec3(t, 1.0);\n" +
              "  return r * inversesqrt(max(dot(r, r), 1e-30));\n}\n" +
              // CoD2's specularity table: alpha picks one of 32 exponents. The
              // narrowest cosines round to 1 at mediump.
              "highp float cod2SpecPower(float a) {\n" +
              "  highp float p = floor(a * 31.0 + 0.5);\n" +
              "  if (p > 30.5) return 30.0;\n" +
              "  highp float c = min(cos((180.0 - 175.0 * p / 30.0) * 0.0043633231), 0.9999999);\n" +
              "  return min(-2.3025851 / log(c), 2048.0);\n}\n" +
              // Indirect light comes mostly from the sky: a normal map shades it
              // relative to the surface's own normal.
              "float cod2Sky(highp vec3 n) { return 1.0 + 0.5 * dot(n, u_cod2SkyEye.xyz); }\n" +
              // COD2_RELIEF_CAVITY: where the map leans far from the surface,
              // at the edges of stones, bricks and planks and in the grooves
              // between them, less of that light reaches it. The sun already
              // models relief where it shines, so only the shade takes it.
              "float cod2Cavity(highp vec3 b, highp vec3 n, float sunlit) {\n" +
              "  return 1.0 - 0.5 * (1.0 - max(dot(b, n), 0.0)) * (1.0 - clamp(sunlit, 0.0, 1.0));\n}\n" +
              // COD2_RELIEF_CREASE: the map's slopes a pixel's footprint further
              // along +u and +v. Where they converge the surface folds in, a joint
              // between stones or a groove between planks; where they part it
              // folds out, an edge. The fold is per world unit, so a map at half
              // resolution folds as one at full does, and it fades out as pixels
              // grow past a unit and a half of the world, 0.75 to 2.25.
              "float cod2Crease(highp vec2 uv) {\n" +
              "  highp vec2 du1 = dFdx(uv), du2 = dFdy(uv);\n" +
              "  highp vec2 size = vec2(textureSize(u_cod2NormalMap, 0));\n" +
              "  highp vec2 t1 = du1 * size, t2 = du2 * size;\n" +
              "  highp float span = max(sqrt(max(dot(t1, t1), dot(t2, t2))), 1.0);\n" +
              "  highp vec3 dp1 = dFdx(v_cod2Eye), dp2 = dFdy(v_cod2Eye);\n" +
              "  highp float units = 0.5 * span * (sqrt(dot(dp1, dp1) / max(dot(t1, t1), 1e-12)) +\n" +
              "    sqrt(dot(dp2, dp2) / max(dot(t2, t2), 1e-12)));\n" +
              "  vec2 m = texture2D(u_cod2NormalMap, uv).ag;\n" +
              "  vec2 mu = texture2D(u_cod2NormalMap, uv + vec2(span / size.x, 0.0)).ag;\n" +
              "  vec2 mv = texture2D(u_cod2NormalMap, uv + vec2(0.0, span / size.y)).ag;\n" +
              "  return 2.0 * (mu.x - m.x + mv.y - m.y) / max(units, 1e-6) *\n" +
              "    (1.0 - smoothstep(0.75, 2.25, 0.5 * (length(dp1) + length(dp2))));\n}\n" +
              "float cod2Phong(highp vec3 n, highp vec3 l, highp float power) {\n" +
              "  highp vec3 v = -v_cod2Eye * inversesqrt(max(dot(v_cod2Eye, v_cod2Eye), 1e-30));\n" +
              "  highp float h = max(dot(n, normalize(l + v)), 0.0);\n" +
              "  return pow(h, power) * step(0.0001, dot(n, l));\n}\n";
            var materialPass = "if (u_cod2Detail.z > 0.5)\n" +
              "  gl_FragColor.rgb *= texture2D(u_cod2DetailMap, " + pointUV + " * u_cod2Detail.xy).rgb * 2.0;\n";
            if (pointNormal) {
              var materialLights = [];
              for (var materialLight = 0; materialLight < GLEmulation.MAX_LIGHTS; ++materialLight)
                if (GLEmulation.lightEnabled[materialLight]) materialLights.push(materialLight);
              pointDefs += "uniform highp vec4 u_lightModelAmbient; uniform highp vec4 u_materialAmbient; uniform highp vec4 u_materialDiffuse;\n";
              for (var lightIndex = 0; lightIndex < materialLights.length; ++lightIndex) {
                var id = materialLights[lightIndex];
                pointDefs += "uniform highp vec4 u_lightAmbient" + id + "; uniform highp vec4 u_lightDiffuse" + id +
                  "; uniform highp vec4 u_lightPosition" + id + ";\n";
              }
              materialPass += "if (u_cod2Surface.x > 0.5 || u_cod2Surface.y > 0.5 || u_cod2Model.x > 0.0 || u_cod2Model.y > 0.5) {\n" +
                "highp vec3 cod2N = v_cod2Normal;\n" +
                "cod2N *= inversesqrt(max(dot(cod2N, cod2N), 1e-30));\n" +
                "highp vec3 cod2B = cod2N;\n" +
                "if (u_cod2Surface.x > 0.5) cod2B = cod2Bump(cod2N, " + pointUV + ", 1.0);\n" +
                "vec4 cod2S = u_cod2Surface.y > 0.5 ? texture2D(u_cod2SpecMap, " + pointUV + ") : vec4(0.0);\n" +
                // Paint is duller than the bare steel or varnish under it.
                (camoPaint ? "cod2S.rgb *= 1.0 - 0.5 * cod2Paint;\n" : "") +
                "highp float cod2P = cod2SpecPower(cod2S.a);\n" +
                // Relight the vertex color with the bumped normal: the ratio
                // keeps the original grid lights, clamp and tint.
                "vec3 cod2Ambient = u_lightModelAmbient.rgb, cod2Diffuse = vec3(0.0);\n" +
                "vec3 cod2Flat = vec3(0.0), cod2Bumped = vec3(0.0), cod2Spec = vec3(0.0);\n";
              for (var lightIndex = 0; lightIndex < materialLights.length; ++lightIndex) {
                var id = materialLights[lightIndex];
                materialPass += "{ highp vec3 l = normalize(u_lightPosition" + id + ".xyz);\n" +
                  "vec3 d = u_lightDiffuse" + id + ".rgb * u_materialDiffuse.rgb;\n" +
                  "cod2Ambient += u_lightAmbient" + id + ".rgb; cod2Diffuse += d;\n" +
                  "cod2Flat += d * max(dot(cod2N, l), 0.0); cod2Bumped += d * max(dot(cod2B, l), 0.0); }\n";
              }
              // The grid's strongest light leads, the sun comes last; DX7
              // colors are half scale for MODULATE2X.
              var specularLights = materialLights.length > 1 ?
                [materialLights[0], materialLights[materialLights.length - 1]] : materialLights;
              for (var lightIndex = 0; lightIndex < specularLights.length; ++lightIndex) {
                var id = specularLights[lightIndex];
                materialPass += "cod2Spec += u_lightDiffuse" + id + ".rgb * cod2Phong(cod2B, normalize(u_lightPosition" + id + ".xyz), cod2P);\n";
              }
              // Most models get only the grid's average: the engine folds the
              // sun into that ambient at full strength, whatever the angle.
              materialPass += "cod2Ambient *= u_materialAmbient.rgb;\n" +
                "vec3 cod2Sun = dot(cod2Diffuse, vec3(1.0)) < 0.0001 ? u_cod2SunColor.rgb * u_cod2Model.x * u_materialAmbient.rgb : vec3(0.0);\n" +
                "float cod2NLG = max(dot(cod2N, u_cod2SunEye), 0.0), cod2NLB = max(dot(cod2B, u_cod2SunEye), 0.0);\n" +
                "float cod2Incidence = mix(1.0 + cod2NLB - cod2NLG, cod2NLB, u_cod2SunColor.a);\n" +
                "cod2Flat += cod2Ambient;\n" +
                "cod2Bumped += (cod2Ambient - cod2Sun) * (cod2Sky(cod2B) / cod2Sky(cod2N) * cod2Cavity(cod2B, cod2N, u_cod2Model.x)) +\n" +
                "  cod2Sun * cod2Incidence;\n" +
                "gl_FragColor.rgb *= max(cod2Bumped, vec3(0.0)) / max(cod2Flat, vec3(0.004));\n" +
                // COD2_VIEWMODEL_LIGHT: the arms and weapon, held under the open
                // sky, take its light from above the left shoulder (eye space,
                // toward the light): faces turned up and toward it brighten and
                // faces turned down and away darken, so the weapon reads by its
                // form in flat shade too, and the specular map catches it.
                "if (u_cod2Model.y > 0.5) {\n" +
                "const highp vec3 cod2Shoulder = vec3(-0.454, 0.655, 0.605);\n" +
                "gl_FragColor.rgb *= mix(0.68, 1.28, smoothstep(-0.5, 0.9, dot(cod2B, cod2Shoulder)));\n" +
                "cod2Spec += cod2Flat * 0.6 * cod2Phong(cod2B, cod2Shoulder, cod2P);\n}\n" +
                "cod2Spec += cod2Sun * cod2Phong(cod2B, u_cod2SunEye, cod2P);\n" +
                "gl_FragColor.rgb += cod2S.rgb * cod2Spec * (2.0 * u_cod2Surface.w);\n" +
                // A broad reflection of the sky and ground, stronger at grazing
                // angles: what keeps metal reading as metal in the shade.
                "if (u_cod2Surface.y > 0.5 && u_cod2SkyEye.w > 0.0) {\n" +
                "highp vec3 cod2V = -v_cod2Eye * inversesqrt(max(dot(v_cod2Eye, v_cod2Eye), 1e-30));\n" +
                "float cod2Up = dot(reflect(-cod2V, cod2B), u_cod2SkyEye.xyz);\n" +
                "float cod2Fresnel = 0.2 + 0.8 * pow(1.0 - max(dot(cod2B, cod2V), 0.0), 5.0);\n" +
                "vec3 cod2Env = (cod2Ambient - cod2Sun) * 2.0 * mix(0.35, 1.0, smoothstep(-0.3, 0.7, cod2Up));\n" +
                "gl_FragColor.rgb += cod2S.rgb * cod2Env * (cod2Fresnel * clamp(cod2P / 48.0, 0.25, 1.0) * u_cod2Surface.w);\n}\n}\n";
              // COD2_SCOPE_GLASS: a scope's glass in the hands (web_effects.c
              // names its disc in the color map) mirrors the world drawn before
              // it, as a convex lens shows what faces it: upright, turned left
              // for right and drawn in toward its rim, through a cool coating
              // over dark glass, more at grazing angles and toward the rim; the
              // sun glints off its curve and its mount shades its edge. Placed by
              // where the disc lies on the screen, the world stays upright
              // however the glass was mapped. Derivatives come before the
              // branch, where they are defined.
              pointDefs += "uniform highp vec4 u_cod2Lens; uniform sampler2D u_cod2LensScene;\n";
              materialPass += "if (u_cod2Lens.z > 0.0) {\n" +
                "highp vec2 cod2LensUv = " + pointUV + ";\n" +
                "highp mat2 cod2LensJ = mat2(dFdx(cod2LensUv), dFdy(cod2LensUv));\n" +
                "float cod2LensR = length((cod2LensUv - u_cod2Lens.xy) / u_cod2Lens.zw);\n" +
                "if (cod2LensR < 1.0 && abs(determinant(cod2LensJ)) > 1e-14) {\n" +
                "highp mat2 cod2LensI = inverse(cod2LensJ);\n" +
                "highp vec2 cod2LensO = cod2LensI * (cod2LensUv - u_cod2Lens.xy);\n" +
                "highp float cod2LensPx = max(0.5 * (length(cod2LensI * vec2(u_cod2Lens.z, 0.0)) +\n" +
                "  length(cod2LensI * vec2(0.0, u_cod2Lens.w))), 1.0);\n" +
                "highp vec2 cod2LensM = vec2(-cod2LensO.x, cod2LensO.y) / cod2LensPx;\n" +
                "cod2LensM *= 0.42 + 0.16 * dot(cod2LensM, cod2LensM);\n" +
                "float cod2LensLod = log2(max(0.6 * float(textureSize(u_cod2LensScene, 0).x) / cod2LensPx, 1.0)) + 1.0;\n" +
                "vec3 cod2LensWorld = textureLod(u_cod2LensScene, clamp(vec2(0.5) + cod2LensM, 0.0, 1.0), cod2LensLod).rgb;\n" +
                "highp vec3 cod2LensV = -v_cod2Eye * inversesqrt(max(dot(v_cod2Eye, v_cod2Eye), 1e-30));\n" +
                "highp vec3 cod2LensN = v_cod2Normal * inversesqrt(max(dot(v_cod2Normal, v_cod2Normal), 1e-30));\n" +
                // The glass faces the eye whichever way the model wound it.
                "if (dot(cod2LensN, cod2LensV) < 0.0) cod2LensN = -cod2LensN;\n" +
                "float cod2LensFresnel = mix(0.3, 0.8, pow(1.0 - dot(cod2LensN, cod2LensV), 4.0)) *\n" +
                "  mix(0.85, 1.25, cod2LensR * cod2LensR);\n" +
                // The coating cools the reflection, and turns violet toward the rim.
                "vec3 cod2LensCoat = mix(vec3(0.66, 0.78, 0.95), vec3(0.85, 0.62, 1.0), smoothstep(0.55, 0.95, cod2LensR));\n" +
                "vec3 cod2LensColor = vec3(0.008, 0.010, 0.014) + cod2LensWorld * cod2LensCoat * cod2LensFresnel;\n" +
                // The sun where it reaches the weapon, and the light it is held
                // under, glint off the glass's curve.
                "highp vec3 cod2LensCurve = normalize(cod2LensN + 0.5 * vec3(cod2LensO / cod2LensPx, 0.0));\n" +
                "highp vec3 cod2LensR3 = reflect(-cod2LensV, cod2LensCurve);\n" +
                "cod2LensColor += u_cod2SunColor.rgb * (1.5 * u_cod2Model.x * pow(max(dot(cod2LensR3, u_cod2SunEye), 0.0), 80.0)) +\n" +
                "  vec3(0.12) * pow(max(dot(cod2LensR3, vec3(-0.454, 0.655, 0.605)), 0.0), 40.0);\n" +
                "cod2LensColor *= 1.0 - 0.5 * smoothstep(0.75, 1.0, cod2LensR);\n" +
                "gl_FragColor.rgb = mix(gl_FragColor.rgb, cod2LensColor, 1.0 - smoothstep(0.93, 1.0, cod2LensR));\n}\n}\n";
            } else if (materialLightmap) {
              var lightmapUV = GLImmediate.useTextureMatrix ? "(u_textureMatrix1 * v_texCoord1).xy" : "v_texCoord1.xy";
              var lightmapSample = "texture2D(u_texUnit1, " + lightmapUV + ")";
              var lightmapTexel = lightmapSample;
              if (fsTexEnvPass.indexOf(lightmapSample) !== -1) {
                fsTexEnvPass = "vec4 cod2LightmapTexel = " + lightmapSample + ";\n" +
                  fsTexEnvPass.split(lightmapSample).join("cod2LightmapTexel");
                lightmapTexel = "cod2LightmapTexel";
              }
              // COD2_SUN_SHADOW: moving models' shadow in the sun's depth map
              // (web_sun_shadows.c, unit 18). It takes away the sun's share of
              // the lightmap, so baked shade and interiors never darken. Four
              // filtered comparisons on a rotated grid soften its edge.
              pointDefs += "uniform highp mat4 u_cod2SunShadowEye; uniform highp vec4 u_cod2SunShadow; uniform highp vec4 u_cod2SunShadowRect;\n" +
                "uniform mediump sampler2DShadow u_cod2SunShadowMap;\n" +
                "float cod2SunShadow(float visible) {\n" +
                "  if (u_cod2SunShadow.x <= 0.0 || visible <= 0.004) return 1.0;\n" +
                "  highp vec3 p = (u_cod2SunShadowEye * vec4(v_cod2Eye, 1.0)).xyz;\n" +
                // Only the casters' own rectangle of the map can hold a shadow.
                "  if (any(lessThan(p.xy, u_cod2SunShadowRect.xy)) || any(greaterThan(p.xy, u_cod2SunShadowRect.zw))) return 1.0;\n" +
                "  highp vec2 edge = min(p.xy, 1.0 - p.xy);\n" +
                "  float fade = clamp(min(edge.x, edge.y) * u_cod2SunShadow.z, 0.0, 1.0);\n" +
                "  if (fade <= 0.0 || p.z >= 1.0) return 1.0;\n" +
                "  highp float d = p.z - u_cod2SunShadow.w, t = u_cod2SunShadow.y;\n" +
                "  float lit = textureLod(u_cod2SunShadowMap, vec3(p.xy + vec2(-0.5, -1.5) * t, d), 0.0) +\n" +
                "    textureLod(u_cod2SunShadowMap, vec3(p.xy + vec2(1.5, -0.5) * t, d), 0.0) +\n" +
                "    textureLod(u_cod2SunShadowMap, vec3(p.xy + vec2(0.5, 1.5) * t, d), 0.0) +\n" +
                "    textureLod(u_cod2SunShadowMap, vec3(p.xy + vec2(-1.5, 0.5) * t, d), 0.0);\n" +
                "  return 1.0 - u_cod2SunShadow.x * fade * (1.0 - 0.25 * lit);\n}\n" +
                // COD2_SURFACE_VARIATION: smooth value noise over the color map's
                // own coordinates, so it holds still on the surface and repeats
                // nowhere the map does.
                "highp float cod2Hash(highp vec2 p) {\n" +
                "  highp vec3 q = fract(vec3(p.xyx) * 0.1031);\n" +
                "  q += dot(q, q.yzx + 33.33);\n" +
                "  return fract((q.x + q.y) * q.z);\n}\n" +
                "float cod2Noise(highp vec2 p) {\n" +
                "  highp vec2 i = floor(p), f = fract(p);\n" +
                "  vec2 u = f * f * (3.0 - 2.0 * f);\n" +
                "  return mix(mix(cod2Hash(i), cod2Hash(i + vec2(1.0, 0.0)), u.x),\n" +
                "    mix(cod2Hash(i + vec2(0.0, 1.0)), cod2Hash(i + vec2(1.0, 1.0)), u.x), u.y);\n}\n" +
                // COD2_DETAIL_RELIEF: the detail map's grain read as a height; its
                // slope one texel along +u and +v tilts the lit normal. It flattens
                // as the map minifies, as the normal map's relief does.
                "highp vec3 cod2DetailBump(highp vec3 b, highp vec3 n, highp vec2 uv, float strength) {\n" +
                "  highp vec2 du1 = dFdx(uv), du2 = dFdy(uv);\n" +
                "  highp vec2 size = vec2(textureSize(u_cod2DetailMap, 0));\n" +
                "  highp float texels = sqrt(max(dot(du1 * size, du1 * size), dot(du2 * size, du2 * size)));\n" +
                "  float fade = clamp(1.5 - 0.5 * texels, 0.0, 1.0);\n" +
                "  if (fade <= 0.0) return b;\n" +
                "  float h = dot(texture2D(u_cod2DetailMap, uv).rgb, vec3(0.3333));\n" +
                "  float hu = dot(texture2D(u_cod2DetailMap, uv + vec2(1.0 / size.x, 0.0)).rgb, vec3(0.3333));\n" +
                "  float hv = dot(texture2D(u_cod2DetailMap, uv + vec2(0.0, 1.0 / size.y)).rgb, vec3(0.3333));\n" +
                "  highp vec3 dp1 = dFdx(v_cod2Eye), dp2 = dFdy(v_cod2Eye);\n" +
                "  highp vec3 a = cross(dp2, n), c = cross(n, dp1);\n" +
                "  highp vec3 tu = a * du1.x + c * du2.x, tv = a * du1.y + c * du2.y;\n" +
                "  highp float s = inversesqrt(max(max(dot(tu, tu), dot(tv, tv)), 1e-30));\n" +
                "  highp vec3 r = b - (strength * fade) * ((hu - h) * tu + (hv - h) * tv) * s;\n" +
                "  return r * inversesqrt(max(dot(r, r), 1e-30));\n}\n";
              // COD2_TEXTURE_SHARPEN: a magnified color map against itself a mip
              // and a half blurrier, so its stones, cracks and grain keep their
              // edges. Past two and a half texels a pixel it would only sparkle.
              var baseLine = "vec4 cod2BaseTexel = texture2D(u_texUnit0, " + pointUV + ", u_cod2Detail.w);\n";
              if (fsTexEnvPass.indexOf(baseLine) !== -1)
                fsTexEnvPass = fsTexEnvPass.replace(baseLine, baseLine +
                  "if (u_cod2Surface.z > 0.5 && u_cod2Relief.w > 0.0) {\n" +
                  "  highp vec2 cod2Texels = " + pointUV + " * vec2(textureSize(u_texUnit0, 0));\n" +
                  "  highp float cod2Span = sqrt(max(dot(dFdx(cod2Texels), dFdx(cod2Texels)), dot(dFdy(cod2Texels), dFdy(cod2Texels))));\n" +
                  "  float cod2Sharpen = 0.5 * u_cod2Relief.w * (1.0 - smoothstep(1.0, 2.5, cod2Span));\n" +
                  "  if (cod2Sharpen > 0.0) {\n" +
                  "    vec3 cod2Soft = textureLod(u_texUnit0, " + pointUV + ", max(log2(max(cod2Span, 1e-4)) + u_cod2Detail.w, 0.0) + 1.5).rgb;\n" +
                  "    cod2BaseTexel.rgb = max(cod2BaseTexel.rgb + cod2Sharpen * (cod2BaseTexel.rgb - cod2Soft), vec3(0.0));\n" +
                  "  }\n}\n");
              // COD2_TERRAIN_BLEND: a terrain layer blended by its vertices' alpha
              // takes over where its texture stands above its own average
              // brightness and gives way where it sinks below, most about the
              // middle of the blend, so layers meet along stones and tufts rather
              // than in a smear; its average opacity holds.
              var alphaLine = "float tej_env0_alpha = v_color.a * tej_env0_texload0.a;\n";
              if (fsTexEnvPass.indexOf(alphaLine) !== -1)
                fsTexEnvPass = fsTexEnvPass.replace(alphaLine, alphaLine +
                  "if (u_cod2Surface.z > 0.5 && u_cod2Relief.y > 0.0 && v_color.a < 0.995) {\n" +
                  "  float cod2Mean = dot(textureLod(u_texUnit0, " + pointUV + ", 12.0).rgb, vec3(0.299, 0.587, 0.114));\n" +
                  "  float cod2Height = dot(tej_env0_texload0.rgb, vec3(0.299, 0.587, 0.114)) / max(cod2Mean, 0.02) - 1.0;\n" +
                  "  float cod2Blend = clamp(v_color.a + 0.9 * u_cod2Relief.y * cod2Height * (1.0 - abs(2.0 * v_color.a - 1.0)), 0.0, 1.0);\n" +
                  "  tej_env0_alpha = mix(cod2Blend, smoothstep(0.0, 1.0, cod2Blend), 0.6 * min(u_cod2Relief.y, 1.0)) * tej_env0_texload0.a;\n}\n");
              // The browser stores the sun's visibility in the lightmap alpha;
              // the DX7 bake added the sun at full strength whatever the angle.
              materialPass += "if (u_cod2Surface.z > 0.5) {\n" +
                "highp vec3 cod2N = cross(dFdx(v_cod2Eye), dFdy(v_cod2Eye));\n" +
                "cod2N *= inversesqrt(max(dot(cod2N, cod2N), 1e-30));\n" +
                "if (dot(cod2N, v_cod2Eye) > 0.0) cod2N = -cod2N;\n" +
                // COD2_SURFACE_VARIATION: two octaves of noise a few repeats of the
                // map wide, set off its grid, vary the surface's tone, the ground
                // by up to 22% and walls by 8%, its paler patches drier and warmer
                // and its darker ones cooler about the map's own color, so a field
                // or a road no longer repeats one tile and a map keeps its palette.
                "if (u_cod2Relief.y > 0.0) {\n" +
                "highp vec2 cod2VarUv = " + pointUV + ";\n" +
                "float cod2Var = 0.65 * cod2Noise(cod2VarUv * 0.43 + vec2(17.0, 31.0)) +\n" +
                "  0.35 * cod2Noise(mat2(0.8, -0.6, 0.6, 0.8) * cod2VarUv * 1.37 + vec2(5.0, 9.0));\n" +
                "float cod2Floor = dot(cod2N, u_cod2SkyEye.xyz);\n" +
                "gl_FragColor.rgb *= 1.0 + u_cod2Relief.y * mix(0.08, 0.22, cod2Floor * cod2Floor) * (2.0 * cod2Var - 1.0);\n" +
                "gl_FragColor.rgb *= 1.0 + 0.6 * min(u_cod2Relief.y, 1.5) * (cod2Var - 0.5) * vec3(0.05, -0.01, -0.16);\n}\n" +
                "highp vec3 cod2B = cod2N;\n" +
                "if (u_cod2Surface.x > 0.5) cod2B = cod2Bump(cod2N, " + pointUV + ", 1.0);\n" +
                // COD2_DETAIL_RELIEF: the detail map's grain lifts the surface it
                // darkens and lightens, so the sun catches sand, stone and plaster.
                "if (u_cod2Detail.z > 0.5 && u_cod2Relief.z > 0.0) cod2B = cod2DetailBump(cod2B, cod2N, " + pointUV +
                  " * u_cod2Detail.xy, 6.0 * u_cod2Relief.z);\n" +
                "vec4 cod2S = u_cod2Surface.y > 0.5 ? texture2D(u_cod2SpecMap, " + pointUV + ") : vec4(0.0);\n" +
                "highp float cod2P = cod2SpecPower(cod2S.a);\n" +
                "vec4 cod2L = " + lightmapTexel + ";\n" +
                "vec3 cod2SunPart = u_cod2SunColor.rgb * cod2L.a;\n" +
                "float cod2NLG = max(dot(cod2N, u_cod2SunEye), 0.0), cod2NLB = max(dot(cod2B, u_cod2SunEye), 0.0);\n" +
                // Strength 1 is the DX9 incidence; 0 keeps the flat bake and
                // applies only the normal map's relief.
                "float cod2Incidence = mix(1.0 + cod2NLB - cod2NLG, cod2NLB, u_cod2SunColor.a);\n" +
                "vec3 cod2Indirect = max(cod2L.rgb - cod2SunPart, vec3(0.0));\n" +
                "cod2SunPart *= cod2SunShadow(cod2L.a);\n" +
                "vec3 cod2Lit = cod2Indirect * (cod2Sky(cod2B) / cod2Sky(cod2N) * cod2Cavity(cod2B, cod2N, cod2L.a)) +\n" +
                "  cod2SunPart * cod2Incidence;\n" +
                // COD2_RELIEF_CREASE: a joint or groove takes less of every light,
                // the sun's too, and an edge a little more; slight folds, the
                // stone's own grain, take none.
                "if (u_cod2Surface.x > 0.5 && u_cod2Relief.x > 0.0) {\n" +
                "float cod2Fold = cod2Crease(" + pointUV + ");\n" +
                "cod2Lit *= (1.0 - 0.22 * u_cod2Relief.x * smoothstep(0.2, 1.0, -cod2Fold)) *\n" +
                "  (1.0 + 0.06 * u_cod2Relief.x * smoothstep(0.2, 1.0, cod2Fold));\n}\n" +
                "gl_FragColor.rgb *= max(cod2Lit, vec3(0.0)) / max(cod2L.rgb, vec3(0.004));\n" +
                "gl_FragColor.rgb += cod2S.rgb * cod2SunPart * (2.0 * u_cod2Surface.w * cod2Phong(cod2B, u_cod2SunEye, cod2P));\n" +
                // COD2_WORLD_REFLECTION: the light around the surface, the
                // lightmap's indirect share, mirrored as models mirror their
                // ambient: brighter toward the sky, more at grazing angles.
                // Rough maps keep little of that gain, their facets hiding
                // one another, so stone stays matte and polish shines.
                "if (u_cod2Surface.y > 0.5 && u_cod2SkyEye.w > 0.0) {\n" +
                "highp vec3 cod2V = -v_cod2Eye * inversesqrt(max(dot(v_cod2Eye, v_cod2Eye), 1e-30));\n" +
                "float cod2Gloss = clamp(cod2P / 48.0, 0.25, 1.0);\n" +
                "float cod2Up = dot(reflect(-cod2V, cod2B), u_cod2SkyEye.xyz);\n" +
                "float cod2Fresnel = 0.2 + 0.8 * cod2Gloss * pow(1.0 - max(dot(cod2B, cod2V), 0.0), 5.0);\n" +
                "vec3 cod2Env = cod2Indirect * 2.0 * mix(0.35, 1.0, smoothstep(-0.3, 0.7, cod2Up));\n" +
                "gl_FragColor.rgb += cod2S.rgb * cod2Env * (cod2Fresnel * cod2Gloss * u_cod2Surface.w);\n}\n" +
                // Without material lighting the lightmap's sun share goes alone.
                "} else if (u_cod2SunShadow.x > 0.0) {\n" +
                "vec4 cod2L = " + lightmapTexel + ";\n" +
                "float cod2Shade = 1.0 - cod2SunShadow(cod2L.a);\n" +
                "if (cod2Shade > 0.0) gl_FragColor.rgb *= max(cod2L.rgb - u_cod2SunColor.rgb * (cod2L.a * cod2Shade), vec3(0.0)) /\n" +
                "  max(cod2L.rgb, vec3(0.004));\n}\n";
            }
            pointPass = materialPass + pointPass;
            if (!pointNormal && !materialLightmap) {
              // The DX9 zfeather: an effect fades where the scene's depth
              // (unit 19, copied before the first feathered draw) comes within
              // 1 / featherParms.x units of it, and as close to the eye.
              pointDefs += "uniform highp sampler2D u_cod2FeatherDepth;\n";
              cod2EffectPass = "if (u_cod2Effect.y != 0.0) {\n" +
                "highp float cod2Z = -v_cod2Eye.z, cod2Scale = abs(u_cod2Effect.y);\n" +
                "highp float cod2SceneZ = u_cod2Effect.w / max(0.9995002746582031 -\n" +
                "  texelFetch(u_cod2FeatherDepth, ivec2(gl_FragCoord.xy), 0).r, 0.000001);\n" +
                "float cod2Feather = clamp(abs(cod2SceneZ - cod2Z) * cod2Scale, 0.0, 1.0) *\n" +
                "  clamp((cod2Z - u_cod2Effect.w / 0.9995002746582031) * cod2Scale, 0.0, 1.0);\n" +
                "if (u_cod2Effect.y > 0.0) gl_FragColor.a *= cod2Feather; else gl_FragColor *= cod2Feather;\n}\n" +
                // Fire burns hotter the brighter it already is.
                "if (u_cod2Effect.z > 0.0)\n" +
                "  gl_FragColor.rgb *= 1.0 + u_cod2Effect.z * dot(gl_FragColor.rgb, vec3(0.299, 0.587, 0.114));\n";
            }
          }
          var vsSource = ['''
POINT_VERTEX_ORIGINAL = 'vsPointSizeDefs, vsClipPlaneDefs, vsLightingDefs, "void main()"'
POINT_VERTEX_LEGACY = 'vsPointSizeDefs, vsClipPlaneDefs, vsLightingDefs, pointVarying, "void main()"'
POINT_POSITION_ORIGINAL = '"  gl_Position = u_projection * ecPosition;", "  v_color = a_color;"'
POINT_POSITION_LEGACY = '"  gl_Position = u_projection * ecPosition;", (pointLights ? "v_cod2Eye = ecPosition.xyz;" : ""), (pointNormal ? "v_cod2Normal = u_normalMatrix * a_normal;" : ""), "  v_color = a_color;"'
POINT_VERTEX_REPLACEMENT = POINT_VERTEX_LEGACY.replace('pointVarying,', 'pointVarying, "uniform lowp int u_cod2ColorOrder;",')
POINT_POSITION_REPLACEMENT = POINT_POSITION_LEGACY.replace('"  v_color = a_color;"',
    '"  vec4 cod2VertexColor = u_cod2ColorOrder == 2 ? a_color.yzwx : (u_cod2ColorOrder == 1 ? a_color.zyxw : a_color);", "  v_color = cod2VertexColor;"')

POINT_FRAGMENT_ORIGINAL = 'fogHeaderIfNeeded, fsClipPlaneDefs, fsAlphaTestDefs, "void main()"'
POINT_FRAGMENT_REPLACEMENT = 'fogHeaderIfNeeded, fsClipPlaneDefs, fsAlphaTestDefs, pointDefs, "void main()"'
POINT_PASS_ORIGINAL = 'fsClipPlanePass, fsTexEnvPass, fogPass, fsAlphaTestPass'
POINT_PASS_LEGACY = 'fsClipPlanePass, fsTexEnvPass, pointPass, fogPass, fsAlphaTestPass'
POINT_PASS_REPLACEMENT = 'fsClipPlanePass, fsTexEnvPass, pointPass, cod2EffectPass, fogPass, fsAlphaTestPass'

COVERAGE_MARKER = '          // COD2_ALPHA_COVERAGE:'
COVERAGE_ORIGINAL = '          var fsSource = [ "precision mediump float;",'
COVERAGE_REPLACEMENT = r'''          // COD2_ALPHA_COVERAGE: an alpha-tested surface the device flags
          // (web_effects.c) resolves its edge into the multisampled scene's
          // coverage. Its alpha, scaled up a quarter per mip level so leaves
          // and grass stay as full far away as near, turns from clear to
          // solid across one pixel about the reference.
          if (pointLights && (GLEmulation.alphaTestFunc == 516 || GLEmulation.alphaTestFunc == 518) && fsAlphaTestPass) {
            fsAlphaTestPass = "if (u_cod2Effect.x > 0.5) {\n" +
              "  highp vec2 cod2Texels = " + pointUV + " * vec2(textureSize(u_texUnit0, 0));\n" +
              "  highp vec2 cod2Dx = dFdx(cod2Texels), cod2Dy = dFdy(cod2Texels);\n" +
              "  float cod2Mip = max(0.5 * log2(max(max(dot(cod2Dx, cod2Dx), dot(cod2Dy, cod2Dy)), 1e-8)) + u_cod2Detail.w, 0.0);\n" +
              "  gl_FragColor.a *= 1.0 + 0.25 * cod2Mip;\n" +
              "  gl_FragColor.a = (gl_FragColor.a - u_alphaTestRef) / max(fwidth(gl_FragColor.a), 0.0001) + 0.5;\n" +
              "  if (gl_FragColor.a <= 0.0) discard;\n" +
              "  gl_FragColor.a = min(gl_FragColor.a, 1.0);\n" +
              "} else {\n" + fsAlphaTestPass + "\n}\n";
          }
          var fsSource = [ "precision mediump float;",'''
POINT_UNIFORMS_LEGACY = r'''        this.normalMatrixLocation = GLctx.getUniformLocation(this.program, "u_normalMatrix");
        this.cod2LightCountLocation = GLctx.getUniformLocation(this.program, "u_cod2LightCount");
        this.cod2LightEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2LightEye[0]");
        this.cod2LightColorLocation = GLctx.getUniformLocation(this.program, "u_cod2LightColor[0]");
        this.cod2LightCount = -1;
        this.cod2LightRevision = -1;'''
POINT_UNIFORMS_ORIGINAL = '        this.normalMatrixLocation = GLctx.getUniformLocation(this.program, "u_normalMatrix");'
POINT_UNIFORMS_REPLACEMENT = r'''        this.normalMatrixLocation = GLctx.getUniformLocation(this.program, "u_normalMatrix");
        this.cod2ColorOrderLocation = GLctx.getUniformLocation(this.program, "u_cod2ColorOrder");
        this.cod2ColorOrder = -1;
        this.cod2LightCountLocation = GLctx.getUniformLocation(this.program, "u_cod2LightCount");
        this.cod2LightEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2LightEye[0]");
        this.cod2LightColorLocation = GLctx.getUniformLocation(this.program, "u_cod2LightColor[0]");
        this.cod2ShadowCountLocation = GLctx.getUniformLocation(this.program, "u_cod2ShadowCount");
        this.cod2ShadowEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2ShadowEye[0]");
        this.cod2SurfaceLocation = GLctx.getUniformLocation(this.program, "u_cod2Surface");
        this.cod2DetailLocation = GLctx.getUniformLocation(this.program, "u_cod2Detail");
        this.cod2SunEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2SunEye");
        this.cod2SunColorLocation = GLctx.getUniformLocation(this.program, "u_cod2SunColor");
        this.cod2ModelLocation = GLctx.getUniformLocation(this.program, "u_cod2Model");
        this.cod2SkyEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2SkyEye");
        this.cod2DetailMapLocation = GLctx.getUniformLocation(this.program, "u_cod2DetailMap");
        this.cod2NormalMapLocation = GLctx.getUniformLocation(this.program, "u_cod2NormalMap");
        this.cod2SpecMapLocation = GLctx.getUniformLocation(this.program, "u_cod2SpecMap");
        this.cod2CamoLocation = GLctx.getUniformLocation(this.program, "u_cod2Camo");
        this.cod2CamoMapLocation = GLctx.getUniformLocation(this.program, "u_cod2CamoMap");
        this.cod2CamoMaskLocation = GLctx.getUniformLocation(this.program, "u_cod2CamoMask");
        this.cod2SunShadowEyeLocation = GLctx.getUniformLocation(this.program, "u_cod2SunShadowEye");
        this.cod2SunShadowLocation = GLctx.getUniformLocation(this.program, "u_cod2SunShadow");
        this.cod2SunShadowRectLocation = GLctx.getUniformLocation(this.program, "u_cod2SunShadowRect");
        this.cod2SunShadowMapLocation = GLctx.getUniformLocation(this.program, "u_cod2SunShadowMap");
        this.cod2EffectLocation = GLctx.getUniformLocation(this.program, "u_cod2Effect");
        this.cod2ReliefLocation = GLctx.getUniformLocation(this.program, "u_cod2Relief");
        this.cod2LensLocation = GLctx.getUniformLocation(this.program, "u_cod2Lens");
        this.cod2LensSceneLocation = GLctx.getUniformLocation(this.program, "u_cod2LensScene");
        this.cod2FeatherDepthLocation = GLctx.getUniformLocation(this.program, "u_cod2FeatherDepth");
        this.cod2MaterialUniforms = null;
        this.cod2LightCount = -1;
        this.cod2LightRevision = -1;
        this.cod2ShadowCount = -1;
        this.cod2ShadowRevision = -1;'''
POINT_UPLOAD_LEGACY = r'''        // COD2_POINT_LIGHT_UNIFORMS: cache per program; no allocation per draw.
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
POINT_UPLOAD_ORIGINAL = '        if (GLImmediate.mode == GLctx.POINTS) {\n          if (this.pointSizeLocation) {'
POINT_UPLOAD_REPLACEMENT = r'''        // COD2_POINT_LIGHT_UNIFORMS: cache per program; no allocation per draw.
        // Consume the per-draw format even when no FX lights were submitted.
        // Subsequent generic GL draws retain their normal RGBA interpretation.
        var colorOrder = Module.cod2VertexColorOrder || 0;
        Module.cod2VertexColorOrder = 0;
        if (this.cod2ColorOrderLocation !== null && this.cod2ColorOrderLocation !== undefined &&
            this.cod2ColorOrder !== colorOrder) {
          GLctx.uniform1i(this.cod2ColorOrderLocation, colorOrder);
          this.cod2ColorOrder = colorOrder;
        }
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
        // COD2_MATERIAL_UNIFORMS: the device sets Module.cod2Material per draw.
        // Every point-light program applies the detail map.
        var cod2Material = Module.cod2Material;
        if (cod2Material && this.cod2DetailLocation !== null && this.cod2DetailLocation !== undefined) {
          var materialUniforms = this.cod2MaterialUniforms;
          if (!materialUniforms) {
            GLctx.uniform1i(this.cod2DetailMapLocation, 5);
            GLctx.uniform1i(this.cod2NormalMapLocation, 6);
            GLctx.uniform1i(this.cod2SpecMapLocation, 7);
            // Only lit model programs paint camouflage.
            if (this.cod2CamoMapLocation) {
              GLctx.uniform1i(this.cod2CamoMapLocation, 16);
              GLctx.uniform1i(this.cod2CamoMaskLocation, 17);
            }
            // Only lightmapped world programs take the sun's depth map.
            if (this.cod2SunShadowMapLocation) GLctx.uniform1i(this.cod2SunShadowMapLocation, 18);
            // Only unlit programs feather effects into the scene's depth.
            if (this.cod2FeatherDepthLocation) GLctx.uniform1i(this.cod2FeatherDepthLocation, 19);
            // Only lit model programs mirror the world in a scope's glass.
            if (this.cod2LensSceneLocation) GLctx.uniform1i(this.cod2LensSceneLocation, 22);
            // Values are compared per program: world and model draws
            // alternate, but each program mostly sees the same ones.
            materialUniforms = this.cod2MaterialUniforms = [];
            var materialNames = ["surface", "detail", "sunEye", "sunColor", "skyEye", "model", "camo",
              "sunShadowEye", "sunShadow", "sunShadowRect", "effect", "relief", "lens"];
            var materialLocations = [this.cod2SurfaceLocation, this.cod2DetailLocation, this.cod2SunEyeLocation,
              this.cod2SunColorLocation, this.cod2SkyEyeLocation, this.cod2ModelLocation, this.cod2CamoLocation,
              this.cod2SunShadowEyeLocation, this.cod2SunShadowLocation, this.cod2SunShadowRectLocation,
              this.cod2EffectLocation, this.cod2ReliefLocation, this.cod2LensLocation];
            for (var materialIndex = 0; materialIndex < materialNames.length; ++materialIndex) {
              if (!materialLocations[materialIndex]) continue;
              materialUniforms.push({name: materialNames[materialIndex], location: materialLocations[materialIndex],
                last: new Float32Array(cod2Material[materialNames[materialIndex]].length).fill(NaN)});
            }
          }
          for (var materialIndex = 0; materialIndex < materialUniforms.length; ++materialIndex) {
            var materialUniform = materialUniforms[materialIndex], materialValue = cod2Material[materialUniform.name];
            var materialLast = materialUniform.last, materialChanged = false;
            for (var materialComponent = 0; materialComponent < materialLast.length; ++materialComponent) {
              if (materialLast[materialComponent] !== materialValue[materialComponent]) {
                materialLast[materialComponent] = materialValue[materialComponent];
                materialChanged = true;
              }
            }
            if (!materialChanged) continue;
            if (materialLast.length === 3) GLctx.uniform3fv(materialUniform.location, materialValue);
            else if (materialLast.length === 16) GLctx.uniformMatrix4fv(materialUniform.location, false, materialValue);
            else GLctx.uniform4fv(materialUniform.location, materialValue);
          }
        }
        if (this.cod2ShadowCountLocation !== null && this.cod2ShadowCountLocation !== undefined) {
          var shadowLights = Module.cod2PointLights;
          var shadowCount = shadowLights && shadowLights.shadowDrawCount ? shadowLights.shadowDrawCount : 0;
          if (this.cod2ShadowCount !== shadowCount) {
            GLctx.uniform1i(this.cod2ShadowCountLocation, shadowCount);
            this.cod2ShadowCount = shadowCount;
          }
          if (shadowCount && this.cod2ShadowRevision !== shadowLights.shadowEyeRevision) {
            GLctx.uniform4fv(this.cod2ShadowEyeLocation, shadowLights.shadowEye);
            this.cod2ShadowRevision = shadowLights.shadowEyeRevision;
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

TEXTURE_LIMIT_ORIGINAL = '    GLImmediate.MAX_TEXTURES = Math.min(Module["GL_MAX_TEXTURE_IMAGE_UNITS"] || GLctx.getParameter(GLctx.MAX_TEXTURE_IMAGE_UNITS), 28);'
TEXTURE_LIMIT_REPLACEMENT = '''    // Texture coordinates share the vertex attribute limit with position/normal/color.
    GLImmediate.MAX_TEXTURES = Math.min(Module["GL_MAX_TEXTURE_IMAGE_UNITS"] || GLctx.getParameter(GLctx.MAX_TEXTURE_IMAGE_UNITS), 28,
      GLctx.getParameter(GLctx.MAX_VERTEX_ATTRIBS) - GLImmediate.TEXTURE0);'''
TEXTURE_ATTRIBUTES_ORIGINAL = '''        if (this.hasTextures) {
          for (var i = 0; i < GLImmediate.MAX_TEXTURES; i++) {
            if (!GLctx.currentArrayBufferBinding) {'''
TEXTURE_ATTRIBUTES_REPLACEMENT = '''        if (this.hasTextures) {
          for (var i = 0; i < GLImmediate.MAX_TEXTURES; i++) {
            if (!GLctx.currentArrayBufferBinding && this.texCoordLocations[i] !== undefined && this.texCoordLocations[i] >= 0) {'''
COLOR_ATTRIBUTE_ORIGINAL = '          GLctx.vertexAttrib4fv(this.colorLocation, [ 1, 1, 1, 1 ]);'
COLOR_ATTRIBUTE_REPLACEMENT = '          if (this.colorLocation >= 0) GLctx.vertexAttrib4fv(this.colorLocation, [ 1, 1, 1, 1 ]);'


MATRIX_LOAD_ORIGINAL = """function _glLoadMatrixf(matrix) {
  matrix >>>= 0;
  GLImmediate.matricesModified = true;"""
MATRIX_LOAD_REPLACEMENT = """function _glLoadMatrixf(matrix) {
  matrix >>>= 0;
  // COD2_UNCHANGED_MATRIX: preserve versions and inverse/transpose caches.
  // Compare values, never pointers: callers reuse mutable WASM storage.
  var current = GLImmediate.matrix[GLImmediate.currentMatrix];
  var equal = true;
  for (var i = 0; i < 16; ++i) {
    var value = HEAPF32[(matrix >>> 2) + i];
    if (current[i] !== value || (value === 0 && !Object.is(current[i], value))) { equal = false; break; }
  }
  if (equal) return;
  GLImmediate.matricesModified = true;"""
MATRIX_IDENTITY_ORIGINAL = """var _glLoadIdentity = () => {
  GLImmediate.matricesModified = true;"""
MATRIX_IDENTITY_REPLACEMENT = """var _glLoadIdentity = () => {
  // An identity load must still replace signed zeros and nonfinite values.
  var current = GLImmediate.matrix[GLImmediate.currentMatrix];
  var equal = true;
  for (var i = 0; i < 16; ++i) {
    if (!Object.is(current[i], i % 5 === 0 ? 1 : 0)) { equal = false; break; }
  }
  if (equal) return;
  GLImmediate.matricesModified = true;"""
TEXTURE_MATRIX_CACHE_ORIGINAL = "        this.textureMatrixVersion = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];"
TEXTURE_MATRIX_CACHE_REPLACEMENT = "        this.textureMatrixVersion = new Array(2 + GLImmediate.MAX_TEXTURES).fill(-1);"
NORMAL_MATRIX_ORIGINAL = "          if (GLEmulation.lightEnabled) {"
NORMAL_MATRIX_REPLACEMENT = "          if (this.normalMatrixLocation !== null) {"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("javascript", type=Path)
    path = parser.parse_args().javascript
    source = path.read_text()
    marker = '          // COD2_POINT_LIGHT_SHADER:'
    if source.count(marker) == 1:
        start = source.index(marker)
        end = source.index(POINT_DEFS_ORIGINAL, start) + len(POINT_DEFS_ORIGINAL)
        source = source[:start] + POINT_DEFS_ORIGINAL + source[end:]
    # Normalize prior patched uniform blocks too, including the removed ADS
    # material blur. This supports reapplying/upgrading without stale uploads.
    if source.count('        this.cod2LightCountLocation =') == 1:
        start = source.index(POINT_UNIFORMS_ORIGINAL)
        end = source.index('        this.hasTextures = hasTextures;', start)
        source = source[:start] + POINT_UNIFORMS_ORIGINAL + "\n" + source[end:]
    upload_marker = '        // COD2_POINT_LIGHT_UNIFORMS:'
    if source.count(upload_marker) == 1:
        start = source.index(upload_marker)
        end = source.index(POINT_UPLOAD_ORIGINAL, start)
        source = source[:start] + source[end:]
    if source.count(COVERAGE_MARKER) == 1:
        start = source.index(COVERAGE_MARKER)
        end = source.index(COVERAGE_ORIGINAL, start)
        source = source[:start] + source[end:]
    for legacy, original in ((LIGHT_COLOR_LEGACY, LIGHT_COLOR_ORIGINAL),
                             (POINT_VERTEX_LEGACY, POINT_VERTEX_ORIGINAL),
                             (POINT_POSITION_LEGACY, POINT_POSITION_ORIGINAL),
                             (POINT_DEFS_LEGACY, POINT_DEFS_ORIGINAL),
                             (POINT_UNIFORMS_LEGACY, POINT_UNIFORMS_ORIGINAL),
                             (POINT_UPLOAD_LEGACY, POINT_UPLOAD_ORIGINAL),
                             (POINT_PASS_LEGACY, POINT_PASS_ORIGINAL)):
        if source.count(legacy) == 1:
            source = source.replace(legacy, original, 1)
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
                                  (COVERAGE_ORIGINAL, COVERAGE_REPLACEMENT),
                                  (POINT_UNIFORMS_ORIGINAL, POINT_UNIFORMS_REPLACEMENT),
                                  (POINT_UPLOAD_ORIGINAL, POINT_UPLOAD_REPLACEMENT),
                                  (SHADER_CHECK_ORIGINAL, SHADER_CHECK_REPLACEMENT),
                                  (POINT_GLSL_ORIGINAL, POINT_GLSL_REPLACEMENT),
                                  (POINT_VS_UPLOAD_ORIGINAL, POINT_VS_UPLOAD_REPLACEMENT),
                                  (POINT_FS_UPLOAD_ORIGINAL, POINT_FS_UPLOAD_REPLACEMENT),
                                  (TEXTURE_LIMIT_ORIGINAL, TEXTURE_LIMIT_REPLACEMENT),
                                  (TEXTURE_ATTRIBUTES_ORIGINAL, TEXTURE_ATTRIBUTES_REPLACEMENT),
                                  (COLOR_ATTRIBUTE_ORIGINAL, COLOR_ATTRIBUTE_REPLACEMENT),
                                  (MATRIX_LOAD_ORIGINAL, MATRIX_LOAD_REPLACEMENT),
                                  (MATRIX_IDENTITY_ORIGINAL, MATRIX_IDENTITY_REPLACEMENT),
                                  (NORMAL_MATRIX_ORIGINAL, NORMAL_MATRIX_REPLACEMENT),
                                  (TEXTURE_MATRIX_CACHE_ORIGINAL, TEXTURE_MATRIX_CACHE_REPLACEMENT)):
        if source.count(replacement) == 1:
            continue
        if source.count(original) != 1:
            raise SystemExit("Unexpected Emscripten GL generator; refusing to patch")
        source = source.replace(original, replacement, 1)
    path.write_text(source)
    print("Fixed Emscripten GL texture calls, cube samplers, model colors, point lights and indexed client-array ranges")


if __name__ == "__main__":
    main()
