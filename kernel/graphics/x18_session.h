#pragma once
#include <stdint.h>
#include "framebuffer.h"
#include "graphics.h"
#include "font.h"
#include "x18/x18.h"

namespace x18session {

// Ядро рисует в формате 0xAARRGGBB тем же порядком полей, что и x18::Color.
inline x18::Color to_x18(Color c) { return x18::Color(c.r, c.g, c.b, c.a); }

// ============================================================================
// Мост Xandr <-> ядро HeartBeat.
//
// Движок x18 (репозиторий Xandr, header-only) собирается прямо в ядро.
// Его Surface накладывается поверх back-буфера framebuffer'а, поэтому сцена
// X18 рисуется теми же примитивами, что и в host-демо, но сразу в кадр ОС.
//
// Это первый шаг: сессия X18 отображается на экране загрузки и отдаёт
// детерминированную чексумму (проверка, что движок реально отработал в ядре).
// ============================================================================

// Surface движка Xandr поверх текущего back-буфера ядра.
inline x18::Surface surface() {
    return x18::Surface::wrap(framebuffer::get_buffer(),
                              (int)framebuffer::get_width(),
                              (int)framebuffer::get_height(),
                              (int)(framebuffer::get_pitch() / 4));
}

// Отрисовать демо-сцену рабочего стола X18 в back-буфер.
// Возвращает чексумму кадра (сумма средних цветов полос).
inline uint32_t draw_desktop(int frame = 0) {
    x18::Surface s = surface();
    x18::Raster r(s);

    x18::demo_scene(r, s.w, s.h, frame, true);

    uint32_t sum = 0;
    int bands = 8;
    int band_h = s.h / bands;
    if (band_h < 1) band_h = 1;
    for (int i = 0; i < bands; i++) {
        int y = i * band_h;
        int h = (i == bands - 1) ? (s.h - y) : band_h;
        sum = sum * 31u + r.sample_avg(0, y, s.w, h);
    }
    return sum;
}

} // namespace x18session
