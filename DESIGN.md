# 設計說明（理由與來源，方便你稽核）

## 加密架構
| 用途 | 演算法 | 位置 |
|---|---|---|
| 鑰匙檔 → 鎖 | SHA-512（串流計算整個檔案） | `crypto.cpp: sha512File` |
| 密碼 + 鎖 → 主金鑰 | PBKDF2-HMAC-SHA512，600,000 次，隨機 32 B salt，輸出 64 B，取前 32 B 當 KEK | `vault.cpp: deriveAndOpen` |
| 包住 RSA 私鑰與 index key | AES-256-GCM（KEK），AAD 綁定 mode/iter/salt | `vault.json` |
| Token 秘密 | 每個 Token 一把隨機 AES-256 金鑰（CEK）→ AES-256-GCM 加密；CEK 用 RSA-4096 OAEP（SHA-256 / MGF1-SHA-256）包起來。明文前面附 SHA-256 做完整性二次檢查 | `.tkn` |
| 名稱 / 說明 / 期限 / 作廢旗標 | AES-256-GCM（index key，登入後在記憶體），AAD 綁定檔案 id | `.tkn`、`_group.grp` |

為什麼這樣分：
- **新增 Token 只需要公鑰**，私鑰平常不進記憶體；只有「顯示 Token」時才用密碼/鑰匙檔重新解開私鑰，所以重新驗證是真的需要，不只是 UI 擋一下。
- 名稱/說明不需驗證即可修改，因此它們用 session 內的 index key 加密，而不是 RSA。
- GCM tag 讓錯誤密碼與檔案被竄改都會被偵測到；AAD 讓檔案不能被互相調換。
- OAEP 單次最多可加密 446 B（4096-bit、SHA-256），所以只用來包 32 B 的 CEK，Token 本身長度不受限。

## 驗證方法（v1.1）
| 因素 | 角色 |
|---|---|
| 密碼、鑰匙檔 | **金鑰材料**：一起進 PBKDF2-HMAC-SHA512 衍生 KEK，沒有它們就解不開任何東西 |
| TOTP（RFC 6238，HMAC-SHA1、6 位、30 秒、±1 步容差） | **額外的驗證關卡**：TOTP 金鑰以 AES-256-GCM（KEK）加密存在 `vault.json` 的 `encTotp`。必須先用密碼／鑰匙檔解開 KEK，才能取出金鑰檢查驗證碼 |

規則由 `validateMode()` 統一檢查（建立、變更都會擋）：至少一種金鑰材料；密碼＋鑰匙檔必須有 TOTP；TOTP 不可單獨使用。我把「雙重認證必須搭配 TOTP」解讀為「同時使用密碼與鑰匙檔時，TOTP 為必要」。若你的意思是「任何多因素都要 TOTP」或「TOTP 必須與其中一種搭配」，只需改 `validateMode()` 一處。

**TOTP 的誠實限制**：驗證碼每 30 秒就變，不可能當作加密金鑰的一部分，所以它保護的是「這支程式的介面」，不是檔案本身。若攻擊者同時拿到 `vault.json` 與你的密碼（或鑰匙檔），可以離線自己解開 KEK 並跳過 TOTP 檢查。真正擋住離線攻擊的仍是密碼強度／鑰匙檔。另外：沒有做驗證碼重放防護（同一個 30 秒內可重用），因為本機程式在解鎖後短時間內還要連續做敏感操作；錯誤訊息一律用同一句，不透露是哪個因素錯。

**重設驗證方法**：先完整驗證（含 TOTP），之後在 UI 上不再要求第二次新鮮驗證碼（`changeCredentials(..., oldTotpAlreadyVerified=true)`），因為使用者填設定表單可能超過 30 秒；密碼／鑰匙檔仍會被重新檢查。RSA 金鑰與 index key 不變，所以 Token 檔不用重寫，只重寫 `vault.json`（新 salt、新 KEK）。

**忘記密碼**：`Vault::wipeAll()` 只刪 `vault.json` 與 `Token/` 底下的檔案（先覆寫再刪除），不碰資料夾裡其他東西。沒有後門、沒有備份金鑰，這是刻意的：任何「不需密碼就能解密」的機制都等於沒有加密。

## Windows ARM64 的 DLL
ARM64 版在 x64 的 runner 上交叉編譯，**不能執行**，所以也不能執行 ARM64 的 `windeployqt`。做法：從 `qt-target/.../msvc2022_arm64` 直接複製 `Qt6Core / Gui / Widgets / Concurrent.dll`、`platforms/qwindows.dll`、`styles/qmodernwindowsstyle.dll`、`imageformats/{qjpeg,qgif,qico}.dll`（有的話），再從 Visual Studio 的 redistributable 資料夾複製 ARM64 的 Visual C++ 執行庫（app-local）。OpenSSL 是靜態連結，不需要 DLL。

