# Gera os sprites 8-bit (indexados, 16 cores) -> sprites.h + preview.png
from PIL import Image
W, H = 28, 44
T,K,HR,HS,SK,SS,WH,EY,MO,A,AS,B,BS,SV,PK,SH = range(16)

def grid(w=W,h=H): return [[T]*w for _ in range(h)]
def rect(g,x0,y0,x1,y1,c):
    for y in range(y0,y1+1):
        for x in range(x0,x1+1):
            if 0<=y<len(g) and 0<=x<len(g[0]): g[y][x]=c
def px(g,x,y,c): rect(g,x,y,x,y,c)
def mirror(g,x0,y0,x1,y1,c):   # desenha e espelha no eixo vertical
    w=len(g[0]); rect(g,x0,y0,x1,y1,c); rect(g,w-1-x1,y0,w-1-x0,y1,c)

# ---------------- cabelo de tras ----------------
def hair_back(spy):
    g=grid()
    rect(g,10,1,17,1,HR); rect(g,8,2,19,2,HR); rect(g,7,3,20,3,HR); rect(g,6,4,21,15,HR)
    if spy==0:   # Sam: longo e liso ate a cintura
        rect(g,5,7,22,24,HR); rect(g,6,25,21,27,HR); rect(g,7,28,20,29,HR)
        mirror(g,5,9,5,24,HS); mirror(g,6,25,6,27,HS)
        for y in range(18,29,2): mirror(g,8,y,8,y+1,HS)
    elif spy==1: # Clover: chanel com pontas viradas pra fora
        rect(g,5,6,22,15,HR); rect(g,6,16,21,17,HR)
        mirror(g,3,15,5,17,HR); mirror(g,2,14,3,15,HR); mirror(g,5,8,5,13,HS); mirror(g,4,17,6,17,HS)
    else:        # Alex: curtinho arredondado
        rect(g,5,6,22,13,HR); rect(g,6,14,21,15,HR); mirror(g,7,16,9,16,HR)
        mirror(g,5,8,5,12,HS)
    return g

# ---------------- rosto + franja ----------------
def head(spy):
    g=grid()
    rect(g,9,5,18,5,SK); rect(g,8,6,19,13,SK); rect(g,9,14,18,14,SK); rect(g,11,15,16,15,SK)
    # olhos grandes de desenho
    mirror(g,9,9,12,9,K)
    mirror(g,10,10,11,11,EY); mirror(g,10,10,10,10,K); mirror(g,12,10,12,11,WH)
    mirror(g,9,12,9,12,SS)                      # blush
    rect(g,13,13,14,13,MO)
    # topo do cabelo
    rect(g,9,3,18,4,HR); rect(g,8,5,19,5,HR)
    if spy==0:   # franja de lado
        rect(g,8,6,16,6,HR); rect(g,8,7,13,7,HR); rect(g,8,8,10,8,HR); rect(g,19,6,19,8,HR)
        rect(g,11,3,16,3,HS); px(g,14,6,HS); px(g,11,7,HS)
    elif spy==1: # repartido no meio/lado
        rect(g,8,6,12,6,HR); rect(g,16,6,19,6,HR); rect(g,8,7,10,7,HR); rect(g,18,7,19,7,HR)
        rect(g,8,8,8,8,HR); rect(g,19,8,19,8,HR); rect(g,13,3,14,5,HS)
    else:        # franja reta
        rect(g,8,6,19,7,HR); rect(g,8,8,9,8,HR); rect(g,18,8,19,8,HR)
        rect(g,10,3,15,3,HS); px(g,12,7,HS); px(g,15,7,HS)
    return g

