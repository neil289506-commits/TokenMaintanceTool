<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US" sourcelanguage="zh_TW">
  <context>
    <name>TokenVault</name>
    <message>
      <source>%1 個 Token</source>
      <comment>mainwindow.cpp</comment>
      <translation>%1 token(s)</translation>
    </message>
    <message>
      <source>%1 個 Token · %2</source>
      <comment>mainwindow.cpp</comment>
      <translation>%1 token(s) · %2</translation>
    </message>
    <message>
      <source>%1 秒後自動鎖定</source>
      <comment>tokendetail.cpp</comment>
      <translation>Locks automatically in %1 s</translation>
    </message>
    <message>
      <source>1 天</source>
      <comment>editdialogs.cpp</comment>
      <translation>1 day</translation>
    </message>
    <message>
      <source>1 年</source>
      <comment>editdialogs.cpp</comment>
      <translation>1 year</translation>
    </message>
    <message>
      <source>14 天</source>
      <comment>editdialogs.cpp</comment>
      <translation>14 days</translation>
    </message>
    <message>
      <source>30 天</source>
      <comment>editdialogs.cpp</comment>
      <translation>30 days</translation>
    </message>
    <message>
      <source>48 位英文數字</source>
      <comment>backupui.cpp</comment>
      <translation>48 letters and digits</translation>
    </message>
    <message>
      <source>7 天</source>
      <comment>editdialogs.cpp</comment>
      <translation>7 days</translation>
    </message>
    <message>
      <source>TOTP 驗證碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>TOTP code</translation>
    </message>
    <message>
      <source>Token</source>
      <comment>editdialogs.cpp</comment>
      <translation>Token</translation>
    </message>
    <message>
      <source>Token %1/%2 無法讀取或已被竄改</source>
      <comment>vault.cpp</comment>
      <translation>Token %1/%2 cannot be read or was tampered with</translation>
    </message>
    <message>
      <source>Token 不存在</source>
      <comment>vault.cpp</comment>
      <translation>Token does not exist</translation>
    </message>
    <message>
      <source>Token 名稱</source>
      <comment>editdialogs.cpp</comment>
      <translation>Token name</translation>
    </message>
    <message>
      <source>Token 完整性檢查 (SHA-256) 失敗</source>
      <comment>vault.cpp</comment>
      <translation>Token integrity check (SHA-256) failed</translation>
    </message>
    <message>
      <source>Token 會用 AES-256 + RSA-4096 加密後存成 .tkn 檔。</source>
      <comment>editdialogs.cpp</comment>
      <translation>Tokens are encrypted with AES-256 + RSA-4096 and saved as .tkn files.</translation>
    </message>
    <message>
      <source>Token 期限提醒</source>
      <comment>mainwindow.cpp</comment>
      <translation>Token expiry reminder</translation>
    </message>
    <message>
      <source>Token 檔案不存在</source>
      <comment>vault.cpp</comment>
      <translation>Token file does not exist</translation>
    </message>
    <message>
      <source>Token 解密失敗（檔案可能已損毀或被竄改）</source>
      <comment>vault.cpp</comment>
      <translation>Token decryption failed (the file may be damaged or tampered with)</translation>
    </message>
    <message>
      <source>Token 詳細資料</source>
      <comment>tokendetail.cpp</comment>
      <translation>Token details</translation>
    </message>
    <message>
      <source>TokenVault</source>
      <comment>main.cpp</comment>
      <translation>TokenVault</translation>
    </message>
    <message>
      <source>Token「%1」解密失敗，已中止匯出</source>
      <comment>vault.cpp</comment>
      <translation>Token "%1" could not be decrypted; export aborted</translation>
    </message>
    <message>
      <source>ZIP 壓縮包 (*.zip)</source>
      <comment>backupui.cpp</comment>
      <translation>ZIP archive (*.zip)</translation>
    </message>
    <message>
      <source>ZIP 壓縮包 (*.zip);;所有檔案 (*)</source>
      <comment>backupui.cpp</comment>
      <translation>ZIP archive (*.zip);;All files (*)</translation>
    </message>
    <message>
      <source>• 這兩組金鑰只會顯示這一次，之後無法再查看或找回。
• 請分開保存，不要和壓縮包放在一起。
• 壓縮包內含目前的密碼與 TOTP 金鑰（皆已加密）。同時取得壓縮包與兩組金鑰的人，等於取得全部資料。
• 壓縮包的檔案清單會以明文顯示群組與 Token 的名稱（ZIP 格式的限制），內容則已加密。</source>
      <comment>backupui.cpp</comment>
      <translation>• These two keys are shown only once and cannot be viewed or recovered later.
• Store them separately, not next to the archive.
• The archive contains your current password and TOTP secret (both encrypted). Anyone who has the archive and both keys has all your data.
• The archive's file list shows group and token names in plain text (a limitation of the ZIP format); the contents are encrypted.</translation>
    </message>
    <message>
      <source>⚠ 密碼與鑰匙檔都無法找回。遺失後，保險庫內的所有 Token 將永遠無法解密（只能清空重來）。
鑰匙檔請勿與保險庫放在同一個位置，內容也不可被修改。</source>
      <comment>authdialogs.cpp</comment>
      <translation>⚠ Neither the password nor the key file can be recovered. If you lose them, every token in the vault is gone for good (the only way out is to wipe and start over).
