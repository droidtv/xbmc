/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#if defined(HAS_GLES)
#include "GUITextureGLES.h"
#endif
#include "Texture.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"

#if defined(HAS_GLES)


CGUITextureGLES::CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureGLES::Begin(color_t color)
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  glActiveTexture(GL_TEXTURE0);
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());

  // Setup Colors
  for (int i = 0; i < 4; i++)
  {
    m_col[i][0] = (GLubyte)GET_R(color);
    m_col[i][1] = (GLubyte)GET_G(color);
    m_col[i][2] = (GLubyte)GET_B(color);
    m_col[i][3] = (GLubyte)GET_A(color);
  }

  bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[0][3] < 255;

  if (m_diffuse.size())
  {
    if (m_col[0][0] == 255 && m_col[0][1] == 255 && m_col[0][2] == 255 && m_col[0][3] == 255 )
    {
      g_Windowing.EnableGUIShader(SM_MULTI);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_MULTI_BLENDCOLOR);
    }

    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_diffuse.m_textures[0]->GetTextureObject());

    hasAlpha = true;
  }
  else
  {
    if ( hasAlpha )
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE_NOBLEND);
    }
  }

  if ( hasAlpha )
  {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glEnable( GL_BLEND );
  }
  else
  {
    glDisable(GL_BLEND);
  }
  m_packedVertices.clear();
}

void CGUITextureGLES::End()
{
  GLint posLoc  = g_Windowing.GUIShaderGetPos();
  GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();
  GLint tex1Loc = g_Windowing.GUIShaderGetCoord1();
  GLint uniColLoc = g_Windowing.GUIShaderGetUniCol();

  if(uniColLoc >= 0)
  {
    glUniform4f(uniColLoc,(m_col[0][0] / 255.0), (m_col[0][1] / 255.0), (m_col[0][2] / 255.0), (m_col[0][3] / 255.0));
  }

  if(m_diffuse.size())
  {
    glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, u2));
    glEnableVertexAttribArray(tex1Loc);
  }
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, x));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, u1));
  glEnableVertexAttribArray(tex0Loc);

  GLushort *idx;
  idx = new GLushort[m_packedVertices.size()*6 / 4];
  for (unsigned int i=0, size=0; i < m_packedVertices.size(); i+=4, size+=6)
  {
    idx[size+0] = i+0;
    idx[size+1] = i+1;
    idx[size+2] = i+2;
    idx[size+3] = i+2;
    idx[size+4] = i+3;
    idx[size+5] = i+0;
  }

  glDrawElements(GL_TRIANGLES, m_packedVertices.size()*6 / 4, GL_UNSIGNED_SHORT, idx);

  if (m_diffuse.size())
  {
    glDisableVertexAttribArray(tex1Loc);
    glActiveTexture(GL_TEXTURE0);
  }

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  glEnable(GL_BLEND);
  g_Windowing.DisableGUIShader();
  delete [] idx;
}

void CGUITextureGLES::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  PackedVertex vertices[4];

  // Setup texture coordinates
  //TopLeft
  vertices[0].u1 = texture.x1;
  vertices[0].v1 = texture.y1;
  //TopRight
  if (orientation & 4)
  {
    vertices[1].u1 = texture.x1;
    vertices[1].v1 = texture.y2;
  }
  else
  {
    vertices[1].u1 = texture.x2;
    vertices[1].v1 = texture.y1;
  }
  //BottomRight
  vertices[2].u1 = texture.x2;
  vertices[2].v1 = texture.y2;
  //BottomLeft
  if (orientation & 4)
  {
    vertices[3].u1 = texture.x2;
    vertices[3].v1 = texture.y1;
  }
  else
  {
    vertices[3].u1 = texture.x1;
    vertices[3].v1 = texture.y2;
  }

  if (m_diffuse.size())
  {
    //TopLeft
    vertices[0].u2 = diffuse.x1;
    vertices[0].v2 = diffuse.y1;
    //TopRight
    if (m_info.orientation & 4)
    {
      vertices[1].u2 = diffuse.x1;
      vertices[1].v2 = diffuse.y2;
    }
    else
    {
      vertices[1].u2 = diffuse.x2;
      vertices[1].v2 = diffuse.y1;
    }
    //BottomRight
    vertices[2].u2 = diffuse.x2;
    vertices[2].v2 = diffuse.y2;
    //BottomLeft
    if (m_info.orientation & 4)
    {
      vertices[3].u2 = diffuse.x2;
      vertices[3].v2 = diffuse.y1;
    }
    else
    {
      vertices[3].u2 = diffuse.x1;
      vertices[3].v2 = diffuse.y2;
    }
  }

  for (int i=0; i<4; i++)
  {
    vertices[i].x = x[i];
    vertices[i].y = y[i];
    vertices[i].z = z[i];
    vertices[i].r = m_col[i][0];
    vertices[i].g = m_col[i][1];
    vertices[i].b = m_col[i][2];
    vertices[i].a = m_col[i][3];
    m_packedVertices.push_back(vertices[i]);
  }
}

void CGUITextureGLES::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  if (texture)
  {
    glActiveTexture(GL_TEXTURE0);
    texture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
  }

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On

  VerifyGLState();

  GLubyte col[4][4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  if (texture)
    g_Windowing.EnableGUIShader(SM_TEXTURE);
  else
    g_Windowing.EnableGUIShader(SM_DEFAULT);

  GLint posLoc   = g_Windowing.GUIShaderGetPos();
  GLint colLoc   = g_Windowing.GUIShaderGetCol();
  GLint tex0Loc  = g_Windowing.GUIShaderGetCoord0();
  GLint uniColLoc= g_Windowing.GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  if(colLoc >= 0)
    glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, GL_TRUE, 0, col);
  if (texture)
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  if (texture)
    glEnableVertexAttribArray(tex0Loc);
  if(colLoc >= 0)
    glEnableVertexAttribArray(colLoc);

  for (int i=0; i<4; i++)
  {
    // Setup Colour Values
    col[i][0] = (GLubyte)GET_R(color);
    col[i][1] = (GLubyte)GET_G(color);
    col[i][2] = (GLubyte)GET_B(color);
    col[i][3] = (GLubyte)GET_A(color);
  }

  glUniform4f(uniColLoc,col[0][0] / 255, col[0][1] / 255, col[0][2] / 255, col[0][3] / 255);

  // Setup vertex position values
  #define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x))
  ver[0][0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x1, rect.y1));
  ver[0][1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x1, rect.y1));
  ver[0][2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x1, rect.y1));
  ver[1][0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x2, rect.y1));
  ver[1][1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x2, rect.y1));
  ver[1][2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x2, rect.y1));
  ver[2][0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x2, rect.y2));
  ver[2][1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x2, rect.y2));
  ver[2][2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x2, rect.y2));
  ver[3][0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(rect.x1, rect.y2));
  ver[3][1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(rect.x1, rect.y2));
  ver[3][2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(rect.x1, rect.y2));
  if (texture)
  {
    // Setup texture coordinates
    CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
    tex[0][0] = tex[3][0] = coords.x1;
    tex[0][1] = tex[1][1] = coords.y1;
    tex[1][0] = tex[2][0] = coords.x2;
    tex[2][1] = tex[3][1] = coords.y2;
  }
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  if(colLoc >= 0)
    glDisableVertexAttribArray(colLoc);
  if (texture)
    glDisableVertexAttribArray(tex0Loc);

  g_Windowing.DisableGUIShader();
}

#endif
