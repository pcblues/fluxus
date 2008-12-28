// Copyright (C) 2005 Dave Griffiths
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "Renderer.h"
#include "TypePrimitive.h"
#include "State.h"

using namespace Fluxus;
	
TypePrimitive::TypePrimitive() 
{
}

TypePrimitive::TypePrimitive(const TypePrimitive &other) 
{
}

TypePrimitive* TypePrimitive::Clone() const 
{
	return new TypePrimitive(*this); 
}

bool TypePrimitive::LoadTTF(const string &FontFilename)
{
	FT_Error error;
	error = FT_Init_FreeType(&m_Library);
	error = FT_New_Face(m_Library, FontFilename.c_str(), 0, &m_Face);

	if (error)
	{
		cerr<<"TypePrimitive::TypePrimitive: could not load font: "<<FontFilename<<endl;
		return false;
	}

	// use 5pt at 100dpi
	error = FT_Set_Char_Size(m_Face, 50 * 64, 0, 100, 0);
	m_Slot = m_Face->glyph;
	return true;
}

void TypePrimitive::Clear()
{
	for (vector<GlyphGeometry*>::iterator i=m_GlyphVec.begin(); 
		i!=m_GlyphVec.end(); ++i)
	{
		delete *i;
	}
	m_GlyphVec.clear();
}

void TypePrimitive::SetText(const string &s)
{
	Clear();
	
	for (unsigned int n=0; n<s.size(); n++)
	{
		FT_Error error;
		error = FT_Load_Char(m_Face, s[n], FT_LOAD_DEFAULT);
		if (error) return;

		int glList = glGenLists(2);
		GlyphGeometry* geo = new GlyphGeometry;
		BuildGeometry(m_Slot,*geo,0);
		m_GlyphVec.push_back(geo);	
	}
}

void TypePrimitive::SetTextExtruded(const string &s, float depth)
{
	Clear();
	
	for (unsigned int n=0; n<s.size(); n++)
	{
		FT_Error error;
		error = FT_Load_Char(m_Face, s[n], FT_LOAD_DEFAULT);
		if (error) return;

		int glList = glGenLists(2);
		GlyphGeometry* geo = new GlyphGeometry;
		BuildGeometry(m_Slot,*geo,0);
		BuildExtrusion(m_Slot,*geo,-depth);
		//BuildGeometry(m_Slot,*geo,-1000,false);
		m_GlyphVec.push_back(geo);	
	}
}

void TypePrimitive::Render()
{
	for (vector<GlyphGeometry*>::iterator i=m_GlyphVec.begin(); 
		i!=m_GlyphVec.end(); ++i)
	{
		RenderGeometry(**i);
		glTranslatef(m_Slot->metrics.horiAdvance,0,0);
	}
}

