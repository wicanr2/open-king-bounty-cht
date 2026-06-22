# 御封戰將的彩蛋 — 反派通緝名單,其實是開發團隊

在為這個專案逆向原版 DOS 執行檔 `KB.EXE`(1990,New World Computing)時,翻出一個藏了三十多年的小祕密:遊戲裡那些等著你追捕的反派,有幾個其實是**開發者把自己畫進去的諷刺化身**。

原版執行檔裡找不到任何作弊碼或除錯密技,但找得到作者的幽默感。

## 反派 = 開發者

| 遊戲中的反派 | 通緝令上的描述 | 真身 |
|---|---|---|
| **Canegor the Mystic**(別名「The Majestic Sage」威嚴賢者) | 寬大長袍、光頭、全身刻著魔法符號、能在空中漂浮 | **Jon Van Caneghem** — 遊戲設計者。`Canegor` 由 `Caneghem` 變來;這位老大把自己塑造成全場最有架式的「威嚴賢者」 |
| **Mahk Bellowspeak** | 螢光綠的身體、橘色體毛,以及「**無緣無故就大吼大叫的習慣**」 | **Mark Caldwell** — 程式設計師。`Mahk` 就是 `Mark`,把同事愛吼的毛病寫進了通緝令 |

把開發者放進遊戲、互相開玩笑,是 New World Computing 的招牌傳統 —— 後來的《魔法門》(Might and Magic)、《英雄無敵》(Heroes of Might and Magic)系列也一路延續這種惡趣味。

其中 **Canegor = Caneghem** 連維基百科與 CRPG 史學家(CRPG Addict)都認可;**Mahk = Mark** 則是從執行檔字串與通緝令描述推出來的(把同事愛大吼寫進通緝令)。

另一個有意思的:**Hack**(別名 The Rogue)——比較像是在向 1980 年代的 roguelike 經典《Hack》/《Rogue》致敬,而不是某個人。

至於其餘的反派(Murray the Miser、Baron Johnno Makahl、Dread Pirate Rob、Sir Moradon、Bargash、Auric、Czar Nickolai the Mad…),名字與描述看起來也都像在惡搞身邊的人,但**查不到可靠的對照**:原版執行檔的製作名單只列了 5 個人(Caneghem、Mark / Andy Caldwell、Kenneth Mayfield、Vincent DeQuattro),這些名字對不上其餘反派,推測是當年朋友、同事或圈內哏,連專門寫 CRPG 歷史的研究者也只能猜。為免穿鑿附會,這裡只記能確定的兩位。

完整製作群(取自 `KB.EXE` 標題畫面):

- **設計**:Jon Van Caneghem
- **程式**:Mark Caldwell、Andy Caldwell
- **美術**:Kenneth L. Mayfield、Vincent DeQuattro, Jr.
- Copyright © 1990 New World Computing, Inc.

## 開新遊戲時的俏皮話

每次建立新遊戲、引擎在背景鋪設世界時,會閃過一句作者寫的玩笑話:

> *"Please wait while I perform **godlike actions** to make this... a suitable environment for your **bountying enjoyment**!"*
>
> 請稍候,我正在施展**神之手**……為你打造一個適合「賞金狩獵」的世界!

`bountying` 是作者拿遊戲名 *Bounty* 硬湊出來的動詞,英文裡並沒有這個字。

## 一點技術指紋

- 這支執行檔是用 **Borland Turbo C++(1990)** 編譯的(字串 `Turbo C++ - Copyright 1990 Borland Intl.` 還留在裡面)。
- 通關後的結束語:`Thank you for playing King's Bounty.`

## 怎麼找到的

把原版 `KB.EXE` 先解開外殼壓縮(原版用 LZEXE 之類的方式打包),再掃描裡面的可讀文字字串,逐段對照遊戲內容與製作名單比對出來的。圖形資料檔(`256.CC` / `416.CC`)是壓縮過的容器,本身沒有可讀文字。反派與開發者的對照另以網路資料交叉查證。

參考:
- [King's Bounty — Wikipedia](https://en.wikipedia.org/wiki/King%27s_Bounty)
- [The CRPG Addict — Game 149: King's Bounty (1990)](http://crpgaddict.blogspot.com/2014/06/game-149-kings-bounty-1990.html)

> 本頁僅為研究與致敬,引用原版執行檔中的**少量文字**作為佐證(合理引用);原版遊戲與其資料的版權仍屬原權利人,本專案不散布原版檔案本身。向 1990 年的 New World Computing 團隊致敬。
