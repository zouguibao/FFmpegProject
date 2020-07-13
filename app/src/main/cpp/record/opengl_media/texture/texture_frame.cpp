#include "texture_frame.h"

#define LOG_TAG "TextureFrame"

#include "../../CommondTools.h"
TextureFrame::TextureFrame() {

}

TextureFrame::~TextureFrame() {

}

bool TextureFrame::checkGlError(const char* op) {
	GLint error;
	for (error = glGetError(); error; error = glGetError()) {
		LOGE("error::after %s() glError (0x%x)\n", op, error);
		return true;
	}
	return false;
}
