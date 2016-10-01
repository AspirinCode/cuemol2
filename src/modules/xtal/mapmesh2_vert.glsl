// -*-Mode: C++;-*-
//
//  mapmesh2_vert.glsl:
//    vertex shader
//

#if (__VERSION__>=140)
#define USE_TBO 1
#else
#extension GL_EXT_gpu_shader4 : enable 
#extension GL_ARB_compatibility : enable
#endif

////////////////////
// Uniform variables

uniform int isolevel;

uniform ivec3 ivdel[12];

uniform ivec2 edgetab[16];

// Volume data field texture buffer
uniform int ncol;
uniform int nrow;
uniform int nsec;

#ifdef USE_TBO
uniform usamplerBuffer dataFieldTex; 
#else
uniform usampler3D dataFieldTex; 
#endif

////////////////////
// Vertex attributes

// 
attribute vec3 a_pos;
attribute float a_plane;
attribute float a_ord;

uint getDensity(ivec3 iv)
{
#ifdef USE_TBO
  int index = iv.x + ncol*(iv.y + nrow*iv.z);
  return texelFetch(dataFieldTex, index).r;
#else
  return texelFetch3D(dataFieldTex, iv, 0).x;
#endif
}

/// get the crossing value between d0 and d1 (uses isolevel)
float getCrossVal(uint d0, uint d1)
{
  if (d0==d1) return -1.0;

  int deld = int(d1)-int(d0);
  return float(isolevel-int(d0))/float(deld);
}

vec4 calcVecCrs(ivec3 tpos, int i0, float crs, int ibase)
{
  ivec3 iv0, iv1;

  int i1 = (i0+1)%4;

  iv0 = tpos + ivdel[ibase+i0];
  iv1 = tpos + ivdel[ibase+i1];

  vec4 v0 = vec4(float(iv0.x), float(iv0.y), float(iv0.z), 1);
  vec4 v1 = vec4(float(iv1.x), float(iv1.y), float(iv1.z), 1);

  return v0 + (v1-v0)*crs;
}

vec4 wvertex(vec4 v)
{
  vec4 ecPosition = gl_ModelViewMatrix * v;
  FogFragCoord = ffog(ecPosition.z);
  return gl_ProjectionMatrix * ecPosition;
}

float ffog(in float ecDistance)
{
    return(abs(ecDistance));
}

void main(void)
{

  //gl_FrontColor = gl_FrontColorIn[0];
  
  ivec3 ipos = ivec3(a_pos.xyz);
  int iplane = int( a_plane );

  uint val[4];
  uint uisolev = uint(isolevel);
  int i;
  int ii;

  {
    uint flag = 0U;
    uint mask = 1U;
    int ibase = iplane*4;
    
    for (ii=0; ii<4; ++ii) {
      ivec3 iv = ipos + ivdel[ii + ibase];
      val[ii] = getDensity(iv);
      if (val[ii]>uisolev)
        flag += mask;
      mask = mask << 1U;
    }
    
    if (flag==0U || flag>=15U) {
      discard();
      return;
    }

    ivec2 i01 = edgetab[flag];
    float crs0 = getCrossVal(val[i01.x], val[(i01.x+1)%4]);
    float crs1 = getCrossVal(val[i01.y], val[(i01.y+1)%4]);
    
    if (crs0<-0.0 || crs1<-0.0) {
      discard();
      return;
    }

    vec4 v;
    if (a_ord>0)
      v = calcVecCrs(ipos, i01.x, crs0, ibase);
    else
      v = calcVecCrs(ipos, i01.y, crs1, ibase);

    gl_Position = wvertex(v);
  }

  //gl_Position=gl_Vertex;
  //gl_FrontColor=gl_Color;
  gl_FrontColor=vec4(1.0, 1.0, 1.0, 1.0);
}
