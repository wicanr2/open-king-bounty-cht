# King's Bounty / 御封戰將 — 官方繁中譯名對照草表

> **狀態:OCR 草稿,需人工校對。**
> 譯名來源為 1990s 官方繁體中文手冊（DDSC 代理發行《御封戰將》）。
> 中文欄位由掃描手冊經 tesseract OCR（`chi_tra`）抽出，**1990s 掃描品質有限,部分字可能誤辨**;英文欄位以遊戲資料檔 `data/free/*.ini` 與英文手冊 (Kings-Bounty_Manual_DOS_EN) 文字層交叉確認。
> **譯名版權屬原中文代理 (DDSC)**。本表僅作中文化專案內部對照,不含任何手冊全文,亦不得外傳。
>
> 出處頁碼為「手冊內頁碼」(手冊每張掃描頁為 4-up,內頁 1–34)。信心分級:高=英文括號與中文並列且 OCR 清晰;中=中文 OCR 有少量雜訊但可確認;低=OCR 不穩、需核對掃描原圖;待確認=手冊未提供官方譯名。

## 遊戲標題

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 標題 | King's Bounty | 御封戰將 | 高 | 封面/p.1 |

## 人物職業 (Character Classes)

手冊以「人物種類」介紹四種職業。職業頭銜的專有名 (Sir Crimsaun / Lord Palmer / Tynnestra / Mad Moham) 手冊未個別音譯,故標待確認。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 職業 | Knight | 武士 | 高 | p.1, p.6 |
| 職業 | Paladin | 遊俠 | 高 | p.1, p.6 |
| 職業 | Sorceress | 女巫師 | 高 | p.6 |
| 職業 | Barbarian | 蠻俠 (亦見「野蠻人/野人」) | 中 | p.1, p.6 |
| 人名 | Sir Crimsaun the Knight | 待確認 | 待確認 | — |
| 人名 | Lord Palmer the Paladin | 待確認 | 待確認 | — |
| 人名 | Tynnestra the Sorceress | 待確認 | 待確認 | — |
| 人名 | Mad Moham the Barbarian | 待確認 | 待確認 | — |

> 註:職業內文用 Barbarian=「野人」描述兵種化身;封面/選單選角處用「蠻俠」作職業頭銜。中文化時建議職業統一用「蠻俠」,平原怪物 Barbarians 用「野人」(見下)。

## 法術 (Spells) — 14 條

手冊第八章「法術」,分冒險法術 (Adventure Spells) 與戰鬥法術 (Combat Spells)。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 法術 | Bridge | 造橋術 | 高 | p.17 |
| 法術 | Time Stop | 停時術 | 高 | p.17 |
| 法術 | Find Villain | 尋兇術 | 高 | p.17 |
| 法術 | Castle Gate | 入城術 | 高 | p.17 |
| 法術 | Town Gate | 入鄉術 | 高 | p.17 |
| 法術 | Instant Army | 召兵術 | 高 | p.17 |
| 法術 | Raise Control | 增控術 | 中 | p.18 |
| 法術 | Clone | 複製術 | 高 | p.18 |
| 法術 | Teleport | 傳送術 | 高 | p.18 |
| 法術 | Fireball | 火球術 | 高 | p.18 |
| 法術 | Lightning (Bolt) | 閃電術 | 高 | p.18 |
| 法術 | Freeze | 凝滯術 | 中 | p.18 |
| 法術 | Resurrect | 復活術 | 高 | p.18 |
| 法術 | Turn Undead | 超渡術 | 高 | p.18 |

> 註:`spells.ini` 內為 `Lightning`，手冊作 `Lightning Bolt`(閃電術);`Raise Control` 手冊作 `Raise_Control`(增控術)。

## 兵種 / 怪物 (Troops) — 25 種

手冊第九章「四大洲之怪物」按地形分五類:平原 (PLAIN)、森林 (FOREST)、城堡 (CASTLE)、地下城 (DUNGEON)、山丘 (HILL)。英文括號名與 `troops.ini` 對照。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 兵種(平原) | Peasants | 農夫 | 高 | p.20 |
| 兵種(平原) | Wolves | 野狼 | 高 | p.20 |
| 兵種(平原) | Nomads | 流浪者 | 高 | p.21 |
| 兵種(平原) | Barbarians | 野人 | 高 | p.21 |
| 兵種(平原) | Archmages | 法師 | 高 | p.20 |
| 兵種(森林) | Sprites | 小妖精 | 中 | p.21 |
| 兵種(森林) | Gnomes | 地精 | 高 | p.21 |
| 兵種(森林) | Elves | 精靈族 | 高 | p.21 |
| 兵種(森林) | Trolls | 洞穴巨人 | 高 | p.21 |
| 兵種(森林) | Druids | 德魯依 (巫師) | 中 | p.21 |
| 兵種(城堡) | Militia | 義勇軍 | 中 | p.22 |
| 兵種(城堡) | Archers | 弓箭手 | 高 | p.22 |
| 兵種(城堡) | Pikemen | 槍兵 | 高 | p.22 |
| 兵種(城堡) | Cavalry | 騎兵 | 高 | p.22 |
| 兵種(城堡) | Knights | 武士 | 高 | p.22 |
| 兵種(地下城) | Skeletons | 骷髏兵 | 高 | p.22, p.23 |
| 兵種(地下城) | Zombies | 僵屍 | 中 | p.22, p.23 |
| 兵種(地下城) | Ghosts | 鬼 | 高 | p.22, p.23 |
| 兵種(地下城) | Vampires | 吸血鬼 | 高 | p.22, p.23 |
| 兵種(地下城) | Demons | 惡魔 | 中 | p.22, p.23 |
| 兵種(山丘) | Orcs | 牛獸人 | 中 | p.23 |
| 兵種(山丘) | Dwarves | 矮人 | 中 | p.23 |
| 兵種(山丘) | Ogres | 食人怪 | 中 | p.23 |
| 兵種(山丘) | Giants | 巨人 | 中 | p.23 |
| 兵種(山丘) | Dragons | 火龍 | 高 | p.23 |

