﻿#include "Framebuffer.h"
#include "ColorBuffer.h"
#include "Image.h"

//#define SLOPE
#define DDA
//#define BRESENHAM

Framebuffer::Framebuffer(Renderer* renderer, int width, int height)
{
    colorBuffer.width = width;
    colorBuffer.height = height;

    texture = SDL_CreateTexture(renderer->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    colorBuffer.pitch = width * sizeof(color_t);
    colorBuffer.data = new uint8_t[colorBuffer.pitch * colorBuffer.height];

}

Framebuffer::~Framebuffer()
{
    SDL_DestroyTexture(texture);
}

void Framebuffer::Update()
{
    SDL_UpdateTexture(texture, nullptr, colorBuffer.data, colorBuffer.pitch);
}

void Framebuffer::Clear(const color_t& color)
{
    for (int i = 0; i < colorBuffer.width * colorBuffer.height; i++)
    {
        ((color_t*)colorBuffer.data)[i] = color;
    }
}

void Framebuffer::DrawPoint(int x, int y, const color_t& color)
{
    if (x < 0 || x >= colorBuffer.width || y < 0 || y >= colorBuffer.height) return;

    //((color_t*)colorBuffer.data)[x + y * colorBuffer.width] = color;

    uint8_t alpha = color.a;
    uint8_t invAlpha = 255 - alpha;

    color_t& destColor = ((color_t*)(colorBuffer.data))[x + y * colorBuffer.width];

    destColor.r = ((color.r * alpha) + (destColor.r * invAlpha)) >> 8;
    destColor.g = ((color.g * alpha) + (destColor.g * invAlpha)) >> 8;
    destColor.b = ((color.b * alpha) + (destColor.b * invAlpha)) >> 8;

}

void Framebuffer::DrawRect(int x, int y, int rect_width, int rect_height, const color_t& color)
{
    for (int sy = y; sy < y + rect_height; sy++)
    {
        for (int sx = x; sx < x + rect_width; sx++)
        {
            DrawPoint(sx, sy, color);
        }
    }
}

void Framebuffer::DrawLine(int x1, int y1, int x2, int y2, const color_t& color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

#if defined(SLOPE)

    if (dx == 0)
    {
        if (y1 > y2) std::swap(y1, y2);
        for (int y = y1; y <= y2; y++)
        {
            DrawPoint(x1, y, color);
        }
    }
    else
    {
        float m = dy / (float)dx;

        float b = y1 - (m * x1);

        if (std::abs(dx) > std::abs(dy))
        {
            if (x1 > x2) std::swap(x1, x2);
            for (int x = x1; x <= x2; x++)
            {
                int y = (int)round((m * x) + b);
                DrawPoint(x, y, color);
            }
        }
        else
        {
            if (y1 > y2) std::swap(y1, y2);
            for (int y = y1; y <= y2; y++)
            {
                int x = (int)round((y - b) / m);
                DrawPoint(x, y, color);
            }
        }
    }
#elif defined(DDA)
    int steps = std::max(std::abs(dx), std::abs(dy));
    float ddx = dx / (float)steps;
    float ddy = dy / (float)steps;

    float x = (float)x1;
    float y = (float)y1;

    for (int i = 0; i < steps; i++)
    {
        DrawPoint((int)x, (int)y, color);
        x += ddx;
        y += ddy;
    }

#endif
}

void Framebuffer::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const color_t& color)
{
    DrawLine(x1, y1, x2, y2, color);
    DrawLine(x2, y2, x3, y3, color);
    DrawLine(x3, y3, x1, y1, color);
}

void Framebuffer::DrawCircleOctants(int cx, int cy, int x, int y, const color_t& color)
{
    DrawLine(cx - x, cy + y, cx + x, cy + y, color);
    DrawLine(cx - x, cy - y, cx + x, cy - y, color);

    DrawLine(cx - y, cy + x, cx + y, cy + x, color);
    DrawLine(cx - y, cy - x, cx + y, cy - x, color);

   /* DrawPoint(cx + x, cy + y, color);
    DrawPoint(cx + x, cy - y, color);
    DrawPoint(cx - x, cy + y, color);
    DrawPoint(cx - x, cy - y, color);

    DrawPoint(cx + y, cy + x, color);
    DrawPoint(cx + y, cy - x, color);
    DrawPoint(cx - y, cy + x, color);
    DrawPoint(cx - y, cy - x, color);*/
}

