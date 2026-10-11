#!/usr/bin/env python3
"""Converte uma imagem sua (PNG com fundo transparente) em sprite 8-bit para o compowder.

uso: python3 png2sprite.py imagem.png NOME_DO_ARRAY [cores] >> ../custom_sprites_data.h
     (cores = tamanho da paleta, padrao 16; gera tambem NOME_preview.png para conferir)

A imagem e reduzida para 28x44 pixels, com paleta limitada, e sai em RGB565
(0x07E0 = transparente). Depois e so apontar o array em custom_sprites.h.
"""
import sys
from PIL import Image

W, H = 28, 44

def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    path, name = sys.argv[1], sys.argv[2]
    colors = int(sys.argv[3]) if len(sys.argv) > 3 else 16

    im = Image.open(path).convert("RGBA")
    box = im.getbbox()                       # corta as bordas vazias
    if box:
        im = im.crop(box)
    scale = min(W / im.width, H / im.height)  # encaixa mantendo a proporcao
    nw, nh = max(1, round(im.width * scale)), max(1, round(im.height * scale))
    small = im.resize((nw, nh), Image.LANCZOS)
    canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    canvas.paste(small, ((W - nw) // 2, H - nh))   # centraliza, pes no chao

    alpha = canvas.getchannel("A")
    rgb = canvas.convert("RGB").quantize(colors=colors, dither=Image.Dither.NONE).convert("RGB")

    vals, prev = [], Image.new("RGB", (W * 6, H * 6), (225, 200, 250))
    for y in range(H):
        for x in range(W):
            if alpha.getpixel((x, y)) < 128:
                vals.append(0x07E0)
                continue
            r, g, b = rgb.getpixel((x, y))
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            if v == 0x07E0:
                v = 0x07C0                   # evita colidir com a cor transparente
            vals.append(v)
            prev.paste((r, g, b), (x * 6, y * 6, x * 6 + 6, y * 6 + 6))
    prev.save(f"{name}_preview.png")

    print(f"static const uint16_t {name}[{W * H}] PROGMEM = {{")
    for y in range(H):
        print("  " + ",".join(f"0x{v:04X}" for v in vals[y * W:(y + 1) * W]) + ",")
    print("};\n")

if __name__ == "__main__":
    main()
