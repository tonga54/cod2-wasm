"""Transfer the donor's fine wood/steel finish while retaining each gun's UV atlas."""
import io
import struct


def decode_iwi(data):
    from PIL import Image
    w,h=struct.unpack_from('<HH',data,6)
    formats={11:'DXT1',12:'DXT3',13:'DXT5'}
    fmt=formats[data[4]]
    length=((w+3)//4)*((h+3)//4)*(8 if fmt=='DXT1' else 16)
    header=io.BytesIO();Image.new('RGBA',(w,h)).save(header,format='DDS',pixel_format=fmt)
    return Image.open(io.BytesIO(header.getvalue()[:128]+data[-length:])).convert('RGBA')



def thompson_metalness(color):
    """Explicit wood UV islands of the retail 512-square Thompson atlas.

    Keep bright wood grain dielectric too; color-threshold classification
    would turn scratches into metal. The foregrip screw is explicitly steel.
    This mask is authored for the retail atlas, not copied from the donor gun.
    """
    import numpy as np
    from PIL import Image, ImageDraw
    mask=Image.new('L',(2048,2048),255);draw=ImageDraw.Draw(mask)
    islands=[[(177,178),(448,178),(448,267),(177,267)],
             [(449,207),(512,207),(512,271),(449,271)],
             [(0,289),(230,289),(237,321),(0,318)],
             [(25,322),(55,322),(240,367),(240,427),(132,416),(25,363)],
             [(236,271),(282,271),(282,343),(236,343)]]
    for polygon in islands:draw.polygon([(x*4,y*4) for x,y in polygon],fill=0)
    draw.ellipse((33*4,319*4,55*4,341*4),fill=255)
    return np.asarray(mask.resize(color.size,Image.Resampling.LANCZOS),dtype=np.float32)/255


def garand_metalness(color):
    """Retail 1024-square Garand: stock/handguard islands occupy the bottom.

    The receiver, butt plate, screws and cartridge are above the UV seam.
    Classify by authored islands, so pale stock wear never becomes metal.
    """
    import numpy as np
    from PIL import Image, ImageDraw
    mask = Image.new('L', (4096, 4096), 255)
    ImageDraw.Draw(mask).rectangle((0, 616 * 4, 4095, 4095), fill=0)
    return np.asarray(mask.resize(color.size, Image.Resampling.LANCZOS), dtype=np.float32) / 255


def mp40_metalness(color):
    """Authored lower-shell/grip islands in the retail MP40 atlas.

    Coordinates use a 1024-square reference. The barrel, magazine well,
    folding stock, trigger guard and screws remain steel. A dark or worn
    Bakelite pixel cannot be classified correctly from its color alone.
    """
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter
    mask = Image.new('L', (4096, 4096), 255)
    draw = ImageDraw.Draw(mask)
    islands = [[(455,737),(889,737),(897,778),(932,805),(930,817),
                (899,817),(872,793),(846,785),(570,785),(534,809),(506,814),(480,799)],
               [(868,865),(937,842),(1022,967),(940,999),(937,974)]]
    for island in islands:
        draw.polygon([(x*4,y*4) for x,y in island], fill=0)
    for x,y,r in ((512,803,8),(937,903,10)):
        draw.ellipse(((x-r)*4,(y-r)*4,(x+r)*4,(y+r)*4), fill=255)
    region = np.asarray(mask.resize(color.size, Image.Resampling.LANCZOS), dtype=np.float32) / 255
    # Retail diffuse was authored for coated blue-black steel, not a bare
    # conductor. Keep a scattering surface layer on dark intact finish and
    # expose more conductor at the broad pale wear. This is a calibrated
    # subpixel coating mixture, not measured oxide optical constants.
    luminance = np.asarray(color.convert('L').filter(ImageFilter.GaussianBlur(max(color.size)/512)), dtype=np.float32)/255
    wear = np.clip((luminance-.18)/.40,0,1)
    wear = wear*wear*(3-2*wear)
    return region*(.60+.40*wear)


def weapon_roughness(spec, metal, steel_bounds, wood_bounds):
    """Translate retail gloss to bounded steel and dielectric roughness.

    The historical wood_bounds argument also bounds molded Bakelite.
    """
    import numpy as np
    from PIL import Image, ImageFilter
    p=np.floor(spec[:,:,3]/255*31+.5)
    cosine=np.minimum(np.cos((180-175*p/30)*np.pi/720),.9999999)
    power=np.where(p<=30,-np.log(10)/np.log(cosine),30)
    rough=(2/(np.clip(power,1,2048)+2))**.25
    wear=Image.fromarray(np.clip(spec[:,:,:3].mean(axis=2),0,255).astype(np.uint8),'L')
    wear=np.asarray(wear.filter(ImageFilter.GaussianBlur(max(wear.size)/256)),dtype=np.float32)/255
    steel=np.clip(.8*rough+.2*(.55-.12*wear),*steel_bounds)
    timber=np.clip(rough,*wood_bounds)
    return metal*steel+(1-metal)*timber


def thompson_roughness(spec, metal):
    return weapon_roughness(spec, metal, (.36, .65), (.52, .8))


def garand_roughness(spec, metal):
    # Matte service finish on the receiver; dry worn timber on the stock.
    return weapon_roughness(spec, metal, (.48, .72), (.56, .82))


def mp40_roughness(spec, metal):
    # Blued service steel and molded Bakelite, with broad worn highlights.
    return weapon_roughness(spec, metal, (.42, .68), (.38, .62))

def weapon_finish(read, textures, donor, size=2048):
    import numpy as np
    from PIL import Image, ImageFilter
    from browser_remix_assets import write_iwi
    color=decode_iwi(read('images/'+textures['colorMap']+'.iwi'))
    native=color.size
    target=(min(size,color.width*2),min(size,color.height*2))
    c=np.asarray(color,dtype=np.float32)
    # Warm, low-specular wooden stocks keep their own shape, hue and markings.
    spec=decode_iwi(read('images/'+textures['specularMap']+'.iwi')).resize(native,Image.Resampling.BILINEAR)
    sp=np.asarray(spec,dtype=np.float32).copy()
    wood=np.clip((c[:,:,0]-c[:,:,2]-8)/28,0,1)*np.clip((c[:,:,0]-c[:,:,1])/12,0,1)
    normal=decode_iwi(read('images/'+textures['normalMap']+'.iwi')).resize(native,Image.Resampling.BILINEAR)
    n=np.asarray(normal,dtype=np.float32).copy()
    # Separate broad bevels, screws and stamped markings from the retail
    # atlas's pixel-sized pitting. Magnifying and sharpening that pitting made
    # flat steel look hammered; it belongs primarily in the finish instead.
    # Work in source texels so a larger export does not enlarge the filter.
    low=np.asarray(normal.filter(ImageFilter.GaussianBlur(1.0)),dtype=np.float32)
    slope=np.linalg.norm(n[:,:,[3,1]]-127.5,axis=2)
    shape=np.clip((slope-12.0)/20.0,0,1)
    shape=shape*shape*(3-2*shape)
    steel=(1-wood)*(1-shape)
    n=low+(n-low)*(1-0.88*steel[:,:,None])
    # Preserve the broad wear distribution and sharp engravings, while reducing
    # single-texel speckles in color and reflection strength. Alpha is coverage
    # for color and the authored gloss index for specular: do not denoise it.
    soft_color=np.asarray(color.filter(ImageFilter.GaussianBlur(0.65)),dtype=np.float32)
    soft_spec=np.asarray(spec.filter(ImageFilter.GaussianBlur(1.0)),dtype=np.float32)
    c[:,:,:3]+=(soft_color[:,:,:3]-c[:,:,:3])*(0.45*steel[:,:,None])
    sp[:,:,:3]+=(soft_spec[:,:,:3]-sp[:,:,:3])*(0.72*steel[:,:,None])
    def resize(a):
        return np.asarray(Image.fromarray(np.clip(a,0,255).astype(np.uint8),'RGBA').resize(
            target,Image.Resampling.BILINEAR),dtype=np.float32).copy()
    c,sp,n=resize(c),resize(sp),resize(n)
    wood=np.asarray(Image.fromarray(wood.astype(np.float32),'F').resize(
        target,Image.Resampling.BILINEAR))
    detail=[]
    for box in ((24,1120,824,1792),(80,2560,800,2944)):
        patch=donor.crop(box).convert('L').resize((256,256),Image.Resampling.LANCZOS)
        raw=np.asarray(patch,dtype=np.float32)
        local=np.asarray(patch.filter(ImageFilter.GaussianBlur(5)),dtype=np.float32)
        grain=np.clip((raw-local)/32,-1,1)
        # Mirror alternating tiles: continuous value at each shared edge.
        tile=np.block([[grain,grain[:,::-1]],[grain[::-1],grain[::-1,::-1]]])
        detail.append(np.tile(tile,(target[1]//512+1,target[0]//512+1))[:target[1],:target[0]])
    grain=wood*detail[0]+(1-wood)*detail[1]
    c[:,:,:3]*=(1+grain*(0.025+wood*0.035))[:,:,None]
    # Donor machining/grain varies the existing finish, not the painted decals.
    sp[:,:,3]=np.clip(sp[:,:,3]+grain*3,0,255)
    dy,dx=np.gradient(grain)
    n[:,:,3]=np.clip(n[:,:,3]-dx*(0.65+wood*4),1,254)
    n[:,:,1]=np.clip(n[:,:,1]-dy*(0.65+wood*4),1,254)
    n[:,:,0]=n[:,:,2]=n[:,:,1]
    def encode(a,fmt):return write_iwi([Image.fromarray(np.clip(a,0,255).astype(np.uint8),'RGBA')],fmt)
    result={'c':encode(c,'DXT5'),'n':encode(n,'DXT5'),'s':encode(sp,'DXT5')}
    authored = {'weapon_thompson_c': (thompson_metalness, thompson_roughness),
                'weapon_m1garand_c': (garand_metalness, garand_roughness),
                'weapon_mp40_c': (mp40_metalness, mp40_roughness)}
    if textures['colorMap'] in authored:
        from browser_remix_assets import specular_image
        classify, finish_roughness = authored[textures['colorMap']]
        metal=classify(color)
        metal=np.asarray(Image.fromarray(metal,'F').resize(target,Image.Resampling.BILINEAR))
        roughness=finish_roughness(sp,metal)
        finish=np.asarray(specular_image(Image.fromarray(np.rint(roughness*255).astype(np.uint8),'L'))).copy()
        # RGB remains a restrained fallback; the physical shader takes F0
        # from color/metalness and the roughness from the converted alpha.
        result['s']=encode(finish,'DXT5')
        mask=np.full((*metal.shape,4),128,dtype=np.uint8);mask[:,:,3]=np.rint(metal*255).astype(np.uint8)
        result['metal']=encode(mask,'DXT5')
    return result