void Framebuffer::DrawCircle(int cx, int cy, int radius, const color_t& color)
{
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    DrawCircleOctants(cx, cy, x, y, color);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
        DrawCircleOctants(cx, cy, x, y, color);
    }
}

void Framebuffer::DrawSimpleCurve(int x1, int y1, int x2, int y2, int steps, const color_t& color)
{
    float dt = 1.0f / steps;
    for (int i = 0; i < steps; i++)
    {
        float t1 = i * dt;
        float t2 = (i + 1) * dt;

        int sx1 = Lerp(x1, x2, t1);
        int sy1 = Lerp(y1, y2, t1);
        int sx2 = Lerp(x1, x2, t2);
        int sy2 = Lerp(y1, y2, t2);

        DrawLine(sx1, sy1, sx2, sy2, color);
    }

}

void Framebuffer::DrawQuadraticCurve(int x1, int y1, int x2, int y2, int x3, int y3, int steps, const color_t& color)
{
    float dt = 1.0f / steps;
    for (int i = 0; i < steps; i++)
    {
        float t1 = i * dt;
        float t2 = (i + 1) * dt;
        
        float a1 = (float)pow((1.0f - t1), 2.0f);
        float b1 = 2.0f * (1.0f - t1) * t1;
        float c1 = (float)pow(t1, 2.0f);

        int sx1 = (int)(a1 * x1 + b1 * x2 + c1 * x3);
        int sy1 = (int)(a1 * y1 + b1 * y2 + c1 * y3);

        float a2 = (float)pow((1.0f - t2), 2.0f);
        float b2 = 2.0f * (1.0f - t2) * t2;
        float c2 = (float)pow(t2, 2.0f);

        int sx2 = (int)(a2 * x1 + b2 * x2 + c2 * x3);
        int sy2 = (int)(a2 * y1 + b2 * y2 + c2 * y3);

        DrawLine(sx1, sy1, sx2, sy2, color);
    }

}

void Framebuffer::DrawCubicCurve(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int steps, const color_t& color)
{
    float dt = 1.0f / steps;
    for (int i = 0; i < steps; i++)
    {
        // B(t) = (1 - t)³P₀ + 3(1 - t)²tP₁ + 3(1 - t)t²P₂ + t³P₃
        float t1 = i * dt;
        float t2 = (i + 1) * dt;

        float a1 = (float)pow((1 - t1), 3);
        float b1 = (float)3 * pow((1 - t1), 2) * t1;
        float c1 = (float)3 * (1 - t1) * pow(t1, 2);
        float d1 = (float)pow(t1, 3);

        int sx1 = (int)(a1 * x1 + b1 * x2 + c1 * x3 + d1 * x4);
        int sy1 = (int)(a1 * y1 + b1 * y2 + c1 * y3 + d1 * y4);

        float a2 = (float)pow((1 - t2), 3);
        float b2 = (float)3 * pow((1 - t2), 2) * t2;
        float c2 = (float)3 * (1 - t2) * pow(t2, 2);
        float d2 = (float)pow(t2, 3);


        int sx2 = (int)(a2 * x1 + b2 * x2 + c2 * x3 + d2 * x4);
        int sy2 = (int)(a2 * y1 + b2 * y2 + c2 * y3 + d2 * y4);

        DrawLine(sx1, sy1, sx2, sy2, color);
    }
}

void Framebuffer::DrawImage(int x1, int y1, Image* image)
{
    for (int y = 0; y < image->colorBuffer.height; y++)
    {
        int sy = y + y1;
        for (int x = 0; x < image->colorBuffer.width; x++)
        {
            int sx = x + x1;
            if (sx < 0 || sx >= colorBuffer.width || sy < 0 || sy >= colorBuffer.height) continue;

            color_t color = ((color_t*)image->colorBuffer.data)[x + (y * image->colorBuffer.width)];
            DrawPoint(sx, sy, color);
        }
    }
}

int Framebuffer::Lerp(int a, int b, float t)
{
    return (int)(a + ((b - a) * t));
}
