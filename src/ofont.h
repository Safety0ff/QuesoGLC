#ifndef __glc_ofont_h
#define __glc_ofont_h

#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glc.h"
#include "constant.h"
#include "omaster.h"

typedef struct {
    GLint id;
    GLint faceID;
    __glcMaster *parent;
    FT_Face face;
    GLint charMap[2][GLC_MAX_CHARMAP];
    GLint charMapCount;
} __glcFont;

#endif /* __glc_ofont_h */
