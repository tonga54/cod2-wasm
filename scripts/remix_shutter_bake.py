"""Project an owner's shutter mesh/PBR atlas into a brush-compatible surface."""
import numpy as np
from PIL import Image


def sample(texture, uv):
    """Clamped bilinear atlas sampling; UV v already follows image rows."""
    h,w=texture.shape[:2]
    xy=np.clip(uv*[w,h]-.5,[0,0],[w-1,h-1]);lo=np.floor(xy).astype(int);hi=np.minimum(lo+1,[w-1,h-1])
    f=xy-lo
    if texture.ndim==3:f=f[:,:,None]
    a=texture[lo[:,1],lo[:,0]].astype(float);b=texture[lo[:,1],hi[:,0]].astype(float)
    c=texture[hi[:,1],lo[:,0]].astype(float);d=texture[hi[:,1],hi[:,0]].astype(float)
    return (a*(1-f[:,0])+b*f[:,0])*(1-f[:,1])+(c*(1-f[:,0])+d*f[:,0])*f[:,1]


def project(points, corners, normals, uv, textures, size, minimum_coverage=.9):
    """Orthographic view from -Z, up +Y; retain nearest geometry and UV seams."""
    w,h=size;lo=points.min(0);hi=points.max(0)
    pixel=(hi[:2]-points[:,:2])/(hi[:2]-lo[:2])*[w-1,h-1]
    depth=np.full((h,w),-np.inf)
    color=np.empty((h,w,3),np.uint8);color[:]=[28,24,20]
    normal=np.full((h,w,3),128,np.uint8);normal[:,:,2]=255
    rough=np.full((h,w),220,np.uint8);metal=np.zeros((h,w),np.uint8)
    for c in corners:
        p=points[c[:,0]];n=normals[c[:,1]];st=uv[c[:,2]];q=pixel[c[:,0]]
        xmin,ymin=np.maximum(np.floor(q.min(0)).astype(int),0)
        xmax,ymax=np.minimum(np.ceil(q.max(0)).astype(int),[w-1,h-1])
        if xmax<xmin or ymax<ymin:continue
        e1=q[1]-q[0];e2=q[2]-q[0];det=e1[0]*e2[1]-e1[1]*e2[0]
        if abs(det)<1e-8:continue
        y,x=np.mgrid[ymin:ymax+1,xmin:xmax+1];dx=x-q[0,0];dy=y-q[0,1]
        b=(dx*e2[1]-dy*e2[0])/det;c0=(e1[0]*dy-e1[1]*dx)/det;a=1-b-c0
        z=-(a*p[0,2]+b*p[1,2]+c0*p[2,2])
        keep=(a>=-1e-6)&(b>=-1e-6)&(c0>=-1e-6)&(z>depth[ymin:ymax+1,xmin:xmax+1])
        if not keep.any():continue
        yy,xx=y[keep],x[keep];weights=np.stack((a[keep],b[keep],c0[keep]),-1);coords=weights@st
        color[yy,xx]=np.clip(np.rint(sample(textures['color'],coords)),0,255)
        rough[yy,xx]=np.clip(np.rint(sample(textures['rough'],coords)),0,255)
        metal[yy,xx]=np.clip(np.rint(sample(textures['metal'],coords)),0,255)
        nn=weights@n;nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-8)
        d1,d2=st[1]-st[0],st[2]-st[0];den=d1[0]*d2[1]-d1[1]*d2[0]
        if abs(den)>1e-10:
            tangent=((p[1]-p[0])*d2[1]-(p[2]-p[0])*d1[1])/den
            bitangent=((p[2]-p[0])*d1[0]-(p[1]-p[0])*d2[0])/den
            t=tangent-nn*np.sum(nn*tangent,axis=1,keepdims=True)
            t/=np.maximum(np.linalg.norm(t,axis=1,keepdims=True),1e-8)
            bb=np.cross(nn,t);bb*=np.where(np.sum(bb*bitangent,axis=1,keepdims=True)<0,-1,1)
            micro=sample(textures['normal'],coords)[:,:2]/127.5-1
            nn+=t*micro[:,0,None]+bb*micro[:,1,None]
        # Projected U, row-V and toward-eye depth all point along negative
        # source axes. Encode slopes for the renderer's tangent vec3(x,y,1).
        nn=-nn
        slope=nn[:,:2]/np.maximum(nn[:,2,None],.05)
        normal[yy,xx,:2]=np.rint((np.clip(slope,-.98,.98)*.5+.5)*255)
        depth[yy,xx]=z[keep]
    valid=np.isfinite(depth)
    if valid.mean()<minimum_coverage:raise ValueError('Projection does not fill the required surface coverage')
    bottom,top=depth[valid].min(),depth[valid].max()
    height=np.zeros((h,w),np.uint8)
    height[valid]=np.rint((depth[valid]-bottom)/max(top-bottom,1e-6)*255)
    return {key:Image.fromarray(value) for key,value in
            [('color',color),('normal',normal),('rough',rough),('metal',metal),('height',height),
             ('coverage',valid.astype(np.uint8)*255)]}, {
            'coverage':float(valid.mean()),'depthRange':float(top-bottom),'width':float(hi[0]-lo[0]),'height':float(hi[1]-lo[1])}


def read_mesh(path, name):
    from pxr import Usd,UsdGeom
    stage=Usd.Stage.Open(str(path))
    prim=next(p for p in stage.Traverse() if p.GetTypeName()=='Mesh' and p.GetName()==name)
    mesh=UsdGeom.Mesh(prim)
    if mesh.GetNormalsInterpolation()!='faceVarying':raise ValueError('Expected face-varying source normals')
    uv_attr=UsdGeom.PrimvarsAPI(prim).GetPrimvar('st')
    if uv_attr.GetInterpolation()!='faceVarying':raise ValueError('Expected face-varying source UVs')
    points=np.asarray(mesh.GetPointsAttr().Get(),dtype=float)
    ids=np.asarray(mesh.GetFaceVertexIndicesAttr().Get());normals=np.asarray(mesh.GetNormalsAttr().Get(),dtype=float)
    uv=np.asarray(uv_attr.ComputeFlattened(),dtype=float);uv[:,1]=1-uv[:,1]
    triangles=[];cursor=0
    for count in mesh.GetFaceVertexCountsAttr().Get():
        for k in range(1,count-1):
            indices=[cursor,cursor+k,cursor+k+1]
            triangles.append([[ids[j],j,j] for j in indices])
        cursor+=count
    if not all(np.isfinite(a).all() for a in [points,normals,uv]):raise ValueError('Nonfinite source geometry')
    return points,np.asarray(triangles),normals,uv


def paired(image, normal=False):
    """Two closing leaves, opposite hinges and reflected tangent X."""
    flipped=image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    if normal:
        a=np.asarray(flipped).copy();a[:,:,0]=255-a[:,:,0];flipped=Image.fromarray(a)
    out=Image.new(image.mode,(image.width*2,image.height));out.paste(image,(0,0));out.paste(flipped,(image.width,0))
    return out
