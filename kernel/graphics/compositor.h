#pragma once
#include <stdint.h>
#include "framebuffer.h"
#include "x18/x18_session.h"

namespace compositor {

// ============================================================================
// Композитор HeartBeat на базе X18 Session.
//
// Кадр рисуется движком Xandr (x18::Raster) прямо в back-буфер ядра, который
// служит композиционным буфером сессии. В конце кадра Session копирует на
// экран только изменённый (dirty) регион вместо всего кадра. Вызывающая
// сторона отмечает изменённые области через mark()/mark_all().
// ============================================================================

inline x18::Session& session() {
    static x18::Session s;
    return s;
}

// Экранная поверхность (в неё Session выводит dirty-регион).
inline x18::Surface& out_surface() {
    static x18::Surface s;
    return s;
}

inline void init() {
    x18::Session& s = session();
    s.init(framebuffer::get_buffer(),
           (int)framebuffer::get_width(),
           (int)framebuffer::get_height(),
           (int)(framebuffer::get_pitch() / 4));
    out_surface() = x18::Surface::wrap(framebuffer::real_addr,
                                       (int)framebuffer::get_width(),
                                       (int)framebuffer::get_height(),
                                       (int)(framebuffer::get_pitch() / 4));
}

// Начать новый кадр: сбросить накопленный dirty-регион.
inline void begin() {
    session().dirty = x18::Rect{0, 0, 0, 0};
}

// Отметить изменённую прямоугольную область (мировые координаты).
inline void mark(int x, int y, int w, int h) {
    x18::Rect r{x, y, w, h};
    x18::Session& s = session();
    if (s.dirty.empty()) s.dirty = r;
    else                 s.dirty = s.dirty.join(r);
}

// Отметить изменение всего кадра.
inline void mark_all() {
    session().dirty = x18::Rect{0, 0, session().width, session().height};
}

// Скопировать изменённый регион композиционного буфера на экран.
inline void present() {
    x18::Session& s = session();
    s.out = &out_surface();
    s.end();
}

} // namespace compositor
