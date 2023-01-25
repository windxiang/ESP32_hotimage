//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Это программное обеспечение распространяется свободно. Вы можете размещать
// его на вашем сайте, но не забудьте указать ссылку на мой YouTube-канал
// "Электроника в объектике" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Автор: Надыршин Руслан / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#include "font.h"
#include "f16f.h"
#include "f24f.h"
#include "f32f.h"
#include "f6x8m.h"

// 带有指向字体字符表提取功能的指针的表。字体到目前为止
const t_font_getchar font_table_funcs[] = {
    f6x8m_GetCharTable,
    f10x16f_GetCharTable,
    f24f_GetCharTable,
    f32f_GetCharTable
};

/**
 * @brief 返回一个指向描述 Char 字符的结构的指针
 *
 * @param FontID
 * @param Char
 * @return uint8_t*
 */
uint8_t* font_GetFontStruct(uint8_t FontID, uint8_t Char)
{
    return font_table_funcs[FontID](Char);
}

/**
 * @brief 返回字符的宽度
 *
 * @param pCharTable
 * @return uint8_t
 */
uint8_t font_GetCharWidth(uint8_t* pCharTable)
{
    return *pCharTable; // 字符宽度
}

/**
 * @brief 返回字符的高度
 *
 * @param pCharTable
 * @return uint8_t
 */
uint8_t font_GetCharHeight(uint8_t* pCharTable)
{
    pCharTable++;
    return *pCharTable; // 字符高度
}

/**
 * @brief 返回字符的数据
 *
 * @param pCharTable
 * @return uint8_t
 */
uint8_t* font_GetCharFont(uint8_t* pCharTable)
{
    return pCharTable + 2; // 字符数据
}