Do not keep the key file next to the vault, and do not modify its contents.</translation>
    </message>
    <message>
      <source>✓ 已掃描並驗證，可以繼續。</source>
      <comment>authdialogs.cpp</comment>
      <translation>✓ Scanned and verified. You can continue.</translation>
    </message>
    <message>
      <source>✓ 驗證成功</source>
      <comment>authdialogs.cpp</comment>
      <translation>✓ Verified</translation>
    </message>
    <message>
      <source>「%1」的內容不完整</source>
      <comment>backup.cpp</comment>
      <translation>"%1" is incomplete</translation>
    </message>
    <message>
      <source>「%1」解密失敗，檔案可能已損毀或被竄改</source>
      <comment>backup.cpp</comment>
      <translation>"%1" could not be decrypted; the file may be damaged or tampered with</translation>
    </message>
    <message>
      <source>三角形</source>
      <comment>icons.cpp</comment>
      <translation>Triangle</translation>
    </message>
    <message>
      <source>下列內容會被永久刪除，無法復原：
• 所有群組
• 所有 Token（含名稱、說明、有效期限）
• 加密金鑰與 TOTP 設定</source>
      <comment>authdialogs.cpp</comment>
      <translation>The following will be permanently deleted and cannot be restored:
• All groups
• All tokens (names, notes, expiry dates)
• Encryption keys and TOTP settings</translation>
    </message>
    <message>
      <source>不支援的保險庫版本</source>
      <comment>vault.cpp</comment>
      <translation>Unsupported vault version</translation>
    </message>
    <message>
      <source>不是有效的備份壓縮包，或檔案已損毀</source>
      <comment>aeszip.cpp</comment>
      <translation>This is not a valid backup archive, or the file is damaged</translation>
    </message>
    <message>
      <source>以下 Token 即將在 3 天內到期：
%1</source>
      <comment>mainwindow.cpp</comment>
      <translation>These tokens expire within 3 days:
%1</translation>
    </message>
    <message>
      <source>以下 Token 已過期：
%1

</source>
      <comment>mainwindow.cpp</comment>
      <translation>These tokens have expired:
%1