# ---------------- corpo por estilo ----------------
CATSUIT,DRESS,PANTS,SKIRT=range(4)
def body(style):
    g=grid()
    rect(g,12,15,15,17,SK)                                   # pescoco
    arm = A if style in (CATSUIT,PANTS) else SK
    hand= A if style==CATSUIT else SK
    mirror(g,6,19,8,27,arm); mirror(g,6,28,8,29,hand)        # bracos e maos
    if style in (DRESS,SKIRT): mirror(g,6,19,8,20,A)         # manguinha
    rect(g,8,18,19,19,A); rect(g,9,20,18,22,A); rect(g,10,23,17,25,A); rect(g,9,26,18,27,A)
    mirror(g,9,20,9,22,AS); mirror(g,10,23,10,25,AS)
    rect(g,13,18,14,18,SK)                                   # decote
    if style==CATSUIT:
        rect(g,9,28,18,29,A); mirror(g,9,30,12,40,A); mirror(g,9,30,9,40,AS)
        mirror(g,8,41,12,43,A); mirror(g,8,43,12,43,AS)
        rect(g,9,26,18,26,SV); rect(g,13,26,14,27,PK)        # cinto + fivela
        rect(g,12,18,15,18,A); mirror(g,6,26,8,26,AS)
    elif style==DRESS:
        rect(g,10,25,17,25,B)
        for i,y in enumerate(range(26,34)):
            w=min(i//2+1,4); rect(g,9-w,y,18+w,y,A)
        rect(g,5,33,22,33,AS)
        mirror(g,10,34,12,40,SK); mirror(g,9,41,12,43,B)
    elif style==PANTS:
        rect(g,9,26,18,29,B); mirror(g,9,30,12,40,B); mirror(g,9,30,9,40,BS)
        mirror(g,8,41,12,43,SH); rect(g,12,18,15,18,A); mirror(g,6,27,8,27,AS)
    else:
        rect(g,8,26,19,31,B); rect(g,8,31,19,31,BS); rect(g,9,26,18,26,BS)
        mirror(g,10,32,12,35,SK); mirror(g,9,36,12,43,B); mirror(g,9,36,12,36,BS)
    return g

# ---------------- Jerry (32x32, 2 quadros de boca) ----------------
def jerry(open_mouth):
    g=grid(32,32)
    rect(g,3,24,28,31,A); rect(g,5,22,26,23,A); rect(g,1,27,30,31,A)      # terno
    rect(g,13,19,18,23,SK)                                               # pescoco
    for i in range(6): rect(g,11+i,22+i,20-i,22+i,WH)                    # camisa
    rect(g,15,22,16,23,K); rect(g,15,24,16,29,K); rect(g,14,25,17,27,K)  # gravata
    mirror(g,10,22,12,24,AS)                                             # lapela
    mirror(g,7,9,9,13,HR); mirror(g,8,11,8,14,SK)                        # cabelo lateral + orelha
    rect(g,12,2,19,2,SK); rect(g,10,3,21,4,SK); rect(g,9,5,22,16,SK)
    rect(g,10,17,21,18,SK); rect(g,12,19,19,20,SK)
    rect(g,12,3,15,3,WH)                                                 # brilho da careca
    mirror(g,11,8,14,8,HR)                                               # sobrancelhas
    mirror(g,12,10,13,11,K); mirror(g,12,10,12,10,WH)
    rect(g,15,11,16,14,SS); px(g,15,11,SK); px(g,15,12,SK)               # nariz
    mirror(g,10,13,10,14,SS)
    if open_mouth: rect(g,13,16,18,18,MO); rect(g,14,16,17,16,WH)
    else: rect(g,13,17,18,17,MO)
    return g

def compose(spy,style,glasses=False):
    hb,bd,hd=hair_back(spy),body(style),head(spy)
    g=grid()
    for y in range(H):
        for x in range(W):
            g[y][x]=hd[y][x] or bd[y][x] or hb[y][x]
    if glasses:
        mirror(g,9,9,12,11,K); rect(g,13,10,14,10,K)
    return outline(g)

def outline(g):
    h,w=len(g),len(g[0]); o=[r[:] for r in g]
    for y in range(h):
        for x in range(w):
            if g[y][x]==T and any(0<=y+dy<h and 0<=x+dx<w and g[y+dy][x+dx]!=T for dx,dy in((1,0),(-1,0),(0,1),(0,-1))):
                o[y][x]=K
    return o

SPIES=[("SAM",(238,105,35),(190,70,20),(255,222,190),(240,180,150),(40,160,80),(60,190,90),(35,140,65)),
       ("CLOVER",(255,215,80),(225,170,40),(255,218,188),(240,178,150),(60,130,230),(230,40,60),(170,20,45)),
       ("ALEX",(35,30,45),(80,75,110),(205,150,105),(175,115,80),(120,75,35),(250,205,40),(205,155,20))]
OUT=[("SPY SUIT",CATSUIT,None,None,False),("PASSEIO",SKIRT,(255,255,255),(90,140,230),False),
     ("FESTA",DRESS,(255,60,150),(255,220,60),False),("DISFARCE",PANTS,(60,60,75),(40,40,52),True),
     ("PIJAMA",PANTS,(170,220,255),(120,180,250),False),("PRAIA",SKIRT,(255,150,60),(60,200,190),False)]
def shade(c): return tuple(int(v*0.72) for v in c)
def palette(spy,out):
    n,hr,hs,sk,ss,ey,suit,suits=SPIES[spy]
    a=out[2] or suit; b=out[3] or suit
    return [None,(40,20,35),hr,hs,sk,ss,(255,255,255),ey,(215,60,95),a,shade(a),b,shade(b),(205,205,215),(255,90,170),(245,245,245)]
JPAL=[None,(25,25,30),(170,170,178),(130,130,140),(240,200,170),(205,160,130),(255,255,255),(0,0,0),(140,30,40),(40,40,50),(70,70,85)]+[(0,0,0)]*5

if __name__=="__main__":
    S=4
    im=Image.new("RGB",(6*(W+3)*S,3*(H+2)*S+36*S),(225,200,250))
    def blit(g,pal,ox,oy):
        for y,row in enumerate(g):
            for x,c in enumerate(row):
                if c: im.paste(pal[c],(ox+x*S,oy+y*S,ox+x*S+S,oy+y*S+S))
    for s in range(3):
        for o,out in enumerate(OUT):
            blit(compose(s,out[1],out[4]),palette(s,out),o*(W+3)*S+6,s*(H+2)*S+4)
    blit(outline(jerry(False)),JPAL,10,3*(H+2)*S+8); blit(outline(jerry(True)),JPAL,40*S,3*(H+2)*S+8)
    im.save("preview.png")

    def carr(name,g):
        rows=",\n".join("  "+",".join(str(c) for c in r) for r in g)
        return f"static const uint8_t {name}[{len(g)}][{len(g[0])}] PROGMEM = {{\n{rows}\n}};\n"
    with open("sprites.h","w") as f:
        f.write("// Gerado por gen_sprites.py - sprites 8-bit indexados (16 cores)\n#ifndef SPRITES_H\n#define SPRITES_H\n#include <Arduino.h>\n\n")
        f.write(f"#define SPR_W {W}\n#define SPR_H {H}\n#define JERRY_W 32\n#define JERRY_H 32\n\n")
        f.write("// indices: 0 transparente, 1 contorno, 2 cabelo, 3 cabelo sombra, 4 pele, 5 pele sombra,\n// 6 branco, 7 olho, 8 boca, 9 roupa A, 10 A sombra, 11 roupa B, 12 B sombra, 13 prata, 14 rosa, 15 sapato\n\n")
        for s in range(3): f.write(carr(f"SPR_HAIR_BACK_{s}",hair_back(s))+"\n")
        for s in range(3): f.write(carr(f"SPR_HEAD_{s}",head(s))+"\n")
        for st in range(4): f.write(carr(f"SPR_BODY_{st}",body(st))+"\n")
        f.write(carr("SPR_JERRY_0",outline(jerry(False)))+"\n"+carr("SPR_JERRY_1",outline(jerry(True)))+"\n#endif\n")
