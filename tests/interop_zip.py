#!/usr/bin/env python3
"""Checks that the backup archive written by TokenVault is a standard WinZip-AES zip, using two independent
implementations (7-Zip and pyzipper), and that TokenVault can read a zip written by pyzipper.

Not part of ctest (needs 7z and `pip install pyzipper`). Usage, from the build directory:
    python3 ../tests/interop_zip.py ./tst_backup
"""
import os, subprocess, sys, tempfile

try:
    import pyzipper
except ImportError:
    sys.exit("pip install pyzipper")

exe = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "./tst_backup")
tmp = tempfile.mkdtemp()
zip_path = os.path.join(tmp, "backup.zip")

# 1) TokenVault -> 7-Zip / pyzipper
env = dict(os.environ, TV_TEST_ZIP_OUT=zip_path)
subprocess.run([exe, "fullBackupAndRestore"], check=True, env=env, stdout=subprocess.DEVNULL)
key1 = open(zip_path + ".keys").read().split()[0]

out = subprocess.run(["7z", "t", "-p" + key1, zip_path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
assert "Everything is Ok" in out, out
bad = subprocess.run(["7z", "t", "-pwrong-password", zip_path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
assert "Wrong password" in bad, bad
listing = subprocess.run(["7z", "l", "-slt", "-p" + key1, zip_path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
assert "AES-256 Store" in listing, listing

with pyzipper.AESZipFile(zip_path) as z:
    z.setpassword(key1.encode())
    assert z.testzip() is None
    names = z.namelist()
    assert "passcode.txt" in names and "rsa8192.key" in names
print("TokenVault -> 7-Zip / pyzipper: OK (%d entries)" % len(names))

# 2) pyzipper -> TokenVault
foreign = os.path.join(tmp, "foreign.zip")
with pyzipper.AESZipFile(foreign, "w", compression=pyzipper.ZIP_STORED, encryption=pyzipper.WZ_AES) as z:
    z.setpassword(b"pyzipper-pass-123")
    z.setencryption(pyzipper.WZ_AES, nbits=256)
    z.writestr("a.txt", b"hello from pyzipper")
    z.writestr("中文/子資料夾/檔案.ini", b"[x]\na=1\n")
    z.writestr("big.bin", bytes((i * 7 + 3) % 251 for i in range(70000)))
env = dict(os.environ, TV_TEST_ZIP_IN=foreign, TV_TEST_ZIP_PW="pyzipper-pass-123")
subprocess.run([exe, "foreignZipFromPyzipper"], check=True, env=env, stdout=subprocess.DEVNULL)
print("pyzipper -> TokenVault: OK")
