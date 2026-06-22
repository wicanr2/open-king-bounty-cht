#!/usr/bin/env python3
# 產生 Android 觸控 UX mockup 的 SVG (純向量,無外部相依)。輸出 docs/android/mockups/*.svg
import os

OUT = os.path.join(os.path.dirname(__file__), "mockups")
os.makedirs(OUT, exist_ok=True)

W, H = 1180, 620
FONT = "'Noto Sans CJK TC','PingFang TC','Microsoft JhengHei','Heiti TC',sans-serif"
ACC = "#e8821e"      # 主題橘
CTRL = "#ffffff"     # 控制元件
CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H = 150, 70, 880, 480  # 遊戲畫布區


def head(title):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">
<rect width="{W}" height="{H}" rx="26" fill="#14171c"/>
<rect x="40" y="20" width="{W-80}" height="30" rx="6" fill="none"/>
<text x="{W//2}" y="42" fill="#8b95a3" font-size="20" text-anchor="middle">{title}</text>
<rect x="{CANVAS_X}" y="{CANVAS_Y}" width="{CANVAS_W}" height="{CANVAS_H}" rx="6" fill="#0a0c10" stroke="#2a2f37"/>'''


TAIL = "</svg>"


def dpad(cx, cy, r=66, op=0.55):
    a = r * 0.42
    arm = f'M {cx-a} {cy-r} h {2*a} v {r-a} h {r-a} v {2*a} h {-(r-a)} v {r-a} h {-2*a} v {-(r-a)} h {-(r-a)} v {-2*a} h {r-a} z'
    s = f'<g opacity="{op}"><path d="{arm}" fill="#000" stroke="{CTRL}" stroke-width="2.5"/>'
    # 四個三角箭頭
    tri = [
        (cx, cy-r+10, cx-9, cy-r+24, cx+9, cy-r+24),   # up
        (cx, cy+r-10, cx-9, cy+r-24, cx+9, cy+r-24),   # down
        (cx-r+10, cy, cx-r+24, cy-9, cx-r+24, cy+9),   # left
        (cx+r-10, cy, cx+r-24, cy-9, cx+r-24, cy+9),   # right
    ]
    for t in tri:
        s += f'<polygon points="{t[0]},{t[1]} {t[2]},{t[3]} {t[4]},{t[5]}" fill="{CTRL}"/>'
    s += '</g>'
    return s


def rbtn(cx, cy, label, sub, color=CTRL, r=42, op=0.6):
    s = f'<g opacity="{op}"><circle cx="{cx}" cy="{cy}" r="{r}" fill="#000" stroke="{color}" stroke-width="3"/>'
    s += f'<text x="{cx}" y="{cy+9}" fill="{color}" font-size="30" font-weight="700" text-anchor="middle">{label}</text></g>'
    if sub:
        s += f'<text x="{cx}" y="{cy+r+20}" fill="#8b95a3" font-size="15" text-anchor="middle">{sub}</text>'
    return s


def pill(x, y, w, label, h=40, fill="#000", stroke=ACC, tcol="#fff", op=0.85, fs=17):
    return (f'<g opacity="{op}"><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{h//2}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>'
            f'<text x="{x+w//2}" y="{y+h//2+6}" fill="{tcol}" font-size="{fs}" text-anchor="middle">{label}</text></g>')


def hamburger(x, y):
    s = f'<g opacity="0.8"><rect x="{x}" y="{y}" width="44" height="36" rx="8" fill="#000" stroke="{CTRL}" stroke-width="2"/>'
    for i in range(3):
        s += f'<rect x="{x+11}" y="{y+10+i*7}" width="22" height="3" rx="1.5" fill="{CTRL}"/>'
    s += '</g>'
    return s


def label(x, y, t, col="#cdd3da", fs=18, anchor="start", weight="400"):
    return f'<text x="{x}" y="{y}" fill="{col}" font-size="{fs}" text-anchor="{anchor}" font-weight="{weight}">{t}</text>'


def note(t):
    return label(CANVAS_X, H-18, t, col="#6b7480", fs=15)


def cx(frac):  # canvas-relative x
    return int(CANVAS_X + CANVAS_W * frac)


def cy(frac):
    return int(CANVAS_Y + CANVAS_H * frac)


# ---- 畫布內的示意背景 ----
def topbar(text):
    return (f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+8}" width="{CANVAS_W-16}" height="34" rx="4" fill="#1b2740" stroke="{ACC}"/>'
            f'{label(CANVAS_X+20, CANVAS_Y+31, text, col="#ffd479", fs=16)}'
            + hamburger(CANVAS_X+CANVAS_W-56, CANVAS_Y+10))


# ================= 各畫面 =================
def scr_map():
    s = head("① 世界地圖 — 移動")
    # 地圖示意:綠地 + 森林邊 + 河 + 英雄
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#2f9e3f"/>'
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="60" fill="#1f7a30"/>'
    s += f'<path d="M {cx(0.5)} {cy(0.3)} q 60 80 0 160 q -60 80 0 150" stroke="#3a6bd6" stroke-width="26" fill="none" opacity="0.85"/>'
    s += f'<circle cx="{cx(0.42)}" cy="{cy(0.55)}" r="13" fill="#fff"/><circle cx="{cx(0.42)}" cy="{cy(0.55)}" r="7" fill="{ACC}"/>'
    s += label(cx(0.42)-2, cy(0.55)-20, "英雄", col="#fff", fs=13, anchor="middle")
    s += topbar("選項   操作說明   剩餘天數:600")
    # 控制
    s += dpad(245, 470)
    s += rbtn(905, 430, "A", "Enter")
    s += rbtn(975, 495, "B", "ESC")
    s += pill(cx(0.62), cy(0.78), 92, "法術")
    s += pill(cx(0.62)+104, cy(0.78), 92, "查看")
    s += note("左下 D-pad 走地圖｜右下 A/B｜右上 ☰ 系統選單｜情境鍵(法術/查看)讀當前 keymap 浮現")
    return s + TAIL


def scr_town():
    s = head("② 城鎮 / 城堡選單 — 字母選項")
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#10141c"/>'
    opts = ["A) 接新任務", "B) 租船 (500/週)", "C) 購買情報 (1000)", "D) 造橋 (100)", "E) 買攻城武器 (3000)"]
    for i, o in enumerate(opts):
        yy = CANVAS_Y + 120 + i * 46
        s += f'<rect x="{CANVAS_X+30}" y="{yy-26}" width="{CANVAS_W-260}" height="38" rx="6" fill="#1a2335" stroke="#2e3b55"/>'
        s += label(CANVAS_X+48, yy, o, col="#e6ecf3", fs=20)
    s += label(CANVAS_X+30, CANVAS_Y+86, "Town of Hunterville      GP=10K", col="#ffd479", fs=18)
    s += topbar("城鎮")
    # 右側情境快捷列 (字母)
    letters = ["A", "B", "C", "D", "E"]
    for i, L in enumerate(letters):
        s += rbtn(982, 175 + i*68, L, "", color=ACC, r=25, op=0.95)
    s += rbtn(245, 470, "B", "返回", r=40)
    s += note("選單項目可直接點那一行 = 送該字母｜右側情境快捷列同步浮現該畫面有效字母")
    return s + TAIL


def scr_combat():
    s = head("③ 戰鬥 — 走位 / 動作")
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#3a7a3f"/>'
    # 格陣
    for r in range(5):
        for c in range(8):
            xx = CANVAS_X + 60 + c*70; yy = CANVAS_Y + 90 + r*60
            s += f'<rect x="{xx}" y="{yy}" width="64" height="54" fill="none" stroke="#2c5e31" stroke-width="1"/>'
    s += f'<circle cx="{CANVAS_X+95}" cy="{CANVAS_Y+120}" r="20" fill="#cf3030"/>'
    s += f'<circle cx="{CANVAS_X+CANVAS_W-120}" cy="{CANVAS_Y+330}" r="20" fill="#3050cf"/>'
    s += topbar("戰鬥")
    s += dpad(245, 460, r=62)
    s += rbtn(940, 430, "A", "攻擊/確認")
    s += rbtn(1015, 488, "B", "取消")
    s += pill(cx(0.55), cy(0.86), 100, "用魔法 U")
    s += pill(cx(0.55)+112, cy(0.86), 92, "等待 W")
    s += note("D-pad 選格/走位｜A 確認攻擊｜B 取消｜戰鬥動作(用魔法/等待)由 keymap 決定")
    return s + TAIL


def scr_recruit():
    s = head("④ 招募 — 數字輸入")
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#10141c"/>'
    s += label(cx(0.5), CANVAS_Y+120, "Forest — 254 Gnomes available", col="#e6ecf3", fs=22, anchor="middle")
    s += label(cx(0.5), CANVAS_Y+155, "Recruit how many?", col="#9fb0c4", fs=18, anchor="middle")
    # 步進器
    midx = cx(0.5); yy = CANVAS_Y + 250
    s += rbtn(midx-180, yy, "−", "", color="#fff", r=38, op=0.95)
    s += f'<rect x="{midx-110}" y="{yy-36}" width="220" height="72" rx="10" fill="#0a0c10" stroke="{ACC}" stroke-width="2"/>'
    s += label(midx, yy+13, "123", col="#fff", fs=40, anchor="middle", weight="700")
    s += rbtn(midx+180, yy, "+", "", color="#fff", r=38, op=0.95)
    s += pill(midx-150, yy+70, 120, "最大", stroke="#5a6677")
    s += pill(midx+30, yy+70, 120, "OK (Enter)", stroke=ACC, fill="#3a2a10")
    s += rbtn(1015, 495, "B", "取消")
    s += note("數量型輸入浮出步進器 [−] [數值] [+] + 最大/OK,免叫全鍵盤")
    return s + TAIL


def scr_name():
    s = head("⑤ 角色命名 — Android 系統鍵盤 (IME)")
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#10141c"/>'
    s += label(CANVAS_X+40, CANVAS_Y+110, "Name:", col="#9fb0c4", fs=22)
    s += f'<rect x="{CANVAS_X+130}" y="{CANVAS_Y+84}" width="300" height="40" rx="4" fill="#0a0c10" stroke="{ACC}"/>'
    s += label(CANVAS_X+145, CANVAS_Y+112, "Anr_", col="#fff", fs=24)
    # IME 鍵盤示意
    ky = CANVAS_Y + 180; kx = CANVAS_X + 60
    rows = ["q w e r t y u i o p", "a s d f g h j k l", "z x c v b n m"]
    s += f'<rect x="{kx-20}" y="{ky-20}" width="{CANVAS_W-100}" height="240" rx="10" fill="#1b1f27"/>'
    for ri, row in enumerate(rows):
        keys = row.split()
        offx = kx + ri*22
        for ci, k in enumerate(keys):
            xx = offx + ci*72; yy = ky + ri*64
            s += f'<rect x="{xx}" y="{yy}" width="60" height="52" rx="7" fill="#2b323d"/>'
            s += label(xx+30, yy+34, k, col="#fff", fs=22, anchor="middle")
    s += label(cx(0.5), ky+210, "系統 IME｜SDL_StartTextInput()｜送回命名緩衝", col="#8b95a3", fs=15, anchor="middle")
    s += note("命名走 Android 原生輸入法,不自製鍵盤")
    return s + TAIL


def scr_select():
    s = head("⑥ 選角 — 直接點立繪 / 按鈕")
    s += f'<rect x="{CANVAS_X+8}" y="{CANVAS_Y+50}" width="{CANVAS_W-16}" height="{CANVAS_H-58}" fill="#2a6cc4"/>'
    names = ["A 武士", "B 遊俠", "C 蠻俠", "D 女巫師"]
    cols = ["#7c5a3a", "#3a7c4a", "#7c3a3a", "#7c3a6c"]
    for i, (nm, col) in enumerate(zip(names, cols)):
        xx = CANVAS_X + 70 + i*200; yy = CANVAS_Y + 110
        s += f'<rect x="{xx}" y="{yy}" width="150" height="220" rx="8" fill="{col}" stroke="#fff" stroke-width="2"/>'
        s += f'<circle cx="{xx+75}" cy="{yy+80}" r="34" fill="#e8c9a0"/>'
        s += pill(xx+15, yy+170, 120, nm, op=0.95, fill="#000", stroke="#fff")
    s += topbar("Select Character and Press Start")
    s += pill(cx(0.5)-70, CANVAS_Y+CANVAS_H-46, 140, "L 讀取存檔", stroke="#5a6677")
    s += note("直接點職業立繪或下方按鈕 = 選定;讀檔按鈕對應 L 鍵")
    return s + TAIL


SCREENS = {
    "01-world-map.svg": scr_map,
    "02-town-menu.svg": scr_town,
    "03-combat.svg": scr_combat,
    "04-recruit.svg": scr_recruit,
    "05-name-entry.svg": scr_name,
    "06-char-select.svg": scr_select,
}

for fn, fnc in SCREENS.items():
    with open(os.path.join(OUT, fn), "w") as f:
        f.write(fnc())
    print("wrote", fn)