因為無法執行，改用 `.github/scripts/check_deps.py` 靜態檢查：用 MSVC 的 `dumpbin` 確認資料夾內每個 exe/dll 都是 ARM64（擋掉誤放的 x64 DLL），而且它們匯入的每個 DLL 不是同資料夾內有，就是 Windows 系統 DLL（`api-ms-*` 或 System32 內存在）。檢查不過整個 job 就失敗、不會發佈。這個腳本的邏輯我用假的 dumpbin 測過（正常、缺 DLL、架構錯誤三種情況）；真正的 `dumpbin` 輸出格式我沒機會驗證，第一次跑 CI 才會知道。

## QR Code
`third_party/qrcodegen`（Nayuki，MIT）從 GitHub 原樣取得。我用 OpenCV 的 QR 解碼器把程式實際畫出的圖解回字串，確認內容就是正確的 `otpauth://` 網址、金鑰與畫面上的 Base32 相同。

## 備份（匯出／匯入）
| 項目 | 做法 |
|---|---|
| 壓縮包 | ZIP，每個檔案以 WinZip AES-256（AE-2）加密，密碼 = 第一組金鑰（48 位 A-Za-z0-9，約 285 bit，`randomId` 以 RAND_bytes 拒絕取樣產生）。自行實作（`aeszip.cpp`，只用 OpenSSL 基本元件） |
| RSA-8192 | 每次匯出新產生一組。私鑰以 AES-256-GCM 加密，金鑰 = PBKDF2-HMAC-SHA512(第二組金鑰, 隨機 salt, 200,000 次) |
| `passcode.txt` | 內容（目前密碼、鑰匙檔 SHA-512、TOTP 金鑰、每個 .ini 的 AES 金鑰）以混合式加密：RSA-8192-OAEP(SHA-256) 包一把 AES-256-GCM 金鑰 |
| `.ini` | 每個 Token / 群組一個檔，各自一把隨機 AES-256-GCM 金鑰（記在 passcode.txt），AAD 綁定檔案路徑，不能互相調換 |
| 還原 | 用備份裡的密碼 + 鑰匙檔雜湊 + TOTP 金鑰重建保險庫（新的 RSA-4096 與 index key），所以使用者原本的密碼、鑰匙檔、驗證 App 都照常可用 |

**為什麼 RSA-8192 要在驗證前就開始產生**：產生一把 8192 位元金鑰在這台機器上量到 8–15 秒（3 次），較慢的電腦可能到一分鐘以上。匯出流程一開始就在背景產生，和使用者輸入密碼的時間重疊。

**驗證用兩個獨立實作**（`tests/interop_zip.py`）：7-Zip 能列出、測試並解開本程式產生的壓縮包（顯示 `AES-256 Store`，錯誤密碼會被拒絕）；pyzipper 能讀；本程式也能讀 pyzipper 寫的壓縮包。

**已知限制（請知悉）**
1. **ZIP 的檔名不會被加密**：知道壓縮包的人不需任何金鑰就看得到群組與 Token 的名稱（內容仍受保護）。這是 `<群組>/<名稱>.ini` 這個結構 + ZIP 格式的必然結果；若要隱藏名稱，需改用 7z 格式的標頭加密或把檔名改成亂數。
2. **壓縮包內含目前的主密碼**（依需求設計）。同時取得壓縮包與兩組金鑰的人，除了 Token，還會得到主密碼；如果你在別處也用同一組密碼，請改掉。
3. 讀取只支援「Stored（未壓縮）」的 AES 項目。請勿用其他軟體重新壓縮；本程式會明確報錯。
4. 還原時 Token 的建立／更新時間、有效期限、作廢狀態都會保留；匯入的是「備份當下」的快照。
5. 設定頁匯入的順序是：確認 → 完整解密並檢查備份 → 才刪除舊資料。若在刪除之後寫入失敗（例如磁碟已滿），舊資料已不在；目前沒有交易式還原。

## 多語系（i18n）
原始碼裡的 `tr()` 字串是繁體中文（來源語言）；英文是 Qt Linguist 格式的 `translations/TokenVault_en.ts`。選擇自己解析 `.ts`（`i18n.cpp`，約 60 行）而不用 `lrelease`，原因是 CI 要在 Windows / macOS / Linux 與五種 Linux 發行版上建置，少一個必須安裝的工具就少一個可能失敗的地方；代價是查詢只以「原文」為索引（忽略 context）。`tools/i18n_extract.py` 會掃描原始碼、同步 `.ts`，`--check` 模式有任何字串缺翻譯就失敗（已接上 ctest）；`tst_i18n` 另外檢查每一則翻譯的 `%1 %2` 佔位符與原文一致。語言預設跟隨系統，可在「設定 → 語言」切換（需重新啟動）。新增語言只要複製 `.ts`、加到 `i18n::supportedLanguages()` 和選單。

