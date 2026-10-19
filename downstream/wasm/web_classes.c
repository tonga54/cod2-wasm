#include <emscripten.h>

/* The host keeps each gametag's five classes (downstream/gateway). The UI
 * exchanges them as lines of
 * "name\tprimary\tsecondary\tlethal\ttactical\tprimaryCamo\tsecondaryCamo". */
extern void UI_ClassMenu_Loaded(int generation, int status, const char *lines);
extern void UI_ClassMenu_Saved(int generation, int status);

EMSCRIPTEN_KEEPALIVE void web_classes_loaded(int generation, int status, const char *lines) {
    UI_ClassMenu_Loaded(generation, status, lines);
}

EMSCRIPTEN_KEEPALIVE void web_classes_saved(int generation, int status) {
    UI_ClassMenu_Saved(generation, status);
}

EM_JS(void, Web_ClassesLoad, (const char *player, int generation), {
    const name = UTF8ToString(player);
    const field = value => String(value ?? "").split("\t").join(" ").split("\n").join(" ").split("\r").join(" ");
    fetch('/classes?player=' + encodeURIComponent(name), {cache:'no-store'})
      .then(async response => {
        const result = await response.json().catch(() => ({}));
        const classes = Array.isArray(result?.classes) ? result.classes : null;
        // Without the classes, even a success leaves them unavailable.
        const status = response.ok && !classes ? 0 : response.status;
        let text = "";
        // A host from before camouflage keeps the guns' own finish.
        if (classes && response.ok)
            text = classes.map(item => [item?.name, item?.primary, item?.secondary, item?.lethal,
                item?.tactical, item?.primaryCamo ?? "none", item?.secondaryCamo ?? "none"].map(field).join("\t"))
                .join("\n") + "\n";
        const lines = stringToNewUTF8(text.slice(0, 4096));
        try { _web_classes_loaded(generation, status, lines); }
        finally { _free(lines); }
      })
      .catch(() => _web_classes_loaded(generation, 0, 0));
});

EM_JS(void, Web_ClassesSave, (const char *player, const char *lines, int generation), {
    const body = JSON.stringify({player:UTF8ToString(player), classes:UTF8ToString(lines).split("\n")
        .filter(Boolean).map(line => {
            const [name, primary, secondary, lethal, tactical, primaryCamo, secondaryCamo] = line.split("\t");
            return {name, primary, secondary, lethal, tactical, primaryCamo, secondaryCamo};
        })});
    // One request at a time, in order: the last edit is the one kept.
    Module.cod2ClassSave = (Module.cod2ClassSave || Promise.resolve()).then(() =>
        fetch('/classes', {method:'PUT', headers:{'Content-Type':'application/json'}, body})
          .then(response => _web_classes_saved(generation, response.status))
          .catch(() => _web_classes_saved(generation, 0)));
});