void TypePrimitive::RenderGeometry(const GlyphGeometry &geo)
{
	if (m_State.Hints & HINT_UNLIT) glDisable(GL_LIGHTING);
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	if (m_State.Hints & HINT_SOLID)
	{
		for (vector<GlyphGeometry::Mesh>::const_iterator i=geo.m_Meshes.begin(); i!=geo.m_Meshes.end(); i++)
		{
			if (!i->m_Normals.empty())
			{
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT,sizeof(dVector),(void*)(&i->m_Normals.begin()->x));
			}
			
			glVertexPointer(3,GL_FLOAT,sizeof(dVector),(void*)(&i->m_Positions.begin()->x));
			glDrawArrays(i->m_Type,0,i->m_Positions.size());
			
			if (!i->m_Normals.empty())
			{
				glDisableClientState(GL_NORMAL_ARRAY);
			}
		}
	}
	
	if (m_State.Hints & HINT_WIRE)
	{
		glDisable(GL_LIGHTING);
		glPolygonOffset(1,1);
		glColor4fv(m_State.WireColour.arr());
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		for (vector<GlyphGeometry::Mesh>::const_iterator i=geo.m_Meshes.begin(); i!=geo.m_Meshes.end(); i++)
		{
			glVertexPointer(3,GL_FLOAT,sizeof(dVector),(void*)(&i->m_Positions.begin()->x));
			glDrawArrays(i->m_Type,0,i->m_Positions.size());
		}
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); 
		glEnable(GL_LIGHTING);
	}
	
	if (m_State.Hints & HINT_UNLIT) glEnable(GL_LIGHTING);
	
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void TypePrimitive::BuildGeometry(const FT_GlyphSlot &glyph, GlyphGeometry &geo, float depth, bool winding)
{
	vector<double> points;
	GLUtesselator* t = gluNewTess();

#if (defined __APPLE__) && (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4)
	gluTessCallback(t, GLU_TESS_BEGIN_DATA, (GLvoid (*)(...))TypePrimitive::TessBegin);
	gluTessCallback(t, GLU_TESS_VERTEX_DATA, (GLvoid (*)(...))TypePrimitive::TessVertex);
	gluTessCallback(t, GLU_TESS_COMBINE_DATA, (GLvoid (*)(...))TypePrimitive::TessCombine);
	gluTessCallback(t, GLU_TESS_END_DATA, (GLvoid (*)(...))TypePrimitive::TessEnd);
	gluTessCallback(t, GLU_TESS_ERROR_DATA, (GLvoid (*)(...))TypePrimitive::TessError);
#else
	gluTessCallback(t, GLU_TESS_BEGIN_DATA, (void (*)())TypePrimitive::TessBegin);
	gluTessCallback(t, GLU_TESS_VERTEX_DATA, (void (*)())TypePrimitive::TessVertex);
	gluTessCallback(t, GLU_TESS_COMBINE_DATA, (void (*)())TypePrimitive::TessCombine);
	gluTessCallback(t, GLU_TESS_END_DATA, (void (*)())TypePrimitive::TessEnd);
	gluTessCallback(t, GLU_TESS_ERROR_DATA, (void (*)())TypePrimitive::TessError);
#endif

	if (winding)
	{
		gluTessProperty(t, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
		gluTessNormal(t, 0.0f, 0.0f, 1.0f);
	}
	else 
	{
		gluTessProperty(t, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
//		gluTessProperty(t, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NEGATIVE);
		gluTessNormal(t, 0.0f, 0.0f, -1.0f);
	}
	
	gluTessProperty(t, GLU_TESS_TOLERANCE, 0);
	gluTessBeginPolygon(t, &geo);

	unsigned int start=0;
	for(int c=0; c<glyph->outline.n_contours; c++)
	{
		unsigned int end = glyph->outline.contours[c]+1;
		for(unsigned int p = start; p<end; p++)
		{
			points.push_back(glyph->outline.points[p].x);
			points.push_back(glyph->outline.points[p].y);
			points.push_back(depth);
		}
		start=end;
	}

	start=0;
	for(int c=0; c<glyph->outline.n_contours; c++)
	{
		unsigned int end = glyph->outline.contours[c]+1;
		gluTessBeginContour(t);
		for(unsigned int p = start; p<end; p++)
		{
			gluTessVertex(t, &points[p*3],
			                 &points[p*3]);
		}
		start=end;
		gluTessEndContour(t);
	}

	gluTessEndPolygon(t);
	gluDeleteTess(t);
}

void TypePrimitive::TessError(GLenum errCode, GlyphGeometry* geo)
{
    geo->m_Error=errCode;
}


void TypePrimitive::TessVertex(void* data, GlyphGeometry* geo)
{
	double *ptr = (double*)data;
    geo->m_Meshes[geo->m_Meshes.size()-1].m_Positions.push_back(dVector(ptr[0],ptr[1],ptr[2]));
}


void TypePrimitive::TessCombine(double coords[3], void* vertex_data[4], float weight[4], void** outData, GlyphGeometry* geo)
{
   outData=vertex_data;
}


void TypePrimitive::TessBegin(GLenum type, GlyphGeometry* geo)
{
	geo->m_Meshes.push_back(GlyphGeometry::Mesh(type));
}


void TypePrimitive::TessEnd(GlyphGeometry* geo)
{
}

void TypePrimitive::BuildExtrusion(const FT_GlyphSlot &glyph, GlyphGeometry &geo, float depth)
{
	unsigned int start=0;
	geo.m_Meshes.push_back(GlyphGeometry::Mesh(GL_QUADS));
	for(int c=0; c<glyph->outline.n_contours; c++)
	{
		unsigned int end = glyph->outline.contours[c]+1;
		unsigned int p = start+1;
		while(p<end)
		{
			dVector a(glyph->outline.points[p-1].x, glyph->outline.points[p-1].y, 0);
			dVector b(glyph->outline.points[p].x, glyph->outline.points[p].y, 0);
			dVector c(glyph->outline.points[p].x, glyph->outline.points[p].y, depth);
			dVector d(glyph->outline.points[p-1].x, glyph->outline.points[p-1].y, depth);
			
			dVector sidea = a-b;
			dVector sideb = a-c;
			sidea.normalise();
			sideb.normalise();
			dVector n=sidea.cross(sideb);
			n.normalise();
			
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Normals.push_back(n);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Normals.push_back(n);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Normals.push_back(n);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Normals.push_back(n);

			geo.m_Meshes[geo.m_Meshes.size()-1].m_Positions.push_back(a);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Positions.push_back(b);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Positions.push_back(c);
			geo.m_Meshes[geo.m_Meshes.size()-1].m_Positions.push_back(d);
			p++;
		}
		start=end;
	}
}