> 待核對(OCR 雜訊):Sprites 手冊作「小妖精」(OCR 一處出現 Spirites,係英文掃描誤);Vampires 一處 OCR 把標題誤標「僵屍」,但括號 (Vampires) 確為吸血鬼。Orcs「牛獸人」字面可能為「半獸人」誤辨,需核對掃描原圖。

## 工藝品 / 寶物 (Artifacts) — 8 件

手冊第十章「八項神的寶物」。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 寶物 | The Sword of Prowess | 王者之劍 | 高 | p.25/26 |
| 寶物 | The Shield of Protection | 護身盾牌 | 高 | p.25 |
| 寶物 | The Crown of Command | 王冠 (指揮王冠) | 中 | p.25 |
| 寶物 | The Articles of Nobility | 貴族契約 | 高 | p.26 |
| 寶物 | The Amulet of Augmentation | 強力護身符 | 高 | p.26 |
| 寶物 | The Ring of Heroism | 英雄指環 | 高 | p.25 |
| 寶物 | The Book of Necros | 尼克羅斯之書 | 高 | p.25 |
| 寶物 | The Anchor of Admirability | 海權之錨 | 高 | p.26 |

> 註:`artifacts.ini` 作 `The Anchor of Admirability`,英文手冊作 `Admirality`,中文「海權之錨」。Crown of Command 中文 OCR 僅見「…王冠」,完整字串需核對掃描原圖。

## Boss / 惡棍 (Villains) — 17 名

手冊以「惡棍」(generic) 統稱,**未提供 17 名 villain 的個別官方音譯**。以下皆待確認,中文化需自行音譯或沿用英文。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| Boss | Maurice the Griefer | 待確認 | 待確認 | — |
| Boss | Adam the Rogue | 待確認 | 待確認 | — |
| Boss | Princess Mia | 待確認 | 待確認 | — |
| Boss | Lord Ironfist | 待確認 | 待確認 | — |
| Boss | Dread Pirate Robin | 待確認 | 待確認 | — |
| Boss | Karn the Wizard | 待確認 | 待確認 | — |
| Boss | Ragnar Lodbrok | 待確認 | 待確認 | — |
| Boss | Sebastian Black | 待確認 | 待確認 | — |
| Boss | Grishmak | 待確認 | 待確認 | — |
| Boss | Skellor Boroughes | 待確認 | 待確認 | — |
| Boss | Scarecrow | 待確認 | 待確認 | — |
| Boss | Mahogany Klaw | 待確認 | 待確認 | — |
| Boss | Sir James the Traitor | 待確認 | 待確認 | — |
| Boss | Czar Boris III | 待確認 | 待確認 | — |
| Boss | Solidus the Wise | 待確認 | 待確認 | — |
| Boss | Aklor Deathbringer | 待確認 | 待確認 | — |
| Boss | Uwar Drakaris | 待確認 | 待確認 | — |

## 城鎮 / 城堡 / 大陸 (Towns / Castles / Continents)

手冊正文使用城鎮 (Town)、城堡 (Castle) 等地形通名,**未逐一音譯 26 個城鎮 / 26 個城堡的專有地名**;`land.ini` 的大陸名 (Flandria 等) 亦未在手冊出現官方譯名。全部待確認,中文化需自行音譯。

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 通名 | Town | 鄉鎮 | 高 | p.16, p.21 |
| 通名 | Castle | 城堡 | 高 | p.21 |
| 通名 | Dungeon | 地下城 | 高 | p.20 |
| 通名 | Continent | 洲 (四大洲) | 高 | p.20 |
| 城鎮專名 ×26 | Amigale, Mullah, …, Xian | 待確認 | 待確認 | — |
| 城堡專名 ×26 | Akropos, Bakta, …, Zau Fu | 待確認 | 待確認 | — |
| 大陸名 | Flandria | 待確認 | 待確認 | — |

## 其他關鍵專名 (供敘事文本參考)

| 類別 | 英文 | 官方繁中 | 信心 | 出處頁 |
|---|---|---|---|---|
| 人物 | King Maximus | 馬克馬斯國王 | 中 | p.1, p.16 |
| 物品 | Sceptre of Order | 秩序的權杖 / 國王權杖 | 高 | p.6 |
| 反派陣營 | Dark Legions (Arech Dragonbreath) | 待確認 | 待確認 | — |

---

### 抽取方法摘要

- 繁中手冊為 7 張 A4 掃描頁 (4-up,內頁 1–34),無文字層;以 `pdftoppm -r 400 -gray` 轉 PNG,Docker (debian:bookworm-slim) 內 `tesseract 5.3 chi_tra+eng` OCR。
- 英文詞表來自 `data/free/{troops,villains,spells,artifacts,towns,castles,land}.ini` 的 `name =`;英文手冊文字層用於確認英文拼寫與職業頭銜。
- 手冊以「中文(English)」並列格式呈現法術 / 兵種 / 寶物,故這三類配對信心最高;職業、Boss、地名手冊未逐一譯名。

### 統計

- 已配出官方譯名:**約 48 條**(標題 1、職業 4、法術 14、兵種 25、寶物 8,另含通名與關鍵專名數條)。
- 待確認:Boss 17、城鎮 26、城堡 26、大陸名、職業專有人名 4。
</content>
</invoke>
