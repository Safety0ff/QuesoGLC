#include "ocontext.h"
#include "ofont.h"

__glcFont::__glcFont(GLint inID, __glcMaster *inParent)
{
  FT_Face face;
  char buffer[256];

  faceID = 0;
  parent = inParent;
  charMapCount = 0;
  id = inID;

  if (FT_New_Face(__glcContextState::library, (const char*)parent->faceFileName->extract(0, buffer, 256), 0, &face)) {
	/* Unable to load the face file, however this should not happen since
	   it has been succesfully loaded when the master was created */
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	return;
    }
    
    /* select a Unicode charmap */
    if (FT_Select_Charmap(face, ft_encoding_unicode)) {
	/* Arrghhh, no Unicode charmap is available. This should not happen
	   since it has been tested at master creation */
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	return ;
    }
}

__glcFont::~__glcFont()
{
  FT_Done_Face(face);
}
