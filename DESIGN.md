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

## 與你的規格不同之處（請確認）
1. **資料夾名稱**：`Token/<GroupId>/<6碼>.tkn` 的 `<Group>` 我用 **6 碼隨機 ID**，群組真名加密存在 `_group.grp`。原因：真名放在資料夾名會洩漏、特殊字元在 Windows 不合法、改名要搬資料夾。若你堅持用真名可以改，但要放棄上述三點。
2. **「隱藏變成一個點」**：輸入時用標準密碼點，並顯示字元數方便確認貼上；詳細頁未驗證前只顯示單一「●」，不洩漏長度。
3. 額外加了：永不過期選項、自動隱藏/清剪貼簿、閒置自動鎖定、變更主密碼（不需重新加密 Token）、輸錯 3 次後遞增等待、過期提醒、竄改偵測。

## 參考（憑記憶引用，這次對話沒有重新抓取，建議你抽查）
- OWASP Password Storage Cheat Sheet：PBKDF2-HMAC-SHA512 建議 ≥ 210,000 次（本專案取 600,000）。
- NIST SP 800-38D：GCM，96-bit IV。每次加密都用新的隨機 IV。
- RFC 8017：RSAES-OAEP。
- OpenSSL EVP API：`EVP_PKEY_encrypt`、`EVP_aes_256_gcm`、`PKCS5_PBKDF2_HMAC`。
- neil289506-commits/Pomodoro PR #3（有實際讀到，且該 PR 的五個 workflow 皆為綠燈）：Qt 6.8.3；Linux amd64 的 aqt arch 是 `linux_gcc_64`（不是 `gcc_64`）；Linux arm64 要用 `ubuntu-24.04-arm` + `linux_gcc_arm64`（Qt 6.8.3 arm64 套件需要 glibc ≥ 2.38，22.04 不行）；Windows ARM64 用 `win64_msvc2022_arm64_cross_compiled` 在 windows-2022 交叉編譯（`msvc_arch: amd64_arm64`），host Qt 放 `qt-host/`、target Qt 放 `qt-target/`，windeployqt 只用在 x64。
- Pomodoro PR #5 / #6 與 README（有實際讀到）：release 流程、Tag/名稱規則、`package.py` 的用途。
- **讀不到**：Pomodoro 的實際 `.yml` 全文（github.com 的 tree/blob 頁面禁止自動存取），所以 `release.yml`、`package.py` 以及 Windows ARM64 的 cmake 後半段參數（`QT_HOST_PATH` 等）是我依上述描述自行實作，不是複製。
- 先前版本（我寫的）用 `ubuntu-22.04-arm` 建 Linux ARM64 是錯的，已依上面的實測改為 `ubuntu-24.04-arm`。

## 已驗證 / 未驗證
已驗證（Ubuntu 24.04、Qt 6.4.2、OpenSSL 3.0.13、GCC 13、`-Wall -Wextra` 零警告）：
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
