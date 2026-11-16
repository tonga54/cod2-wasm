"""Fit a projected six-pane donor frame to the existing window UV layouts."""
import numpy as np
from PIL import Image
from remix_shutter_bake import sample

# Pane boundaries in the original upright 128x256 images. Empty and broken
# windows retain their original color alpha; painted interiors stay in place.
LAYOUTS = {
    'v_window02': ([0,14,61,70,114,128], [0,14,84,94,163,172,242,256]),
    'v_window07': ([0,14,54,71,118,128], [0,21,84,94,163,176,238,256]),
    'v_window08': ([0,15,61,69,116,128], [0,18,83,91,163,172,241,256]),
}
SOURCE_X = np.array([0,31,234,278,481,512]) / 512
SOURCE_Y = np.array([0,49,345,356,668,679,975,1024]) / 1024


def fit_frame(baked, projection, name, color, normal, specular, size):
    from browser_remix_assets import specular_image
    w, h = size
    tx, ty = LAYOUTS[name]
    u = np.interp((np.arange(w)+.5)/w, np.array(tx)/128, SOURCE_X)
    v = np.interp((np.arange(h)+.5)/h, np.array(ty)/256, SOURCE_Y)
    vv, uu = np.meshgrid(v, u, indexing='ij')
    uv = np.stack((uu.ravel(), vv.ravel()), axis=1)
    fields = {k: sample(np.asarray(im), uv).reshape((h,w)+np.asarray(im).shape[2:])
              for k,im in baked.items()}
    weight = fields['coverage'] / 255
    # Widen/narrow individual rails, including their tangent-space slopes.
    slope = fields['normal'][:,:,:2] / 127.5 - 1
    slope[:,:,0] *= (np.gradient(u)*w)[None,:] * (projection['width']/projection['height']) / (w/h)
    slope[:,:,1] *= (np.gradient(v)*h)[:,None]
    fields['normal'][:,:,:2] = (np.clip(slope,-.98,.98)*.5+.5)*255
    originals = [im.resize(size, Image.Resampling.LANCZOS) for im in (color,normal,specular)]
    base_color, base_normal, base_spec = [np.asarray(im).copy() for im in originals]
    # The closed window's curtains remain painted behind a level pane. They
    # must not bend glass reflections as though the glass were pleated cloth.
    if name == 'v_window07':
        base_normal[:] = [128,128,255]
        dirt = np.mean(base_color[:,:,:3], axis=2) / 255
        pane_rough = Image.fromarray(np.rint(82 + 38*(1-dirt)).astype(np.uint8))
        base_spec = np.asarray(specular_image(pane_rough)).copy()
        base_spec[:,:,:3] = np.rint(34 + 14*dirt[:,:,None]).astype(np.uint8)
    donor_spec = np.asarray(specular_image(Image.fromarray(np.rint(fields['rough']).astype(np.uint8))))
    def blend(old, new):
        a = weight[:,:,None]
        return np.clip(np.rint(old*(1-a)+new*a),0,255).astype(np.uint8)
    out_color = base_color.copy()
    out_color[:,:,:3] = blend(base_color[:,:,:3], fields['color'])
    # Bilinear alpha retains the authored openings without sharpening halos.
    out_color[:,:,3] = np.asarray(color.getchannel('A').resize(size,Image.Resampling.BILINEAR))
    detail = np.full((h,w,4),128,np.uint8)
    detail[:,:,3] = np.rint(fields['height']*weight).astype(np.uint8)
    return {'color':Image.fromarray(out_color), 'normal':Image.fromarray(blend(base_normal, fields['normal'])),
            'specular':Image.fromarray(blend(base_spec,donor_spec)), 'detail':Image.fromarray(detail)}
