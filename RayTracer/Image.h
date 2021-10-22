#pragma once
#include "ColorBuffer.h"
#include <string>

class Image
{
public:
	bool Load(const std::string& filename, uint8_t alpha = 255);

	friend class FrameBuffer;

public:
	ColorBuffer colorBuffer;
	void Flip();
};