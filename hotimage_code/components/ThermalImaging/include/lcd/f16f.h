//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Это программное обеспечение распространяется свободно. Вы можете размещать
// его на вашем сайте, но не забудьте указать ссылку на мой YouTube-канал
// "Электроника в объектике" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Автор: Надыршин Руслан / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#ifndef _F16F_H
#define _F16F_H

#include "esp_system.h"


// 等宽字体，6x8 像素
#define f10x16_FLOAT_WIDTH         10
#define f10x16_FLOAT_HEIGHT        16

// 字体表中的字符数
#define f10x16f_NOFCHARS           256


// 该函数返回一个指向 Char 子表的指针
uint8_t *f10x16f_GetCharTable(uint8_t Char);

#endif
