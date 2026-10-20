#include <emscripten.h>

/* The pad's strong and weak motors, 0 to 1, through web_haptics.js. */
EM_JS(void, WebHaptics_Rumble, (float strong, float weak), {
    if (!Module.cod2Haptics) Module.cod2Haptics = Module.cod2CreateHaptics();
    Module.cod2Haptics.rumble(strong, weak);
});
