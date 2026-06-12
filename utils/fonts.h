/**
 * @file fonts.h
 * @brief Small bitmap font used for in-game HUD text.
 * @author Oskar Dudziak
 */

#ifndef PACMAN_3D_FONTS_H
#define PACMAN_3D_FONTS_H

/** @brief Lookup table of 3x5 glyph patterns indexed by ASCII code. */
inline const char *customFont[256] = { nullptr };

/**
 * @brief Initializes the built-in 3x5 font glyph patterns.
 */
inline void initFont() {
    for (int i = 0; i < 256; i++) customFont[i] = "..." "..." "..." "..." "...";
    customFont['0'] = "###""#.#""#.#""#.#""###";
    customFont['1'] = "..#""..#""..#""..#""..#";
    customFont['2'] = "###""..#""###""#..""###";
    customFont['3'] = "###""..#""###""..#""###";
    customFont['4'] = "#.#""#.#""###""..#""..#";
    customFont['5'] = "###""#..""###""..#""###";
    customFont['6'] = "###""#..""###""#.#""###";
    customFont['7'] = "###""..#""..#""..#""..#";
    customFont['8'] = "###""#.#""###""#.#""###";
    customFont['9'] = "###""#.#""###""..#""###";
    customFont['A'] = "###""#.#""###""#.#""#.#";
    customFont['B'] = "##.""#.#""##.""#.#""##.";
    customFont['C'] = "###""#..""#..""#..""###";
    customFont['D'] = "##.""#.#""#.#""#.#""##.";
    customFont['E'] = "###""#..""###""#..""###";
    customFont['F'] = "###""#..""###""#..""#..";
    customFont['G'] = "###""#..""#.#""#.#""###";
    customFont['H'] = "#.#""#.#""###""#.#""#.#";
    customFont['I'] = "###"".#."".#."".#.""###";
    customFont['J'] = "..#""..#""..#""#.#""###";
    customFont['K'] = "#.#""#.#""##.""#.#""#.#";
    customFont['L'] = "#..""#..""#..""#..""###";
    customFont['M'] = "#.#""###""#.#""#.#""#.#";
    customFont['N'] = "###""#.#""#.#""#.#""#.#";
    customFont['O'] = "###""#.#""#.#""#.#""###";
    customFont['P'] = "###""#.#""###""#..""#..";
    customFont['R'] = "###""#.#""##.""#.#""#.#";
    customFont['S'] = "###""#..""###""..#""###";
    customFont['T'] = "###"".#."".#."".#."".#.";
    customFont['U'] = "#.#""#.#""#.#""#.#""###";
    customFont['W'] = "#.#""#.#""#.#""###""#.#";
    customFont['Y'] = "#.#""#.#"".#."".#."".#.";
    customFont['Z'] = "###""..#"".#.""#..""###";
    customFont['/'] = "..#""..#"".#.""#..""#..";
    customFont[':'] = "..."".#.""..."".#.""...";
    customFont['-'] = "..." "...""###""...""...";
    customFont[' '] = "..." "...""...""...""...";
    customFont['!'] = ".#." ".#."".#.""..."".#.";
    customFont['?'] = "###""..#""##.""..."".#.";
}


#endif //PACMAN_3D_FONTS_H