## 與你的規格不同之處（請確認）
1. **資料夾名稱**：`Token/<GroupId>/<6碼>.tkn` 的 `<Group>` 我用 **6 碼隨機 ID**，群組真名加密存在 `_group.grp`。原因：真名放在資料夾名會洩漏、特殊字元在 Windows 不合法、改名要搬資料夾。若你堅持用真名可以改，但要放棄上述三點。
2. **「隱藏變成一個點」**：輸入時用標準密碼點，並顯示字元數方便確認貼上；詳細頁未驗證前只顯示單一「●」，不洩漏長度。
3. 額外加了：永不過期選項、自動隱藏/清剪貼簿、閒置自動鎖定、變更主密碼（不需重新加密 Token）、輸錯 3 次後遞增等待、過期提醒、竄改偵測。

## 參考（憑記憶引用，這次對話沒有重新抓取，建議你抽查）
- OWASP Password Storage Cheat Sheet：PBKDF2-HMAC-SHA512 建議 ≥ 210,000 次（本專案取 600,000）。
- NIST SP 800-38D：GCM，96-bit IV。每次加密都用新的隨機 IV。
- RFC 8017：RSAES-OAEP。
- RFC 6238 附錄 B、RFC 4226、RFC 4648：TOTP/HOTP/Base32 測試向量，已寫進 `tests/tst_vault.cpp` 並通過（不是憑記憶，是程式實際算出來比對）。
- OpenSSL EVP API：`EVP_PKEY_encrypt`、`EVP_aes_256_gcm`、`PKCS5_PBKDF2_HMAC`。
- neil289506-commits/Pomodoro PR #3（有實際讀到，且該 PR 的五個 workflow 皆為綠燈）：Qt 6.8.3；Linux amd64 的 aqt arch 是 `linux_gcc_64`（不是 `gcc_64`）；Linux arm64 要用 `ubuntu-24.04-arm` + `linux_gcc_arm64`（Qt 6.8.3 arm64 套件需要 glibc ≥ 2.38，22.04 不行）；Windows ARM64 用 `win64_msvc2022_arm64_cross_compiled` 在 windows-2022 交叉編譯（`msvc_arch: amd64_arm64`），host Qt 放 `qt-host/`、target Qt 放 `qt-target/`，windeployqt 只用在 x64；ARM64 的 DLL 改為手動複製（見下方「Windows ARM64 的 DLL」）。
- Pomodoro PR #5 / #6 與 README（有實際讀到）：release 流程、Tag/名稱規則、`package.py` 的用途。
- **讀不到**：Pomodoro 的實際 `.yml` 全文（github.com 的 tree/blob 頁面禁止自動存取），所以 `release.yml`、`package.py` 以及 Windows ARM64 的 cmake 後半段參數（`QT_HOST_PATH` 等）是我依上述描述自行實作，不是複製。
- 先前版本（我寫的）用 `ubuntu-22.04-arm` 建 Linux ARM64 是錯的，已依上面的實測改為 `ubuntu-24.04-arm`。

## 已驗證 / 未驗證
已驗證（Ubuntu 24.04、Qt 6.4.2、OpenSSL 3.0.13、GCC 13、`-Wall -Wextra` 零警告）：
- 離屏環境用程式實際操作真的對話框（輸入文字、按按鈕）：首次設定→密碼＋鑰匙檔時 TOTP 被強制勾選→未掃描 TOTP 不能完成→掃描並輸入驗證碼→解鎖→錯誤驗證碼被拒→忘記密碼（需輸入「刪除全部」）→資料清空→重新進入設定。全部通過。
- `ctest`：建立 → 解鎖 → 加密/解密 → 改 meta → 更新 → 竄改偵測 → 換密碼 → 刪除，全部通過。
- Offscreen 啟動與截圖，主畫面 / 詳細頁 / 各對話框可正常繪製。

**未驗證**（這個環境沒有 Windows / macOS / ARM / Qt 6.8，也不能跑 GitHub Actions）：
- 所有 workflow 的實際執行。尤其是：Windows ARM64 的 cmake 參數與 vcpkg 交叉編譯 OpenSSL、macOS 在 arm64 runner 上用 vcpkg 編 x64-osx 的 OpenSSL 再 lipo 合併、`package.py` 以外的 release.yml。
- macOS 的剪貼簿「隱藏」旗標與 Windows 剪貼簿歷史排除旗標（盡力而為，失效不影響功能）。
- 未簽章：macOS 首次開啟會被 Gatekeeper 擋（右鍵 → 打開）；Windows 可能出現 SmartScreen。

## 已知限制
- QString/QLineEdit 內的密碼與 Token 無法保證從記憶體完全清除（Qt 限制）；位元組層級的金鑰都用 SecureBytes 清零。
- 刪除檔案時會先覆寫再刪除，但在 SSD / 寫時複製檔案系統上不保證實體清除（資料本身已加密）。
- 「作廢」是本機狀態標記，不會呼叫任何服務去真的撤銷該 Token。
- 有人能同時取得保險庫檔案與密碼/鑰匙檔就能解密；強度取決於你的密碼。
