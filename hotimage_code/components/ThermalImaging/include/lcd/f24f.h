//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Это программное обеспечение распространяется свободно. Вы можете размещать
// его на вашем сайте, но не забудьте указать ссылку на мой YouTube-канал
// "Электроника в объектике" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Автор: Надыршин Руслан / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#ifndef _F24F_H
#define _F24F_H

#include "esp_system.h"


// 字体成比例，高度24
#define f24_FLOAT_HEIGHT        24

// 字体表中的字符数
#define f24f_NOFCHARS           10


// 该函数返回一个指向 Char 子表的指针
uint8_t *f24f_GetCharTable(uint8_t Char);

#endif
