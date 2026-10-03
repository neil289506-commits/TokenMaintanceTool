#!/usr/bin/env python3
"""Turn a platform .tar (made on the runner) into the release zip, keeping file modes and symlinks,
and add BUILD_INFO.txt. upload-artifact drops Unix modes/symlinks, which is why the runners pass tar files.

usage: package.py <platform> <in.tar> <out.zip> <cp-number>
env:   GITHUB_SHA, GITHUB_SERVER_URL, GITHUB_REPOSITORY, GITHUB_RUN_ID (all optional)
"""
import os, stat, sys, tarfile, zipfile, time
from datetime import datetime, timedelta, timezone


def main() -> int:
    platform, tar_path, out_zip, cp = sys.argv[1:5]
    now = datetime.now(timezone.utc) + timedelta(hours=8)          # UTC+8, like the Pomodoro releases
    sha = os.environ.get("GITHUB_SHA", "unknown")
    run = "{}/{}/actions/runs/{}".format(os.environ.get("GITHUB_SERVER_URL", "https://github.com"),
                                         os.environ.get("GITHUB_REPOSITORY", "?"), os.environ.get("GITHUB_RUN_ID", "?"))
    info = (f"TokenVault {platform}\nCP: {cp}\ncommit: {sha}\nrun: {run}\n"
            f"built: {now:%Y-%m-%d %H:%M:%S} (UTC+8)\nQt: 6.8.3\n")
    root = f"TokenVault_{platform}"
    with tarfile.open(tar_path) as tf, zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED) as zf:
        for m in tf.getmembers():
            name = f"{root}/{m.name.lstrip('./')}"
            zi = zipfile.ZipInfo(name + ("/" if m.isdir() else ""), time.localtime(m.mtime)[:6])
            zi.create_system = 3                                    # Unix: external_attr carries the mode
            if m.isdir():
                zi.external_attr = ((stat.S_IFDIR | (m.mode & 0o7777)) << 16) | 0x10
                zf.writestr(zi, b"")
            elif m.issym():
                zi.external_attr = (stat.S_IFLNK | 0o777) << 16
                zf.writestr(zi, m.linkname)
            elif m.isreg():
                zi.external_attr = (stat.S_IFREG | (m.mode & 0o7777)) << 16
                zi.compress_type = zipfile.ZIP_DEFLATED
                zf.writestr(zi, tf.extractfile(m).read())
        bi = zipfile.ZipInfo(f"{root}/BUILD_INFO.txt", time.localtime()[:6])
        bi.create_system = 3
        bi.external_attr = (stat.S_IFREG | 0o644) << 16
        zf.writestr(bi, info)
    return 0


if __name__ == "__main__":
    sys.exit(main())