</translation>
    </message>
    <message>
      <source>以密碼保護，至少 8 個字元。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Protect with a password of at least 8 characters.</translation>
    </message>
    <message>
      <source>任何檔案都可以。以檔案內容的 SHA-512 雜湊作為鎖，檔案被修改就打不開。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Any file works. Its SHA-512 hash is the lock; if the file changes, it will no longer open.</translation>
    </message>
    <message>
      <source>作廢</source>
      <comment>tokendetail.cpp</comment>
      <translation>Revoke</translation>
    </message>
    <message>
      <source>作廢 Token</source>
      <comment>tokendetail.cpp</comment>
      <translation>Revoke token</translation>
    </message>
    <message>
      <source>作廢 Token 需要驗證。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Revoking a token requires verification.</translation>
    </message>
    <message>
      <source>保持原有期限</source>
      <comment>editdialogs.cpp</comment>
      <translation>Keep current expiry</translation>
    </message>
    <message>
      <source>保險庫已存在</source>
      <comment>vault.cpp</comment>
      <translation>A vault already exists</translation>
    </message>
    <message>
      <source>保險庫已鎖定</source>
      <comment>vault.cpp</comment>
      <translation>The vault is locked</translation>
    </message>
    <message>
      <source>保險庫檔案已損毀</source>
      <comment>vault.cpp</comment>
      <translation>The vault file is damaged</translation>
    </message>
    <message>
      <source>備份內容過大</source>
      <comment>vault.cpp</comment>
      <translation>The backup is too large</translation>
    </message>
    <message>
      <source>備份壓縮包</source>
      <comment>backupui.cpp</comment>
      <translation>Backup archive</translation>
    </message>
    <message>
      <source>備份已建立</source>
      <comment>backupui.cpp</comment>
      <translation>Backup created</translation>
    </message>
    <message>
      <source>備份已建立。請確實保存剛才顯示的兩組金鑰。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Backup created. Make sure you keep the two keys that were just shown.</translation>
    </message>
    <message>
      <source>備份格式版本不支援</source>
      <comment>backup.cpp</comment>
      <translation>Unsupported backup format version</translation>
    </message>
    <message>
      <source>儲存備份壓縮包</source>
      <comment>backupui.cpp</comment>
      <translation>Save backup archive</translation>
    </message>
    <message>
      <source>儲存名稱與說明</source>
      <comment>tokendetail.cpp</comment>
      <translation>Save name and note</translation>
    </message>
    <message>
      <source>全部</source>
      <comment>mainwindow.cpp</comment>
      <translation>All</translation>
    </message>
    <message>
      <source>全部 Token</source>
      <comment>mainwindow.cpp</comment>
      <translation>All tokens</translation>
    </message>
    <message>
      <source>兩次輸入的密碼不一致</source>
      <comment>authdialogs.cpp</comment>
      <translation>The two passwords do not match</translation>
    </message>
    <message>
      <source>六邊形</source>
      <comment>icons.cpp</comment>
      <translation>Hexagon</translation>
    </message>
    <message>
      <source>再輸入一次</source>
      <comment>authdialogs.cpp</comment>
      <translation>Repeat password</translation>
    </message>
    <message>
      <source>刪除</source>
      <comment>tokendetail.cpp</comment>
      <translation>Delete</translation>
    </message>
    <message>
      <source>刪除 Token</source>
      <comment>tokendetail.cpp</comment>
      <translation>Delete token</translation>
    </message>
    <message>
      <source>刪除 Token 需要驗證。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Deleting a token requires verification.</translation>
    </message>
    <message>
      <source>刪除全部</source>
      <comment>authdialogs.cpp</comment>
      <translation>Delete everything</translation>
    </message>
    <message>
      <source>刪除含有 Token 的群組需要驗證。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Deleting a group that contains tokens requires verification.</translation>
    </message>
    <message>
      <source>刪除群組</source>
      <comment>mainwindow.cpp</comment>
      <translation>Delete group</translation>
    </message>
    <message>
      <source>剩 %1 天</source>
      <comment>icons.cpp</comment>
      <translation>%1 d left</translation>
    </message>
    <message>
      <source>剩 %1 小時</source>
      <comment>icons.cpp</comment>
      <translation>%1 h left</translation>
    </message>
    <message>
      <source>匯入</source>
      <comment>backupui.cpp</comment>
      <translation>Import</translation>
    </message>
    <message>
      <source>匯入備份</source>
      <comment>backupui.cpp</comment>
      <translation>Import backup</translation>
    </message>
    <message>
      <source>匯入備份…</source>
      <comment>mainwindow.cpp</comment>
      <translation>Import backup…</translation>
    </message>
    <message>
      <source>匯入完成。現在起請使用備份中的密碼／鑰匙檔／TOTP 解鎖。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Import complete. From now on unlock with the password / key file / TOTP from the backup.</translation>
    </message>
    <message>
      <source>匯入後會還原備份當時的密碼、鑰匙檔雜湊與 TOTP。若有使用鑰匙檔，請繼續使用原本那個檔案。</source>
      <comment>backupui.cpp</comment>
      <translation>Importing restores the password, key file hash and TOTP the backup was made with. If you use a key file, keep using the original file.</translation>
    </message>
    <message>
      <source>匯入會刪除目前所有資料</source>
      <comment>backupui.cpp</comment>
      <translation>Importing deletes all current data</translation>
    </message>
    <message>
      <source>匯出備份</source>
      <comment>backupui.cpp</comment>
      <translation>Export backup</translation>
    </message>
    <message>
      <source>匯出備份…</source>
      <comment>mainwindow.cpp</comment>
      <translation>Export backup…</translation>
    </message>
    <message>
      <source>匯出完成</source>
      <comment>backupui.cpp</comment>
      <translation>Export complete</translation>
    </message>
    <message>
      <source>匯出會把所有 Token 與驗證資訊打包，請先驗證身分。</source>
      <comment>backupui.cpp</comment>
      <translation>Export packs all tokens and authentication data. Please verify your identity first.</translation>
    </message>
    <message>
      <source>取消</source>
      <comment>uihelpers.cpp</comment>
      <translation>Cancel</translation>
    </message>
    <message>
      <source>同時使用密碼與鑰匙檔（雙重認證）時，必須搭配 TOTP</source>
      <comment>vault.cpp</comment>
      <translation>Using both a password and a key file (two-factor) requires TOTP as well</translation>
    </message>
    <message>
      <source>同時使用密碼與鑰匙檔（雙重認證）時，必須搭配 TOTP。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Using both a password and a key file (two-factor) requires TOTP as well.</translation>
    </message>
    <message>
      <source>名稱</source>
      <comment>editdialogs.cpp</comment>
      <translation>Name</translation>
    </message>
    <message>
      <source>圓形</source>
      <comment>icons.cpp</comment>
      <translation>Circle</translation>
    </message>
    <message>
      <source>圖片 (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;所有檔案 (*)</source>
      <comment>editdialogs.cpp</comment>
      <translation>Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All files (*)</translation>
    </message>
    <message>
      <source>圖片太大</source>
      <comment>icons.cpp</comment>
      <translation>The image is too large</translation>
    </message>
    <message>
      <source>圖示</source>
      <comment>editdialogs.cpp</comment>
      <translation>Icon</translation>
    </message>
    <message>
      <source>在此群組建立 Token</source>
      <comment>mainwindow.cpp</comment>
      <translation>Create token in this group</translation>
    </message>
    <message>
      <source>壓縮包內的「%1」使用了壓縮；請勿用其他軟體重新壓縮，直接使用匯出的原始檔案</source>
      <comment>aeszip.cpp</comment>
      <translation>"%1" in the archive is compressed. Do not recompress the archive with other software; use the exported file as-is.</translation>
    </message>
    <message>
      <source>壓縮包內的「%1」沒有使用 AES 加密，不是本程式匯出的檔案</source>
      <comment>aeszip.cpp</comment>
      <translation>"%1" in the archive is not AES-encrypted, so it was not exported by this program</translation>
    </message>
    <message>
      <source>壓縮包內的「%1」驗證失敗，檔案可能已損毀或被竄改</source>
      <comment>aeszip.cpp</comment>
      <translation>"%1" in the archive failed authentication; the file may be damaged or tampered with</translation>
    </message>
    <message>
      <source>壓縮包缺少檔案：%1</source>
      <comment>backup.cpp</comment>
      <translation>The archive is missing a file: %1</translation>
    </message>
    <message>
      <source>壓縮包過大</source>
      <comment>aeszip.cpp</comment>
      <translation>The archive is too large</translation>
    </message>
    <message>
      <source>完成</source>
      <comment>authdialogs.cpp</comment>
      <translation>Done</translation>
    </message>
    <message>
      <source>密碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Password</translation>
    </message>
    <message>
      <source>密碼至少需要 8 個字元</source>
      <comment>authdialogs.cpp</comment>
      <translation>The password needs at least 8 characters</translation>
    </message>
    <message>
      <source>密碼與鑰匙檔無法找回。要繼續使用，只能刪除整個保險庫後重新設定。</source>
      <comment>authdialogs.cpp</comment>
      <translation>The password and key file cannot be recovered. To keep using the app you must delete the whole vault and set it up again.</translation>
    </message>
    <message>
      <source>尚未設定。請點下方按鈕掃描 QR Code。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Not set up yet. Click the button below to scan the QR code.</translation>
    </message>
    <message>
      <source>已作廢</source>
      <comment>mainwindow.cpp</comment>
      <translation>Revoked</translation>
    </message>
    <message>
      <source>已有備份？匯入備份…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Have a backup? Import it…</translation>
    </message>
    <message>
      <source>已輸入 %1 個字元</source>
      <comment>editdialogs.cpp</comment>
      <translation>%1 characters entered</translation>
    </message>
    <message>
      <source>已過期</source>
      <comment>icons.cpp</comment>
      <translation>Expired</translation>
    </message>
    <message>
      <source>已鎖定。請驗證身分以繼續。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Locked. Verify your identity to continue.</translation>
    </message>
    <message>
      <source>建立 Token</source>
      <comment>editdialogs.cpp</comment>
      <translation>Create token</translation>
    </message>
    <message>
      <source>建立群組</source>
      <comment>editdialogs.cpp</comment>
      <translation>Create group</translation>
    </message>
    <message>
      <source>忘記密碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Forgot password</translation>
    </message>
    <message>
      <source>忘記密碼？</source>
      <comment>authdialogs.cpp</comment>
      <translation>Forgot password?</translation>
    </message>
    <message>
      <source>愛心</source>
      <comment>icons.cpp</comment>
      <translation>Heart</translation>
    </message>
    <message>
      <source>我已安全保存這兩組金鑰</source>
      <comment>backupui.cpp</comment>
      <translation>I have stored both keys safely</translation>
    </message>
    <message>
      <source>找不到或無法讀取鑰匙檔</source>
      <comment>authdialogs.cpp</comment>
      <translation>The key file was not found or cannot be read</translation>
    </message>
    <message>
      <source>找不到符合「%1」的 Token</source>
      <comment>mainwindow.cpp</comment>
      <translation>No tokens match "%1"</translation>
    </message>
    <message>
      <source>掃描 QR Code 設定…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Scan QR code…</translation>
    </message>
    <message>
      <source>搜尋名稱、說明或群組…</source>
      <comment>mainwindow.cpp</comment>
      <translation>Search name, note or group…</translation>
    </message>
    <message>
      <source>搜尋結果</source>
      <comment>mainwindow.cpp</comment>
      <translation>Search results</translation>
    </message>
    <message>
      <source>新 Token</source>
      <comment>editdialogs.cpp</comment>
      <translation>New token</translation>
    </message>
    <message>
      <source>新增</source>
      <comment>mainwindow.cpp</comment>
      <translation>New</translation>
    </message>
    <message>
      <source>方形</source>
      <comment>icons.cpp</comment>
      <translation>Square</translation>
    </message>
    <message>
      <source>星形</source>
      <comment>icons.cpp</comment>
      <translation>Star</translation>
    </message>
    <message>
      <source>更新 Token</source>
      <comment>editdialogs.cpp</comment>
      <translation>Renew token</translation>
    </message>
    <message>
      <source>更新 Token 需要驗證。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Renewing a token requires verification.</translation>
    </message>
    <message>
      <source>有效</source>
      <comment>tokendetail.cpp</comment>
      <translation>Valid</translation>
    </message>
    <message>
      <source>有效期限</source>
      <comment>editdialogs.cpp</comment>
      <translation>Expiry</translation>
    </message>
    <message>
      <source>有效期限必須晚於現在</source>
      <comment>editdialogs.cpp</comment>
      <translation>The expiry must be in the future</translation>
    </message>
    <message>
      <source>檔案數量過多</source>
      <comment>aeszip.cpp</comment>
      <translation>Too many files</translation>
    </message>
    <message>
      <source>歡迎使用 TokenVault</source>
      <comment>authdialogs.cpp</comment>
      <translation>Welcome to TokenVault</translation>
    </message>
    <message>
      <source>正在刪除資料…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Deleting data…</translation>
    </message>
    <message>
      <source>正在更新驗證方法…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Updating the authentication method…</translation>
    </message>
    <message>
      <source>正在產生 RSA-4096 金鑰並衍生加密金鑰，可能需要數秒…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Generating the RSA-4096 key and deriving encryption keys; this can take a few seconds…</translation>
    </message>
    <message>
      <source>正在產生 RSA-8192 金鑰並加密備份，可能需要數十秒…</source>
      <comment>backupui.cpp</comment>
      <translation>Generating the RSA-8192 key and encrypting the backup; this can take tens of seconds…</translation>
    </message>
    <message>
      <source>正在解密並檢查備份…</source>
      <comment>backupui.cpp</comment>
      <translation>Decrypting and checking the backup…</translation>
    </message>
    <message>
      <source>正在還原資料，請稍候…</source>
      <comment>backupui.cpp</comment>
      <translation>Restoring data, please wait…</translation>
    </message>
    <message>
      <source>每次驗證都要再輸入驗證 App 的 6 位數驗證碼。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Every verification also asks for the 6-digit code from your authenticator app.</translation>
    </message>
    <message>
      <source>永不過期</source>
      <comment>editdialogs.cpp</comment>
      <translation>Never expires</translation>
    </message>
    <message>
      <source>永久刪除</source>
      <comment>authdialogs.cpp</comment>
      <translation>Delete permanently</translation>
    </message>
    <message>
      <source>沒有說明</source>
      <comment>mainwindow.cpp</comment>
      <translation>No note</translation>
    </message>
    <message>
      <source>沿用目前的 TOTP 設定（不需要重新掃描）。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Keeping the current TOTP setup (no need to scan again).</translation>
    </message>
    <message>
      <source>清空所有資料並重新開始</source>
      <comment>authdialogs.cpp</comment>
      <translation>Erase all data and start over</translation>
    </message>
    <message>
      <source>清除並繼續</source>
      <comment>backupui.cpp</comment>
      <translation>Erase and continue</translation>
    </message>
    <message>
      <source>瀏覽…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Browse…</translation>
    </message>
    <message>
      <source>無效的群組</source>
      <comment>vault.cpp</comment>
      <translation>Invalid group</translation>
    </message>
    <message>
      <source>無法刪除檔案</source>
      <comment>vault.cpp</comment>
      <translation>Cannot delete the file</translation>
    </message>
    <message>
      <source>無法刪除群組資料夾</source>
      <comment>vault.cpp</comment>
      <translation>Cannot delete the group folder</translation>
    </message>
    <message>
      <source>無法完全刪除資料，請手動刪除資料夾：%1</source>
      <comment>vault.cpp</comment>
      <translation>The data could not be fully deleted. Please delete the folder manually: %1</translation>
    </message>
    <message>
      <source>無法掃描？手動輸入這組金鑰（類型：以時間為基礎）：</source>
      <comment>authdialogs.cpp</comment>
      <translation>Cannot scan? Enter this key manually (type: time-based):</translation>
    </message>
    <message>
      <source>無法讀取 vault.json</source>
      <comment>vault.cpp</comment>
      <translation>Cannot read vault.json</translation>
    </message>
    <message>
      <source>無法讀取這張圖片</source>
      <comment>icons.cpp</comment>
      <translation>Cannot read this image</translation>
    </message>
    <message>
      <source>無法讀取鑰匙檔：%1</source>
      <comment>vault.cpp</comment>
      <translation>Cannot read the key file: %1</translation>
    </message>
    <message>
      <source>無法開啟壓縮包：%1</source>
      <comment>aeszip.cpp</comment>
      <translation>Cannot open the archive: %1</translation>
    </message>
    <message>
      <source>現有的所有群組、Token、密碼／鑰匙檔與 TOTP 設定都會被永久刪除，改用備份裡的內容。</source>
      <comment>backupui.cpp</comment>
      <translation>All existing groups, tokens, the password / key file and TOTP settings will be permanently deleted and replaced by the backup.</translation>
    </message>
    <message>
      <source>用 Google / Microsoft Authenticator、Aegis、1Password 等 App 掃描 QR Code，再輸入 App 顯示的 6 位數驗證碼。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Scan the QR code with Google / Microsoft Authenticator, Aegis, 1Password or similar, then enter the 6-digit code the app shows.</translation>
    </message>
    <message>
      <source>用來解開 .zip 壓縮包（也可以用 7-Zip 直接開啟）。</source>
      <comment>backupui.cpp</comment>
      <translation>Unlocks the .zip archive (you can also open it directly with 7-Zip).</translation>
    </message>
    <message>
      <source>用來解開壓縮包裡的 RSA-8192 私鑰，匯入時需要。</source>
      <comment>backupui.cpp</comment>
      <translation>Unlocks the RSA-8192 private key inside the archive. Needed when importing.</translation>
    </message>
    <message>
      <source>盾牌</source>
      <comment>icons.cpp</comment>
      <translation>Shield</translation>
    </message>
    <message>
      <source>確定</source>
      <comment>uihelpers.cpp</comment>
      <translation>OK</translation>
    </message>
    <message>
      <source>確定要刪除空群組「%1」嗎？</source>
      <comment>mainwindow.cpp</comment>
      <translation>Delete the empty group "%1"?</translation>
    </message>
    <message>
      <source>確定要將「%1」標記為作廢嗎？
（Token 內容仍會保留，之後可以用「更新 Token」恢復。）</source>
      <comment>tokendetail.cpp</comment>
      <translation>Mark "%1" as revoked?
(The token itself is kept; you can restore it later with "Renew token".)</translation>
    </message>
    <message>
      <source>確定要永久刪除「%1」嗎？此動作無法復原。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Permanently delete "%1"? This cannot be undone.</translation>
    </message>
    <message>
      <source>確認清除並匯入</source>
      <comment>backupui.cpp</comment>
      <translation>Confirm erase and import</translation>
    </message>
    <message>
      <source>移除圖片</source>
      <comment>editdialogs.cpp</comment>
      <translation>Remove picture</translation>
    </message>
    <message>
      <source>立即鎖定</source>
      <comment>mainwindow.cpp</comment>
      <translation>Lock now</translation>
    </message>
    <message>
      <source>立即鎖定 (Ctrl+L)</source>
      <comment>mainwindow.cpp</comment>
      <translation>Lock now (Ctrl+L)</translation>
    </message>
    <message>
      <source>第一組金鑰（壓縮包）</source>
      <comment>backupui.cpp</comment>
      <translation>Key 1 (archive)</translation>
    </message>
    <message>
      <source>第一組金鑰（壓縮包）不正確</source>
      <comment>backup.cpp</comment>
      <translation>Key 1 (archive) is incorrect</translation>
    </message>
    <message>
      <source>第一組金鑰：壓縮包</source>
      <comment>backupui.cpp</comment>
      <translation>Key 1: archive</translation>
    </message>
    <message>
      <source>第二組金鑰（RSA）</source>
      <comment>backupui.cpp</comment>
      <translation>Key 2 (RSA)</translation>
    </message>
    <message>
      <source>第二組金鑰（RSA）不正確</source>
      <comment>backup.cpp</comment>
      <translation>Key 2 (RSA) is incorrect</translation>
    </message>
    <message>
      <source>第二組金鑰：RSA</source>
      <comment>backupui.cpp</comment>
      <translation>Key 2: RSA</translation>
    </message>
    <message>
      <source>編輯群組</source>
      <comment>editdialogs.cpp</comment>
      <translation>Edit group</translation>
    </message>
    <message>
      <source>缺少 RSA-8192 金鑰</source>
      <comment>backup.cpp</comment>
      <translation>Missing RSA-8192 key</translation>
    </message>
    <message>
      <source>缺少 TOTP 金鑰</source>
      <comment>vault.cpp</comment>
      <translation>Missing TOTP secret</translation>
    </message>
    <message>
      <source>群組</source>
      <comment>editdialogs.cpp</comment>
      <translation>Groups</translation>
    </message>
    <message>
      <source>群組 %1 無法讀取或已被竄改</source>
      <comment>vault.cpp</comment>
      <translation>Group %1 cannot be read or was tampered with</translation>
    </message>
    <message>
      <source>群組「%1」內有 %2 個 Token，全部都會被永久刪除。確定嗎？</source>
      <comment>mainwindow.cpp</comment>
      <translation>Group "%1" contains %2 tokens. All of them will be permanently deleted. Continue?</translation>
    </message>
    <message>
      <source>群組不存在</source>
      <comment>vault.cpp</comment>
      <translation>The group does not exist</translation>
    </message>
    <message>
      <source>群組名稱</source>
      <comment>editdialogs.cpp</comment>
      <translation>Group name</translation>
    </message>
    <message>
      <source>群組用來整理 Token，可以自訂名稱、圖示與說明。</source>
      <comment>editdialogs.cpp</comment>
      <translation>Groups keep your tokens organised. Choose a name, icon and note.</translation>
    </message>
    <message>
      <source>群組：%1
編號：%2.tkn
建立：%3
更新：%4
到期：%5</source>
      <comment>tokendetail.cpp</comment>
      <translation>Group: %1
File: %2.tkn
Created: %3
Updated: %4
Expires: %5</translation>
    </message>
    <message>
      <source>自動鎖定</source>
      <comment>mainwindow.cpp</comment>
      <translation>Auto-lock</translation>
    </message>
    <message>
      <source>自動鎖定時間…</source>
      <comment>mainwindow.cpp</comment>
      <translation>Auto-lock time…</translation>
    </message>
    <message>
      <source>自訂</source>
      <comment>editdialogs.cpp</comment>
      <translation>Custom</translation>
    </message>
    <message>
      <source>自訂圖片（選填，會優先於下面的圖示）</source>
      <comment>editdialogs.cpp</comment>
      <translation>Custom picture (optional; takes priority over the icon below)</translation>
    </message>
    <message>
      <source>菱形</source>
      <comment>icons.cpp</comment>
      <translation>Diamond</translation>
    </message>
    <message>
      <source>複製</source>
      <comment>authdialogs.cpp</comment>
      <translation>Copy</translation>
    </message>
    <message>
      <source>解鎖保險庫</source>
      <comment>authdialogs.cpp</comment>
      <translation>Unlock vault</translation>
    </message>
    <message>
      <source>設定</source>
      <comment>mainwindow.cpp</comment>
      <translation>Settings</translation>
    </message>
    <message>
      <source>設定 TOTP</source>
      <comment>authdialogs.cpp</comment>
      <translation>Set up TOTP</translation>
    </message>
    <message>
      <source>設定 TOTP 驗證碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Set up TOTP codes</translation>
    </message>
    <message>
      <source>設定密碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Set a password</translation>
    </message>
    <message>
      <source>設定用來保護所有 Token 的方式。資料以 SHA-512 + AES-256 + RSA-4096 加密後只存放在這台電腦。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Choose how all your tokens are protected. Data is encrypted with SHA-512 + AES-256 + RSA-4096 and stays on this computer.</translation>
    </message>
    <message>
      <source>說明</source>
      <comment>editdialogs.cpp</comment>
      <translation>Note</translation>
    </message>
    <message>
      <source>說明（選填）</source>
      <comment>editdialogs.cpp</comment>
      <translation>Note (optional)</translation>
    </message>
    <message>
      <source>請先完成 TOTP 設定（掃描 QR Code 並輸入驗證碼）</source>
      <comment>authdialogs.cpp</comment>
      <translation>Finish the TOTP setup first (scan the QR code and enter a code)</translation>
    </message>
    <message>
      <source>請先建立一個群組。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Please create a group first.</translation>
    </message>
    <message>
      <source>請先設定 TOTP</source>
      <comment>vault.cpp</comment>
      <translation>Please set up TOTP first</translation>
    </message>
    <message>
      <source>請稍候</source>
      <comment>busy.h</comment>
      <translation>Please wait</translation>
    </message>
    <message>
      <source>請等待 %1 秒</source>
      <comment>authdialogs.cpp</comment>
      <translation>Wait %1 s</translation>
    </message>
    <message>
      <source>請至少選擇「密碼」或「鑰匙檔」其中一種</source>
      <comment>vault.cpp</comment>
      <translation>Choose at least a password or a key file</translation>
    </message>
    <message>
      <source>請輸入 6 位數驗證碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Enter the 6-digit code</translation>
    </message>
    <message>
      <source>請輸入 TOTP 驗證碼</source>
      <comment>vault.cpp</comment>
      <translation>Enter the TOTP code</translation>
    </message>
    <message>
      <source>請輸入 Token</source>
      <comment>editdialogs.cpp</comment>
      <translation>Enter the token</translation>
    </message>
    <message>
      <source>請輸入「刪除全部」以確認</source>
      <comment>authdialogs.cpp</comment>
      <translation>Type "Delete everything" to confirm</translation>
    </message>
    <message>
      <source>請輸入名稱</source>
      <comment>editdialogs.cpp</comment>
      <translation>Enter a name</translation>
    </message>
    <message>
      <source>請輸入密碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Enter the password</translation>
    </message>
    <message>
      <source>請輸入第一組金鑰（壓縮包）</source>
      <comment>backup.cpp</comment>
      <translation>Enter key 1 (archive)</translation>
    </message>
    <message>
      <source>請輸入第二組金鑰（RSA）</source>
      <comment>backup.cpp</comment>
      <translation>Enter key 2 (RSA)</translation>
    </message>
    <message>
      <source>請輸入群組名稱</source>
      <comment>editdialogs.cpp</comment>
      <translation>Enter a group name</translation>
    </message>
    <message>
      <source>請輸入驗證資訊以繼續。</source>
      <comment>authdialogs.cpp</comment>
      <translation>Enter your credentials to continue.</translation>
    </message>
    <message>
      <source>請選擇備份壓縮包</source>
      <comment>backupui.cpp</comment>
      <translation>Choose a backup archive</translation>
    </message>
    <message>
      <source>請選擇鑰匙檔</source>
      <comment>authdialogs.cpp</comment>
      <translation>Choose a key file</translation>
    </message>
    <message>
      <source>變更有效期限需要驗證。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Changing the expiry requires verification.</translation>
    </message>
    <message>
      <source>變更期限</source>
      <comment>tokendetail.cpp</comment>
      <translation>Change expiry</translation>
    </message>
    <message>
      <source>貼上或輸入 Token（輸入後會隱藏）</source>
      <comment>editdialogs.cpp</comment>
      <translation>Paste or type the token (it is hidden once entered)</translation>
    </message>
    <message>
      <source>身分驗證</source>
      <comment>mainwindow.cpp</comment>
      <translation>Verification</translation>
    </message>
    <message>
      <source>輸入 App 顯示的驗證碼以確認設定成功</source>
      <comment>authdialogs.cpp</comment>
      <translation>Enter the code shown in the app to confirm the setup</translation>
    </message>
    <message>
      <source>輸入「%1」的新 Token。舊內容會被覆蓋，且會清除「已作廢」狀態。</source>
      <comment>editdialogs.cpp</comment>
      <translation>Enter the new token for "%1". The old value is overwritten and the "revoked" state is cleared.</translation>
    </message>
    <message>
      <source>輸入密碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>Enter password</translation>
    </message>
    <message>
      <source>這個 Token 已經是作廢狀態。</source>
      <comment>tokendetail.cpp</comment>
      <translation>This token is already revoked.</translation>
    </message>
    <message>
      <source>這個動作無法復原。請輸入下面這句英文以確認：</source>
      <comment>backupui.cpp</comment>
      <translation>This cannot be undone. Type the sentence below to confirm:</translation>
    </message>
    <message>
      <source>這裡還沒有 Token
點左下角的 ＋ → 建立 Token</source>
      <comment>mainwindow.cpp</comment>
      <translation>No tokens here yet
Click + at the bottom left → Create token</translation>
    </message>
    <message>
      <source>選擇 .zip 備份檔</source>
      <comment>backupui.cpp</comment>
      <translation>Choose a .zip backup file</translation>
    </message>
    <message>
      <source>選擇備份壓縮包</source>
      <comment>backupui.cpp</comment>
      <translation>Choose the backup archive</translation>
    </message>
    <message>
      <source>選擇備份壓縮包，並輸入匯出時產生的兩組金鑰。</source>
      <comment>backupui.cpp</comment>
      <translation>Choose the backup archive and enter the two keys generated when it was exported.</translation>
    </message>
    <message>
      <source>選擇圖片…</source>
      <comment>editdialogs.cpp</comment>
      <translation>Choose picture…</translation>
    </message>
    <message>
      <source>選擇新的保護方式。RSA 金鑰與既有的 Token 檔案不需要重新加密；完成後舊的密碼／鑰匙檔就不能用了。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Choose the new protection. The RSA key and existing token files do not need to be re-encrypted; afterwards the old password / key file stops working.</translation>
    </message>
    <message>
      <source>選擇群組圖片</source>
      <comment>editdialogs.cpp</comment>
      <translation>Choose group picture</translation>
    </message>
    <message>
      <source>選擇鑰匙檔</source>
      <comment>authdialogs.cpp</comment>
      <translation>Choose key file</translation>
    </message>
    <message>
      <source>選擇鑰匙檔（任何檔案皆可）</source>
      <comment>authdialogs.cpp</comment>
      <translation>Choose a key file (any file works)</translation>
    </message>
    <message>
      <source>還沒有任何群組</source>
      <comment>mainwindow.cpp</comment>
      <translation>No groups yet</translation>
    </message>
    <message>
      <source>部分資料無法讀取</source>
      <comment>mainwindow.cpp</comment>
      <translation>Some data could not be read</translation>
    </message>
    <message>
      <source>部分資料無法讀取，已中止匯出以免備份不完整：
</source>
      <comment>vault.cpp</comment>
      <translation>Some data could not be read; export aborted so the backup is not incomplete:
</translation>
    </message>
    <message>
      <source>重新產生 TOTP 金鑰（需要重新掃描）</source>
      <comment>authdialogs.cpp</comment>
      <translation>Generate a new TOTP secret (needs scanning again)</translation>
    </message>
    <message>
      <source>重新設定…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Set up again…</translation>
    </message>
    <message>
      <source>重設驗證方法</source>
      <comment>mainwindow.cpp</comment>
      <translation>Reset authentication method</translation>
    </message>
    <message>
      <source>重設驗證方法前，必須先輸入目前的密碼／鑰匙檔／驗證碼。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Before resetting, enter your current password / key file / code.</translation>
    </message>
    <message>
      <source>重設驗證方法（密碼／鑰匙檔／TOTP）…</source>
      <comment>mainwindow.cpp</comment>
      <translation>Reset authentication method (password / key file / TOTP)…</translation>
    </message>
    <message>
      <source>鎖定</source>
      <comment>mainwindow.cpp</comment>
      <translation>Lock</translation>
    </message>
    <message>
      <source>鑰匙檔</source>
      <comment>authdialogs.cpp</comment>
      <translation>Key file</translation>
    </message>
    <message>
      <source>鑰匙檔雜湊長度不正確</source>
      <comment>vault.cpp</comment>
      <translation>The key file hash has the wrong length</translation>
    </message>
    <message>
      <source>開啟</source>
      <comment>mainwindow.cpp</comment>
      <translation>Open</translation>
    </message>
    <message>
      <source>閒置幾分鐘後自動鎖定？（0 = 不自動鎖定）</source>
      <comment>mainwindow.cpp</comment>
      <translation>Lock after how many idle minutes? (0 = never)</translation>
    </message>
    <message>
      <source>關閉</source>
      <comment>tokendetail.cpp</comment>
      <translation>Close</translation>
    </message>
    <message>
      <source>離開</source>
      <comment>mainwindow.cpp</comment>
      <translation>Quit</translation>
    </message>
    <message>
      <source>需要重新啟動才能套用語言，重新啟動後需要重新解鎖。現在重新啟動嗎？</source>
      <comment>mainwindow.cpp</comment>
      <translation>The app must restart to apply the language, and you will need to unlock again. Restart now?</translation>
    </message>
    <message>
      <source>需要驗證才能顯示 Token。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Verification is required to show the token.</translation>
    </message>
    <message>
      <source>顏色</source>
      <comment>editdialogs.cpp</comment>
      <translation>Color</translation>
    </message>
    <message>
      <source>顯示</source>
      <comment>tokendetail.cpp</comment>
      <translation>Show</translation>
    </message>
    <message>
      <source>顯示、作廢、更新、刪除與變更期限都需要重新驗證；名稱與說明不需要。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Showing, revoking, renewing, deleting and changing the expiry require verification again; the name and note do not.</translation>
    </message>
    <message>
      <source>顯示金鑰</source>
      <comment>backupui.cpp</comment>
      <translation>Show keys</translation>
    </message>
    <message>
      <source>驗證</source>
      <comment>authdialogs.cpp</comment>
      <translation>Verify</translation>
    </message>
    <message>
      <source>驗證 App 上的 6 位數驗證碼</source>
      <comment>authdialogs.cpp</comment>
      <translation>6-digit code from your authenticator app</translation>
    </message>
    <message>
      <source>驗證中…</source>
      <comment>authdialogs.cpp</comment>
      <translation>Verifying…</translation>
    </message>
    <message>
      <source>驗證失敗：密碼、鑰匙檔或驗證碼不正確</source>
      <comment>vault.cpp</comment>
      <translation>Verification failed: the password, key file or code is incorrect</translation>
    </message>
    <message>
      <source>驗證成功。倒數結束後會自動鎖定。</source>
      <comment>tokendetail.cpp</comment>
      <translation>Verified. It locks again when the countdown ends.</translation>
    </message>
    <message>
      <source>驗證方法已更新。下次解鎖與每次敏感操作都會使用新的設定。</source>
      <comment>mainwindow.cpp</comment>
      <translation>Authentication method updated. The new settings apply from the next unlock and to every sensitive action.</translation>
    </message>
    <message>
      <source>驗證目前身分</source>
      <comment>mainwindow.cpp</comment>
      <translation>Verify your current identity</translation>
    </message>
    <message>
      <source>驗證碼不正確，請確認手機時間是否準確</source>
      <comment>authdialogs.cpp</comment>
      <translation>Incorrect code. Check that your phone's clock is accurate.</translation>
    </message>
    <message>
      <source>驗證身分</source>
      <comment>backupui.cpp</comment>
      <translation>Verify identity</translation>
    </message>
    <message>
      <source>點左下角的 ＋ 建立第一個群組，再把 Token 放進去</source>
      <comment>mainwindow.cpp</comment>
      <translation>Click + at the bottom left to create your first group, then add tokens to it</translation>
    </message>
  </context>
</TS>