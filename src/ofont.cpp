#include "ocontext.h"
#include "ofont.h"

__glcFont::__glcFont(GLint inID, __glcMaster *inParent)
{
  __glcUniChar *s = inParent->faceFileName->findIndex(0);
  GLCchar *buffer = NULL;
  __glcContextState *state = __glcContextState::getCurrent();

  if (!state) {
    __glcContextState::raiseError(GLC_STATE_ERROR);
    return;
  }

  faceID = 0;
  parent = inParent;
  charMapCount = 0;
  id = inID;

  buffer = (GLCchar*)malloc(s->lenBytes());
  if (!buffer) {
    face = NULL;
    return;
  }
  s->dup(buffer, s->lenBytes());

  if (FT_New_Face(state->library, 
		  (const char*)buffer, 0, &face)) {
	/* Unable to load the face file, however this should not happen since
	   it has been succesfully loaded when the master was created */
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	face = NULL;
	return;
    }
    
    /* select a Unicode charmap */
    if (FT_Select_Charmap(face, ft_encoding_unicode)) {
	/* Arrghhh, no Unicode charmap is available. This should not happen
	   since it has been tested at master creation */
	__glcContextState::raiseError(GLC_INTERNAL_ERROR);
	FT_Done_Face(face);
	face = NULL;
	return ;
    }

    free(buffer);
}

__glcFont::~__glcFont()
{
  FT_Done_Face(face);
}
